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

#define RPS_DUMPER_MAGIC 0x2501f5e3	/*620885475 */

struct RpsPayl_Dumper_st
{
  RPSFIELDS_ZONED_VALUE;
  unsigned du_magic;		/* always RPS_DUMPER_MAGIC */
  /* The dumper probably needs to contain a big hash table of visited
     objects; a first pass is scanning the heap, starting from global
     roots including the agenda. During the dump only one pthread should
     be running, every other pthread should be blocked. The agenda should
     be "idle".
   */
  const RpsString_t *du_dirnam;
  RpsHashTblOb_t *du_visitedht;
  RpsDequeOb_t *du_deque;
};


bool
rps_is_valid_dumper (RpsDumper_t * du)
{
  if (!du)
    return false;
  if (du->du_magic != RPS_DUMPER_MAGIC)
    return false;
}				/* end rps_is_valid_dumper */

void
rps_dump_heap (const char *dirn)
{
  if (!dirn)
    dirn = rps_dump_directory;
  if (rps_agenda_is_running ())
    RPS_FATAL ("cannot dump heap into %s while agenda is running", dirn);
  /* TODO: Do we need some temporary dumper object, owning the dumper
     payload below? */
  RpsDumper_t *dumper =		//    
    RPS_ALLOC_ZONE (sizeof (RpsDumper_t),
		    -RpsPyt_Dumper);
  dumper->du_magic = RPS_DUMPER_MAGIC;
  dumper->du_dirnam = rps_alloc_string (dirn);
  dumper->du_visitedht =	//
    rps_hash_tbl_ob_create (16 + 3 * rps_nb_global_root_objects ());
  /* scan the global objects */
  rps_dumper_scan_value (dumper,
			 (RpsValue_t) (rps_set_of_global_root_objects ()), 0);
  RpsObject_t *curob = NULL;
  /* loop to scan visited, but unscanned objects */
  while ((curob = rps_payldeque_pop_first (dumper->du_deque)) != NULL)
    {
      rps_dumper_scan_internal_object (dumper, curob);
    };
  /* once every object is known, dump them by space */
  RPS_FATAL ("unimplemented rps_dump_heap to %s", rps_dump_directory);
}				/* end rps_dump_heap */


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
  if (absent)
    {
      rps_payldeque_push_last (du->du_deque, ob);
    }

  /* If the object was already visited by the dumper, do nothing;
     otherwise postpone the scan of object internal data (class,
     attributes and their values, components, payload...) */
  RPS_FATAL ("unimplemented rps_dumper_scan_object");
#warning unimplemented rps_dumper_scan_object
}				/* end rps_dumper_scan_object */

#warning a lot of dumping routines are missing here
