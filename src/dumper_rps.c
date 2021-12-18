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
 *      © Copyright 2019 - 2021 The Reflective Persistent System Team
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
  pthread_mutex_lock (&ob->ob_mtx);
  rps_dumper_scan_object (du, ob->ob_class);
  if (ob->ob_space)
    rps_dumper_scan_object (du, ob->ob_space);
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
      RPS_FATAL ("unexpected value type#%u @%p", (int) vtyp, (void *) val);
    }
}				/* end rps_dumper_scan_value */



void
rps_dumper_scan_object (RpsDumper_t * du, RpsObject_t * ob)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  if (!ob)
    return;
  RPS_ASSERT (rps_is_valid_object (ob));
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
}				/* end rps_dumper_scan_object */

void
rps_dump_one_space (RpsDumper_t * du, int spix, const RpsObject_t * spacob,
		    const RpsSetOb_t * universet)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (spix >= 0 && spix < RPS_DUMP_MAX_NB_SPACE);
  RPS_ASSERT (rps_is_valid_object (spacob));
  char spacid[32];
  memset (spacid, 0, sizeof (spacid));
  rps_oid_to_cbuf (spacob->ob_id, spacid);
  char filnambuf[128];
  memset (filnambuf, 0, sizeof (filnambuf));
  snprintf (filnambuf, sizeof (filnambuf), "persistore/sp%s-rps.json",
	    spacid);
  char tempathbuf[256];
  memset (tempathbuf, 0, sizeof (tempathbuf));
  snprintf (tempathbuf, sizeof (tempathbuf),
	    "%s/%s-p%d~", du->du_dirnam, filnambuf, (int) getpid ());
  int cardspace = 0;
  int carduniv = rps_set_cardinal (universet);
  du->du_htcurspace = rps_hash_tbl_ob_create (carduniv / 2 + 10);
  for (int oix = 0; oix < carduniv; oix++)
    {
      const RpsObject_t *curob = rps_set_nth_member (universet, oix);
      RPS_ASSERT (rps_is_valid_object (curob));
      bool goodob = false;
      pthread_mutex_lock (&((RpsObject_t *) curob)->ob_mtx);
      goodob = curob->ob_space == spacob;
      pthread_mutex_unlock (&((RpsObject_t *) curob)->ob_mtx);
      if (goodob)
	rps_hash_tbl_ob_add (du->du_htcurspace, curob);
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
      RPS_ASSERT (rps_is_valid_object (curob));
    }
  RPS_FATAL ("incomplete rps_dump_one_space %s", tempathbuf);
#warning incomplete rps_dump_one_space
  du->du_htcurspace = NULL;
}				/* end rps_dump_one_space */


#warning a lot of dumping routines are missing here
void
rps_dump_heap (rps_callframe_t * frame, const char *dirn)
{
  if (!dirn)
    dirn = rps_dump_directory;
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
    char *realdirn = realpath (dirn, NULL);
    if (!realdirn)
      RPS_FATAL ("realpath failed for %s", dirn);
    dumper->du_dirnam = rps_alloc_string (realdirn);
    printf ("\n**Start dumping into %s git %s [%s:%d]\n",
	    realdirn, _rps_git_short_id, __FILE__, __LINE__);
    fflush (NULL);
    free (realdirn);
  }
  dumper->du_visitedht =	//
    rps_hash_tbl_ob_create (16 + 3 * rps_nb_global_root_objects ());
  dumper->du_deque =		//
    rps_deque_for_dumper (dumper);
#warning temporary call to mallopt. Should be removed once loading and dumping completes.
  mallopt (M_CHECK_ACTION, 03);

  /* scan the global objects */
  rps_dumper_scan_value (dumper,
			 (RpsValue_t) (rps_set_of_global_root_objects ()), 0);
  RpsObject_t *curob = NULL;
  long scancnt = 0;
  /* loop to scan visited, but unscanned objects */
  while ((curob = rps_payldeque_pop_first (dumper->du_deque)) != NULL)
    {
      char oidbuf[32];
      memset (oidbuf, 0, sizeof (oidbuf));
      scancnt++;
      rps_oid_to_cbuf (curob->ob_id, oidbuf);
      printf ("dump scan internal#%ld oid %s, remaining %d [%s:%d]\n",
	      scancnt, oidbuf, rps_payldeque_length (dumper->du_deque),
	      __FILE__, __LINE__);
      rps_dumper_scan_internal_object (dumper, curob);
    };
  const RpsSetOb_t *universet =
    rps_hash_tbl_set_elements (dumper->du_visitedht);
  const RpsSetOb_t *spaceset =	//
    rps_hash_tbl_set_elements (dumper->du_spaceht);
  unsigned nbspace = rps_set_cardinal (spaceset);
  unsigned nbobj = rps_set_cardinal (universet);
  /* Temporarily we cannot deal with many spaces.... Should be fixed
     by generating C code later... */
  if (nbspace >= RPS_DUMP_MAX_NB_SPACE)
    RPS_FATAL ("too many %d spaces to dump into %s", nbspace,
	       rps_stringv_utf8bytes ((RpsValue_t) dumper->du_dirnam));
  for (int spix = 0; spix < nbspace; spix++)
    rps_dump_one_space (dumper, spix, rps_set_nth_member (spaceset, spix),
			universet);
  dumper->du_callframe = NULL;
  rps_the_dumper = NULL;
}				/* end rps_dump_heap */
