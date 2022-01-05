/****************************************************************
 * file dumper_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It contains the heap dumping machinery.
 *
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      Abhishek Chakravarti <abhishek@taranjali.org>
 *      Nimesh Neema <nimeshneema@gmail.com>
 *
 *      © Copyright 2019 - 2022 The Reflective Persistent System Team
 *      team@refpersys.org & http://refpersys.org/
 *
 * License:
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "Refpersys.h"
/* for mallopt: */
#include <malloc.h>

#define RPS_DUMPER_MAGIC 0x2501f5e3	/*620885475 */

// A temporary limit on the number of spaces; it will be much larger
// once dumping code has been generated...
#define RPS_DUMP_MAX_NB_SPACE 32

struct RpsPayl_Dumper_st
{
  RPSFIELDS_ZONED_VALUE;
  unsigned du_magic;		/* always RPS_DUMPER_MAGIC */
  rps_callframe_t *du_callframe;
  double du_start_realtime;
  double du_start_cputime;
  /* The dumper probably needs to contain a big hash table of visited
     objects; a first pass is scanning the heap, starting from global
     roots including the agenda. During the dump only one pthread should
     be running, every other pthread should be blocked. The agenda should
     be "idle".
   */
  const RpsString_t *du_dirnam;
  // the large hash table of visited objects....
  RpsHashTblOb_t *du_visitedht;
  // the smaller hash table of visited spaces...
  RpsHashTblOb_t *du_spaceht;
  // the smaller hash table for the current space
  RpsHashTblOb_t *du_htcurspace;
  // the queue of object to scan internally....
  RpsDequeOb_t *du_deque;
  // small set of space objects; at most RPS_DUMP_MAX_NB_SPACE elements
  const RpsSetOb_t *du_spaceset;
  // for each dumped space
  struct
  {
    unsigned sp_size;
    FILE *sp_file;
  } du_spacedescr[RPS_DUMP_MAX_NB_SPACE];
};				/* end struct RpsPayl_Dumper_st */


// In principle, we don't need a global for the dumper. In practice,
// it should help debugging with GDB....
//
// TODO: remove it when the dump is working completely....
static RpsDumper_t *rps_the_dumper;


void rps_dump_one_space (RpsDumper_t * du, int spix,
			 const RpsObject_t * spacob,
			 const RpsSetOb_t * universet);

void rps_dump_object_in_space (RpsDumper_t * du, int spix, FILE * spfil,
			       const RpsObject_t * obj, int obix);

bool
rps_is_valid_dumper (RpsDumper_t * du)
{
  if (!du)
    return false;
  if (du->du_magic != RPS_DUMPER_MAGIC)
    return false;
  return true;
}				/* end rps_is_valid_dumper */


void
rps_dumper_scan_internal_object (RpsDumper_t * du, RpsObject_t * ob)
{
  RPS_ASSERT (du && du->du_magic == RPS_DUMPER_MAGIC);
  RPS_ASSERT (ob && rps_is_valid_object (ob));
  RPS_ASSERT (du->du_spaceht
	      && du->du_spaceht->htbob_magic == RPS_HTBOB_MAGIC);
  char oidbuf[32];
  memset (oidbuf, 0, sizeof (oidbuf));
  rps_oid_to_cbuf (ob->ob_id, oidbuf);
  RPS_DEBUG_NLPRINTF (DUMP, "start scan-internal-ob %s", oidbuf);
  pthread_mutex_lock (&ob->ob_mtx);
  rps_dumper_scan_object (du, ob->ob_class);
  if (ob->ob_space)
    {
      RPS_ASSERT (rps_is_valid_object (ob->ob_space));
      (void) rps_hash_tbl_ob_add (du->du_spaceht, ob->ob_space);
      rps_dumper_scan_object (du, ob->ob_space);
    }
  /// scan the table of attributes
  {
    RpsAttrTable_t *atbl = ob->ob_attrtable;
    if (atbl)
      {
	RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (atbl) == -RpsPyt_AttrTable);
	intptr_t atblsiz = rps_prime_of_index (atbl->zm_xtra);
	unsigned atbllen = atbl->zm_length;
	RPS_ASSERT (atbllen <= atblsiz);
	for (int aix = 0; aix < atblsiz; aix++)
	  {
	    RpsObject_t *curattrob = atbl->attr_entries[aix].ent_attr;
	    if (curattrob != NULL && curattrob != RPS_HTB_EMPTY_SLOT)
	      {
		rps_dumper_scan_object (du, curattrob);
		rps_dumper_scan_value (du, atbl->attr_entries[aix].ent_val,
				       0);
	      }
	  }
      };
  }
  /// scan the components
  {
    unsigned nbcomp = ob->ob_nbcomp;
    RpsValue_t *comparr = ob->ob_comparr;
    for (int cix = 0; cix < (int) nbcomp; cix++)
      if (comparr[cix] != RPS_NULL_VALUE)
	rps_dumper_scan_value (du, comparr[cix], 0);
  }
  /// scan the payload
  {
    struct rps_owned_payload_st *payl =
      (struct rps_owned_payload_st *) (ob->ob_payload);
    if (payl)
      {
	RPS_ASSERT (payl->payl_owner == ob);
	rps_dump_scan_object_payload (du, ob);
      }
  };
end:
  pthread_mutex_unlock (&ob->ob_mtx);
  RPS_DEBUG_PRINTF (DUMP, "end scan-internal-ob %s\n", oidbuf);
}				/* end rps_dumper_scan_internal_object */


void
rps_dumper_scan_value (RpsDumper_t * du, RpsValue_t val, unsigned depth)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  if (val == RPS_NULL_VALUE)
    return;
  if (depth > RPS_MAX_VALUE_DEPTH)
    RPS_FATAL ("too deep %u value to scan @%p", depth, (void *) val);
  enum RpsType vtyp = rps_value_type (val);
  RPS_DEBUG_PRINTF (DUMP, "scan-val depth %u val %s %#lx", depth,
		    rps_type_str ((int) vtyp), val);
  switch (vtyp)
    {
    case RPS_TYPE_INT:
      return;
    case RPS_TYPE_STRING:
      return;
    case RPS_TYPE_JSON:
      return;
    case RPS_TYPE_GTKWIDGET:
      return;
    case RPS_TYPE_TUPLE:
      {
	const RpsTupleOb_t *tupv = (const RpsTupleOb_t *) val;
	for (uint32_t ix = 0; ix < tupv->zm_length; ix++)
	  if (tupv->tuple_comp[ix])
	    rps_dumper_scan_object (du,
				    (RpsObject_t *) (tupv->tuple_comp[ix]));
      };
      return;
    case RPS_TYPE_SET:
      {
	const RpsSetOb_t *setv = (const RpsSetOb_t *) val;
	for (uint32_t ix = 0; ix < setv->zm_length; ix++)
	  rps_dumper_scan_object (du, (RpsObject_t *) (setv->set_elem[ix]));
      };
      return;
    case RPS_TYPE_CLOSURE:
      {
	const RpsClosure_t *closv = (const RpsClosure_t *) val;
	rps_dumper_scan_object (du, closv->clos_conn);
	if (closv->clos_meta)
	  rps_dumper_scan_value (du, closv->clos_meta, depth + 1);
	for (uint32_t ix = 0; ix < closv->zm_length; ix++)
	  rps_dumper_scan_value (du, closv->clos_val[ix], depth + 1);
      };
      return;
    case RPS_TYPE_OBJECT:
      {
	RpsObject_t *ob = (RpsObject_t *) val;
	rps_dumper_scan_object (du, ob);
      };
      return;
    case RPS_TYPE_FILE:
      return;
    default:
      RPS_FATAL ("unexpected value to scan type#%u @%p", (int) vtyp,
		 (void *) val);
    }
}				/* end rps_dumper_scan_value */



void
rps_dumper_scan_object (RpsDumper_t * du, RpsObject_t * ob)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  if (!ob)
    return;
  RPS_ASSERT (rps_is_valid_object (ob));
  char obid[32];
  memset (obid, 0, sizeof (obid));
  rps_oid_to_cbuf (ob->ob_id, obid);
  bool absent = rps_hash_tbl_ob_add (du->du_visitedht, ob);
  if (ob->ob_space)
    (void) rps_hash_tbl_ob_add (du->du_spaceht, ob->ob_space);
  /* If the object was already visited by the dumper, do nothing;
     otherwise postpone the scan of object internal data (class,
     attributes and their values, components, payload...) */
  if (absent)
    {
      rps_payldeque_push_last (du->du_deque, ob);
    }
  if (absent)
    RPS_DEBUG_PRINTF (DUMP, "scan new object %s", obid);
  else
    RPS_DEBUG_PRINTF (DUMP, "scan known object %s", obid);
}				/* end rps_dumper_scan_object */

void
rps_dump_one_space (RpsDumper_t * du, int spix, const RpsObject_t * spacob,
		    const RpsSetOb_t * universet)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (spix >= 0 && spix < RPS_DUMP_MAX_NB_SPACE);
  RPS_ASSERT (rps_is_valid_object ((RpsObject_t *) spacob));
  char spacid[32];
  memset (spacid, 0, sizeof (spacid));
  rps_oid_to_cbuf (spacob->ob_id, spacid);
  RPS_DEBUG_NLPRINTF (DUMP, "start dump-one-space spix#%d %s", spix, spacid);
  char filnambuf[128];
  memset (filnambuf, 0, sizeof (filnambuf));
  snprintf (filnambuf, sizeof (filnambuf), "persistore/sp%s-rps.json",
	    spacid);
  char tempathbuf[256];
  memset (tempathbuf, 0, sizeof (tempathbuf));
  snprintf (tempathbuf, sizeof (tempathbuf),
	    "%s/%s-p%d~", rps_stringv_utf8bytes ((RpsValue_t) du->du_dirnam),
	    filnambuf, (int) getpid ());
  int cardspace = 0;
  int carduniv = rps_set_cardinal (universet);
  du->du_htcurspace = rps_hash_tbl_ob_create (carduniv / 2 + 10);
  for (int oix = 0; oix < carduniv; oix++)
    {
      const RpsObject_t *curob = rps_set_nth_member (universet, oix);
      RPS_ASSERT (rps_is_valid_object ((RpsObject_t *) curob));
      char curid[32];
      memset (curid, 0, sizeof (curid));
      rps_oid_to_cbuf (curob->ob_id, curid);
      bool goodob = false;
      pthread_mutex_lock (&((RpsObject_t *) curob)->ob_mtx);
      goodob = curob->ob_space == spacob;
      pthread_mutex_unlock (&((RpsObject_t *) curob)->ob_mtx);
      if (goodob)
	{
	  rps_hash_tbl_ob_add (du->du_htcurspace, (RpsObject_t *) curob);
	  RPS_DEBUG_PRINTF (DUMP,
			    " dump-one-space spix#%d oix#%d goodob id %s",
			    spix, oix, curid);
	}
      else
	RPS_DEBUG_PRINTF (DUMP,
			  " dump-one-space spix#%d oix#%d otherob id %s",
			  spix, oix, curid);
    };
  const RpsSetOb_t *curspaceset =
    rps_hash_tbl_set_elements (du->du_htcurspace);
  unsigned spacesize = rps_set_cardinal (curspaceset);
  FILE *spfil = du->du_spacedescr[spix].sp_file = fopen (tempathbuf, "w");
  if (!spfil)
    RPS_FATAL ("failed to open %s - %m", tempathbuf);
  fprintf (spfil, "/// GENERATED file %s / DO NOT EDIT\n", filnambuf);
  rps_emit_gplv3plus_notice (spfil, filnambuf, "///", "");
  fprintf (spfil, "\n\n");
  fflush (spfil);
  fprintf (spfil, "///!!! prologue of RefPerSys space file:\n");
  fprintf (spfil, "{\n");
  fprintf (spfil, " \"format\" : \"%s\",\n", RPS_MANIFEST_FORMAT);
  fprintf (spfil, " \"nbobjects\" : %u,\n", spacesize);
  fprintf (spfil, " \"spaceid\" : \"%s\",\n", spacid);
  fprintf (spfil, "}\n");
  fflush (spfil);
  for (int oix = 0; oix < (int) spacesize; oix++)
    {
      const RpsObject_t *curob = rps_set_nth_member (curspaceset, oix);
      RPS_ASSERT (rps_is_valid_object ((RpsObject_t *) curob));
      rps_dump_object_in_space (du, spix, spfil, curob, oix);
      fflush (spfil);
    }
  du->du_htcurspace = NULL;
  RPS_DEBUG_PRINTF (DUMP, "end dump-one-space spix#%d %s\n", spix, spacid);
}				/* end rps_dump_one_space */

json_t *
rps_dump_json_for_object (RpsDumper_t * du, const RpsObject_t * ob)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  if (!ob)
    return json_null ();
  RPS_ASSERT (rps_is_valid_object ((RpsObject_t *) ob));
  char obidbuf[32];
  memset (obidbuf, 0, sizeof (obidbuf));
  rps_oid_to_cbuf (ob->ob_id, obidbuf);
  return json_string (obidbuf);
}				/* end rps_dump_json_for_object */


bool
rps_is_dumpable_value (RpsDumper_t * du, RpsValue_t val)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  enum RpsType vtyp = rps_value_type (val);
  switch (vtyp)
    {
    case RPS_TYPE_INT:
      return true;
    case RPS_TYPE_STRING:
      return true;
    case RPS_TYPE_DOUBLE:
      return true;
    case RPS_TYPE_JSON:
      return true;
    case RPS_TYPE_GTKWIDGET:
      return false;
    case RPS_TYPE_TUPLE:
      return true;
    case RPS_TYPE_SET:
      return true;
    case RPS_TYPE_CLOSURE:
      {
	const RpsClosure_t *closv = (RpsClosure_t *) val;
	return rps_is_dumpable_object (du, closv->clos_conn);
      };
    case RPS_TYPE_OBJECT:
      return rps_is_dumpable_object (du, (RpsObject_t *) val);
    case RPS_TYPE_FILE:
      return false;
    default:
      RPS_FATAL ("corrupted value type#%d for %p", (int) vtyp, (void *) val);
    }
}				/* end rps_is_dumpable_value */

bool
rps_is_dumpable_object (RpsDumper_t * du, RpsObject_t * ob)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (rps_is_valid_object (ob));
  if (!ob->ob_space)
    return false;
#warning rps_is_dumpable_object could test more features.... perhaps lack of some "undumpable" attribute?
  return true;
}				/* end of rps_is_dumpable_object */

/// This function should be compatible with conventions followed by
/// rps_loader_json_to_value function in file loader_rps.c
json_t *
rps_dump_json_for_value (RpsDumper_t * du, RpsValue_t val, unsigned depth)
{
  json_t *jres = NULL;
  RPS_ASSERT (rps_is_valid_dumper (du));
  if (depth > RPS_MAX_VALUE_DEPTH)
    RPS_FATAL ("too deep %u value to dump @%p", depth, (void *) val);
  enum RpsType vtyp = rps_value_type (val);
  switch (vtyp)
    {
    case RPS_TYPE_INT:
      jres = json_integer (rps_value_to_integer (val));
      break;
    case RPS_TYPE_DOUBLE:
      jres = json_real (rps_double_value (val));
      break;
    case RPS_TYPE_STRING:
      {
	const char *by = rps_stringv_utf8bytes (val);
	if (by && by[0] != '_')
	  jres = json_string (by);
	else
	  {
	    jres = json_object ();
	    json_object_set (jres, "vtype", json_string ("string"));
	    json_object_set (jres, "string", json_string (by));
	  }
      }
      break;
    case RPS_TYPE_JSON:
      jres = json_object ();
      json_object_set (jres, "vtype", json_string ("json"));
      json_object_set (jres, "json", (json_t *) rps_json_value (val));
      break;
    case RPS_TYPE_GTKWIDGET:
      jres = json_null ();
      break;
    case RPS_TYPE_TUPLE:
      jres = json_object ();
      json_object_set (jres, "vtype", json_string ("tuple"));
      {
	json_t *jsarr = json_array ();
	json_object_set (jres, "tuple", jsarr);
	unsigned tusiz = rps_vtuple_size ((RpsTupleOb_t *) val);
	for (int ix = 0; ix < (int) tusiz; ix++)
	  {
	    RpsObject_t *curob = rps_vtuple_nth ((RpsTupleOb_t *) val, ix);
	    json_t *jscomp =	//
	      rps_dump_json_for_object (du, curob);
	    json_array_append_new (jsarr, jscomp);
	  }
      }
      break;
    case RPS_TYPE_SET:
      jres = json_object ();
      json_object_set (jres, "vtype", json_string ("set"));
      {
	json_t *jsarr = json_array ();
	json_object_set (jres, "set", jsarr);
	unsigned card = rps_set_cardinal ((const RpsSetOb_t *) val);
	for (int ix = 0; ix < (int) card; ix++)
	  {
	    RpsObject_t *elemob =	//
	      rps_set_nth_member ((const RpsSetOb_t *) val, ix);
	    json_t *jscomp =	//
	      rps_dump_json_for_object (du, elemob);
	    json_array_append_new (jsarr, jscomp);
	  }
      }
      break;
    case RPS_TYPE_CLOSURE:
      jres = json_object ();
      json_object_set (jres, "vtype", json_string ("closure"));
      {
	unsigned clsiz = rps_closure_size (val);
	json_t *jsclarr = json_array ();
	RpsObject_t *connob = rps_closure_connective (val);
	json_t *jsconn =	//
	  rps_dump_json_for_object (du, connob);
	json_object_set (jres, "env", jsclarr);
	json_object_set (jres, "fn", jsconn);
	for (int ix = 0; ix < (int) clsiz; ix++)
	  {
	    RpsValue_t curclov =	//
	      rps_closure_get_closed_value (val, ix);
	    json_t *jsclval =	//
	      rps_dump_json_for_value (du, curclov, depth + 1);
	    json_array_append_new (jsclarr, jsclval);
	  };
	RpsValue_t clmeta = rps_closure_meta (val);
	if (clmeta != RPS_NULL_VALUE)
	  {
	    json_t *jsmeta = rps_dump_json_for_value (du, clmeta, depth + 1);
	    json_object_set (jres, "meta", jsmeta);
	  }
      }
      break;
    case RPS_TYPE_OBJECT:
      {
	json_t *jsob =		//
	  rps_dump_json_for_object (du, (RpsObject_t *) val);
	jres = json_object ();
	json_object_set (jres, "vtype", json_string ("object"));
	json_object_set (jres, "object", jres);
      }
      break;
    case RPS_TYPE_FILE:
      jres = json_null ();
      break;
    default:
      RPS_FATAL ("unexpected value to dump type#%u @%p", (int) vtyp,
		 (void *) val);
    }
  return jres;
}				/* end of rps_dump_json_for_value */



struct rpsdumpobject_values_st
{
  RpsClosure_t *dumpclos;
  RpsValue_t resapply;
};

struct rpsdumpobject_objects_st
{
  RpsObject_t *dumpobj;
  RpsObject_t *classobj;
  RpsObject_t *symbobj;
};

const struct rps_callframedescr_st rpscfd_dumpobject_in_space =	//
{.calfrd_magic = RPS_CALLFRD_MAGIC,
  .calfrd_nbvalue =
    sizeof (struct rpsdumpobject_values_st) / sizeof (RpsValue_t),
  .calfrd_nbobject =
    sizeof (struct rpsdumpobject_objects_st) / sizeof (RpsObject_t *)
};

void
rps_dump_object_in_space (RpsDumper_t * du, int spix, FILE * spfil,
			  const RpsObject_t * obj, int oix)
{
  /* we need some valid callframe */
  struct dumpobject_call_frame_st
  {
    RPSFIELDS_PAYLOAD_PROTOCALLFRAME;
    struct rpsdumpobject_values_st v;
    struct rpsdumpobject_objects_st o;
  } _;
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (spix >= 0 && spix < RPS_DUMP_MAX_NB_SPACE);
  RPS_ASSERT (spfil != NULL);
  RPS_ASSERT (rps_is_valid_object ((RpsObject_t *) obj));
  _.o.dumpobj = obj;
  char obidbuf[32];
  memset (obidbuf, 0, sizeof (obidbuf));
  rps_oid_to_cbuf (obj->ob_id, obidbuf);
  pthread_mutex_lock (&(((RpsObject_t *) obj)->ob_mtx));
  fprintf (spfil, "\n\n(//+ob%s\n", obidbuf);
  const RpsObject_t *obclas = obj->ob_class;
  _.o.classobj = obclas;
  RpsClassInfo_t *paylcla = NULL;
  RpsSymbol_t *paylsycla = NULL;
  char obclaidbuf[32];
  memset (obclaidbuf, 0, sizeof (obclaidbuf));
  rps_oid_to_cbuf (obclas->ob_id, obclaidbuf);
  if (obclas)
    paylcla =
      rps_get_object_payload_of_type ((RpsObject_t *) obclas,
				      -RpsPyt_ClassInfo);
  if (paylcla)
    {
      /***- 
	  char obclaidbuf[32];
	  memset (obclaidbuf, 0, sizeof (obclaidbuf));
	  rps_oid_to_cbuf(obclas->ob_id, obclaidbuf);
	  printf ("dump#%d %s obcla %s paylcla@%p [%s:%d]\n",
	  oix, obidbuf, obclaidbuf, paylcla, __FILE__, __LINE__);
	  -***/
      if (paylcla->pclass_symbol)
	{
	  _.o.symbobj = paylcla->pclass_symbol;
	  /***-
	      char obsymidbuf[32];
	      memset (obsymidbuf, 0, sizeof (obsymidbuf));
	      rps_oid_to_cbuf(paylcla->pclass_symbol->ob_id, obsymidbuf);
	      printf ("dump#%d %s obcla %s pclass_symbol %s  [%s:%d]\n",
	      oix, obidbuf, obclaidbuf, obsymidbuf,
	      __FILE__, __LINE__);
	      -***/
	  paylsycla =
	    rps_get_object_payload_of_type (paylcla->pclass_symbol,
					    -RpsPyt_Symbol);
	  if (paylsycla && paylsycla->symb_name)
	    fprintf (spfil, "//∈%s\n",
		     rps_stringv_utf8bytes ((RpsValue_t)
					    (paylsycla->symb_name)));
	}
    }
  json_t *jsob = json_object ();
  json_object_set_new (jsob, "oid", json_string (obidbuf));
  json_object_set_new (jsob, "mtime", json_real (obj->ob_mtime));
  json_object_set_new (jsob, "class", json_string (obclaidbuf));
#warning rps_dump_object_in_space should fill jsob, annd dump it piece by piece
  /*** TODO: Use rps_value_compute_method_closure to get a closure
       dumping the object, otherwise do a "physical" dump.
  ***/
  const RpsClosure_t *dumpclos = _.v.dumpclos	//
    = rps_value_compute_method_closure ((RpsValue_t) obj,	//
					RPS_ROOT_OB (_6FSANbZbPmZNb2JeVi)	//dump_object
    );
  if (dumpclos)
    {
      RPS_DEBUG_PRINTF (DUMP, "dumped object %s before applying dump closure",
			obidbuf);
      _.v.resapply =
	rps_closure_apply_dumpj ((rps_callframe_t *) & _, dumpclos, du,
				 (RpsValue_t) obj, jsob);
      RPS_DEBUG_PRINTF (DUMP, "dumped object %s after applying dump closure",
			obidbuf);
    }
  else
    {
      RPS_DEBUG_PRINTF (DUMP, "dumped object %s without dump closure",
			obidbuf);
#warning should dump using rpscloj_dump_object_components & rpscloj_dump_object_attributes
    }
  fprintf (spfil, "{\n");
  const char *curkey = NULL;
  json_t *jsva = NULL;
  int nbat = json_object_size (jsob);
  int cnt = 0;
  json_object_foreach (jsob, curkey, jsva)
  {
    RPS_ASSERT (jsva != NULL);
    // We take care when duming the "mtime" field to emit only it with
    // two decimal digits. Since clock is in practice inaccurate.
    if (!strcmp (curkey, "mtime") && json_is_real (jsva))
      {
	fprintf (spfil, " \"mtime\" : %.2f", json_real_value (jsva));
      }
    else
      {
	fprintf (spfil, " \"%s\" : ", curkey);
	json_dumpf (jsva, spfil,
		    JSON_INDENT (2) | JSON_SORT_KEYS |
		    JSON_REAL_PRECISION (12) | JSON_ENCODE_ANY);
      }
    if (cnt < nbat - 1)
      fputs (",\n", spfil);
    else
      fputc ('\n', spfil);
    cnt++;
  }
  fprintf (spfil, "}\n//-ob%s\n", obidbuf);
  fflush (spfil);
  json_decrefp (&jsob);
  pthread_mutex_unlock (&((RpsObject_t *) obj)->ob_mtx);
}				/* end rps_dump_object_in_space */


#warning a lot of dumping routines are missing here
void
rps_dump_heap (rps_callframe_t * frame, const char *dirn)
{
  if (!dirn)
    dirn = rps_dump_directory;
  RPS_DEBUG_NLPRINTF (DUMP, "| start dumping to %s", dirn);
  if (rps_agenda_is_running ())
    RPS_FATAL ("cannot dump heap into %s while agenda is running", dirn);
  RpsDumper_t *dumper = NULL;
  {
    if (g_mkdir_with_parents (dirn, 0750) < 0)
      RPS_FATAL ("g_mkdir_with_parents failed for %s", dirn);
    /* TODO: Do we need some temporary dumper object, owning the dumper
       payload below? */
    dumper = RPS_ALLOC_ZONE (sizeof (RpsDumper_t), -RpsPyt_Dumper);
    rps_the_dumper = dumper;
    dumper->du_magic = RPS_DUMPER_MAGIC;
    dumper->du_callframe = frame;
    dumper->du_start_realtime = rps_real_time ();
    dumper->du_start_cputime = rps_process_cpu_time ();
    char *realdirn = realpath (dirn, NULL);
    if (!realdirn)
      RPS_FATAL ("realpath failed for %s", dirn);
    char tempathbuf[256];
    memset (tempathbuf, 0, sizeof (tempathbuf));
    snprintf (tempathbuf, sizeof (tempathbuf), "%s/persistore", realdirn);
    g_mkdir_with_parents (tempathbuf, 0750);
    dumper->du_dirnam = rps_alloc_string (realdirn);
    printf ("\n**Start dumping into %s git %s [%s:%d]\n",
	    realdirn, _rps_git_short_id, __FILE__, __LINE__);
    fflush (NULL);
    free (realdirn);
  }
  dumper->du_visitedht =	//
    rps_hash_tbl_ob_create (16 + 3 * rps_nb_global_root_objects ());
  dumper->du_spaceht =		//
    rps_hash_tbl_ob_create (3 + rps_nb_global_root_objects () / 5);
  dumper->du_deque =		//
    rps_deque_for_dumper (dumper);
#warning temporary call to mallopt. Should be removed once loading and dumping completes.
  mallopt (M_CHECK_ACTION, 03);

  /* scan the global objects */
  rps_dumper_scan_value (dumper,
			 (RpsValue_t) (rps_set_of_global_root_objects ()), 0);
  RpsObject_t *curob = NULL;
  long scancnt = 0;
  RPS_ASSERT (dumper->du_spaceht
	      && rps_hash_tbl_is_valid (dumper->du_spaceht));
  RPS_ASSERT (dumper->du_visitedht
	      && rps_hash_tbl_is_valid (dumper->du_visitedht));
  /* loop to scan visited, but unscanned objects */
  while ((curob = rps_payldeque_pop_first (dumper->du_deque)) != NULL)
    {
      char oidbuf[32];
      memset (oidbuf, 0, sizeof (oidbuf));
      scancnt++;
      rps_oid_to_cbuf (curob->ob_id, oidbuf);
      RPS_DEBUG_PRINTF (DUMP, "dump scan internal#%ld oid %s %s remaining %d",
			scancnt, oidbuf,
			curob->ob_space ? "!" : "°",
			rps_payldeque_length (dumper->du_deque));
      rps_dumper_scan_internal_object (dumper, curob);
      if (scancnt % 16 == 0)
	{
	  RPS_ASSERT (rps_hash_tbl_ob_cardinal (dumper->du_spaceht) > 0);
	  RPS_ASSERT (rps_hash_tbl_ob_cardinal (dumper->du_visitedht) > 0);
	}
    };				/* end while du_deque not empty */
  const RpsSetOb_t *universet =
    rps_hash_tbl_set_elements (dumper->du_visitedht);
  const RpsSetOb_t *spaceset =	//
    rps_hash_tbl_set_elements (dumper->du_spaceht);
  unsigned nbspace = rps_set_cardinal (spaceset);
  unsigned nbobj = rps_set_cardinal (universet);
  RPS_DEBUG_NLPRINTF (DUMP, "dump_heap nbspace=%u nbobj=%u\n", nbspace,
		      nbobj);
  RPS_ASSERTPRINTF (nbspace > 0 && nbobj > 0,
		    "dump_heap invalid nbspace=%u nbobj=%u", nbspace, nbobj);
  /* Temporarily we cannot deal with many spaces.... Should be fixed
     by generating C code later... */
  if (nbspace >= RPS_DUMP_MAX_NB_SPACE)
    RPS_FATAL ("too many %d spaces to dump into %s", nbspace,
	       rps_stringv_utf8bytes ((RpsValue_t) dumper->du_dirnam));
  for (int spix = 0; spix < nbspace; spix++)
    rps_dump_one_space (dumper, spix, rps_set_nth_member (spaceset, spix),
			universet);
  dumper->du_callframe = NULL;
  printf
    ("\n** RefPerSys %s dumped into %s directory %d spaces and %d objects in %.4f real %.4f cpu seconds\n"
     " ... (%.2f real, %.2f cpu µs/obj) [%s:%d]\n",
     _rps_git_short_id,
     rps_stringv_utf8bytes ((RpsValue_t) dumper->du_dirnam), nbspace, nbobj,
     rps_real_time () - dumper->du_start_realtime,
     rps_process_cpu_time () - dumper->du_start_cputime,
     1.0e6 * (rps_real_time () - dumper->du_start_realtime) / nbobj,
     1.0e6 * (rps_process_cpu_time () - dumper->du_start_cputime) / nbobj,
     __FILE__, __LINE__);
  fflush (NULL);
  printf(".... universe set %V\n", (RpsValue_t)universet);
  fflush (NULL);
  printf(".... space set %V\n", (RpsValue_t)spaceset);
  fflush (NULL);
  rps_the_dumper = NULL;
}				/* end rps_dump_heap */


/*********** end of file dumper_rps.c **********/
