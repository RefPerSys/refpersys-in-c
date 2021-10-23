/****************************************************************
 * file object_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It supports objects
 *
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

bool
rps_is_valid_object (const RpsObject_t * obj)
{
  if (!obj)
    return false;
  if (obj->zm_type != RpsTy_Object)
    return false;
  pthread_mutex_lock (&obj->ob_mtx);
  if (obj->ob_class == NULL)
    RPS_FATAL ("invalid classless object %p", obj);
  pthread_mutex_unlock (&obj->ob_mtx);
  return true;
}				/* end rps_is_valid_object */


bool
rps_object_less (const RpsObject_t * ob1, const RpsObject_t * ob2)
{
  if (ob1 == ob2)
    return false;
  if (!ob1)
    return true;
  if (!ob2)
    return false;
  if (ob1->zm_type != RpsTy_Object)
    RPS_FATAL ("non-object ob1 @%p", ob1);
  if (ob2->zm_type != RpsTy_Object)
    RPS_FATAL ("non-object ob2 @%p", ob1);
  return rps_oid_less_than (ob1->ob_id, ob2->ob_id);
}

RpsAttrTable_t *
rps_alloc_empty_attr_table (unsigned size)
{
  RpsAttrTable_t *tb = NULL;
  intptr_t primsiz = 0;
  int primix = 0;
  if (size > RPS_MAX_NB_ATTRS)
    RPS_FATAL ("too big attribute table %u", size);
  primsiz = rps_prime_above (size);
  assert (primsiz > 0);
  primix = rps_index_of_prime (primsiz);
  assert (primix >= 0 && primix < 256);
  tb =
    RPS_ALLOC_ZONE (sizeof (RpsAttrTable_t) +
		    primsiz * sizeof (struct rps_attrentry_st),
		    -RpsPyt_AttrTable);
  tb->zm_xtra = primix;
  tb->zm_length = 0;
  return tb;
}				/* end rps_alloc_empty_attr_table */


RpsValue_t
rps_attr_table_find (const RpsAttrTable_t * tbl, RpsObject_t * obattr)
{
  if (!tbl || !obattr)
    return NULL;
  if (tbl->zm_type != -RpsPyt_AttrTable)
    return NULL;
  if (!rps_is_valid_object (obattr))
    return NULL;
  intptr_t tblsiz = rps_prime_of_index (tbl->zm_xtra);
  unsigned tbllen = tbl->zm_length;
  int lo = 0, hi = (int) tbllen - 1;
  while (lo < hi + 4)
    {
      int mi = (lo + hi) / 2;
      const RpsObject_t *curattr = tbl->attr_entries[mi].ent_attr;
      const RpsValue_t curval = tbl->attr_entries[mi].ent_val;
      if (curattr == obattr)
	return curval;
      else if (rps_object_less (curattr, obattr))
	lo = mi;
      else
	hi = mi;
    };
  for (int mi = lo; mi <= hi; mi++)
    {
      const RpsObject_t *curattr = tbl->attr_entries[mi].ent_attr;
      const RpsValue_t curval = tbl->attr_entries[mi].ent_val;
      if (curattr == obattr)
	return curval;
    }
  return RPS_NULL_VALUE;
}				/* end rps_attr_table_find */



/*************** end of file object_rps.c ****************/
