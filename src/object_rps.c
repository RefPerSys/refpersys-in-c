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
rps_is_valid_object (RpsObject_t * obj)
{
  if (!obj)
    return false;
  if (obj->zm_type != RPS_TYPE_OBJECT)
    return false;
  pthread_mutex_lock (&obj->ob_mtx);
  if (obj->ob_class == NULL)
    RPS_FATAL ("invalid classless object %p", obj);
  pthread_mutex_unlock (&obj->ob_mtx);
  return true;
}				/* end rps_is_valid_object */


bool
rps_object_less (RpsObject_t * ob1, RpsObject_t * ob2)
{
  if (ob1 == ob2)
    return false;
  if (!ob1)
    return true;
  if (!ob2)
    return false;
  if (ob1->zm_type != RPS_TYPE_OBJECT)
    RPS_FATAL ("non-object ob1 @%p", ob1);
  if (ob2->zm_type != RPS_TYPE_OBJECT)
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
  RPS_ASSERT (primsiz > 0);
  primix = rps_index_of_prime (primsiz);
  RPS_ASSERT (primix >= 0 && primix < 256);
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
    return RPS_NULL_VALUE;
  if (tbl->zm_type != -RpsPyt_AttrTable)
    return RPS_NULL_VALUE;
  if (!rps_is_valid_object (obattr))
    return RPS_NULL_VALUE;
  intptr_t tblsiz = rps_prime_of_index (tbl->zm_xtra);
  unsigned tbllen = tbl->zm_length;
  int lo = 0, hi = (int) tbllen - 1;
  while (lo < hi + 4)
    {
      int mi = (lo + hi) / 2;
      RpsObject_t *curattr = tbl->attr_entries[mi].ent_attr;
      RpsValue_t curval = tbl->attr_entries[mi].ent_val;
      if (curattr == obattr)
	return curval;
      else if (rps_object_less (curattr, obattr))
	lo = mi;
      else
	hi = mi;
    };
  for (int mi = lo; mi <= hi; mi++)
    {
      RpsObject_t *curattr = tbl->attr_entries[mi].ent_attr;
      RpsValue_t curval = tbl->attr_entries[mi].ent_val;
      if (curattr == obattr)
	return curval;
    }
  return RPS_NULL_VALUE;
}				/* end rps_attr_table_find */


/* internal routine to put or insert an entry ... Return true if successful */
static bool
rps_attr_table_entry_put (RpsAttrTable_t * tbl, RpsObject_t * obattr,
			  RpsValue_t obval)
{
  RPS_ASSERT (tbl != NULL);
  intptr_t tblsiz = rps_prime_of_index (tbl->zm_xtra);
  unsigned tbllen = tbl->zm_length;
  int lo = 0, hi = (int) tbllen - 1;
  while (lo < hi + 4)
    {
      int mi = (lo + hi) / 2;
      RpsObject_t *curattr = tbl->attr_entries[mi].ent_attr;
      if (curattr == obattr)
	{
	  tbl->attr_entries[mi].ent_val = obval;
	  return true;
	}
      else if (rps_object_less (curattr, obattr))
	lo = mi;
      else
	hi = mi;
    };
  RPS_ASSERT (tbllen < tblsiz);
  for (int ix = lo; ix <= hi; ix++)
    {
      RpsObject_t *curattr = tbl->attr_entries[ix].ent_attr;
      if (curattr == obattr)
	{
	  tbl->attr_entries[ix].ent_val = obval;
	  return true;
	}
      else if (rps_object_less (curattr, obattr))
	{
	  memmove (tbl->attr_entries + ix, tbl->attr_entries + ix + 1,
		   (tbllen - ix) * sizeof (struct rps_attrentry_st));
	  tbl->attr_entries[ix].ent_attr = obattr;
	  tbl->attr_entries[ix].ent_val = obval;
	  tbl->zm_length = tbllen + 1;
	  return true;
	}
    }
  return false;
}				/* end rps_attr_table_entry_put */

RpsAttrTable_t *
rps_attr_table_put (RpsAttrTable_t * tbl, RpsObject_t * obattr,
		    RpsValue_t val)
{
  if (!obattr || !rps_is_valid_object (obattr))
    return tbl;
  if (!val)
    return tbl;
  if (!tbl)
    tbl = rps_alloc_empty_attr_table (2);
  if (rps_attr_table_entry_put (tbl, obattr, val))
    return tbl;
  RpsAttrTable_t *old_tbl = tbl;
  intptr_t oldtblsiz = rps_prime_of_index (old_tbl->zm_xtra);
  unsigned oldtbllen = old_tbl->zm_length;
  RpsAttrTable_t *new_tbl =
    rps_alloc_empty_attr_table (oldtbllen + 2 + oldtblsiz / 5);
  memcpy (new_tbl->attr_entries, old_tbl->attr_entries,
	  oldtbllen * sizeof (struct rps_attrentry_st));
  new_tbl->zm_length = oldtbllen;
  // the test below should always succeed!
  if (!rps_attr_table_entry_put (new_tbl, obattr, val))
    RPS_FATAL ("corruption in rps_attr_table_put for new_tbl @%p", new_tbl);
  free (old_tbl);
  return new_tbl;
}				/* end rps_attr_table_put */


RpsAttrTable_t *
rps_attr_table_remove (RpsAttrTable_t * tbl, RpsObject_t * obattr)
{
  if (!obattr || !rps_is_valid_object (obattr))
    return tbl;
  if (!tbl)
    return NULL;
  RpsAttrTable_t *old_tbl = tbl;
  int oldprimix = old_tbl->zm_xtra;
  intptr_t oldtblsiz = rps_prime_of_index (oldprimix);
  unsigned oldtbllen = old_tbl->zm_length;
  int pos = -1;
  int lo = 0, hi = (int) oldtbllen - 1;
  while (lo < hi + 4)
    {
      int mi = (lo + hi) / 2;
      RpsObject_t *curattr = old_tbl->attr_entries[mi].ent_attr;
      if (curattr == obattr)
	{
	  pos = mi;
	  break;
	}
      else if (rps_object_less (curattr, obattr))
	lo = mi;
      else
	hi = mi;
    };
  if (pos < 0)
    for (int ix = lo; ix <= hi; ix++)
      {
	RpsObject_t *curattr = old_tbl->attr_entries[ix].ent_attr;
	if (curattr == obattr)
	  {
	    pos = ix;
	    break;
	  }
      };
  if (pos < 0)			/* not found */
    return old_tbl;
  if (oldtblsiz > 6 && oldtbllen < oldtblsiz / 2)
    {
      /* perhaps shrink the table */
      int newprimsiz = rps_prime_above (oldtbllen - 1);
      RPS_ASSERT (newprimsiz > 0);
      int newprimix = rps_index_of_prime (newprimsiz);
      RPS_ASSERT (newprimix >= 0 && newprimix < 256);
      if (newprimix < oldprimix)
	{
	  RpsAttrTable_t *new_tbl =
	    RPS_ALLOC_ZONE (sizeof (RpsAttrTable_t) +
			    newprimsiz * sizeof (struct rps_attrentry_st),
			    -RpsPyt_AttrTable);
	  new_tbl->zm_xtra = newprimix;
	  new_tbl->zm_length = 0;
	  for (int ix = 0; ix < pos; ix++)
	    new_tbl->attr_entries[ix] = old_tbl->attr_entries[ix];
	  for (int ix = pos + 1; ix < oldtbllen; ix++)
	    new_tbl->attr_entries[ix] = old_tbl->attr_entries[ix - 1];
	  new_tbl->zm_length = oldtbllen - 1;
	  free (old_tbl);
	  return new_tbl;
	}
    };
  /* don't shrink, but remove entry at pos and shift entries above it */
  for (int ix = pos + 1; ix < oldtbllen; ix++)
    old_tbl->attr_entries[ix] = old_tbl->attr_entries[ix - 1];
  old_tbl->attr_entries[oldtbllen - 1].ent_attr = NULL;
  old_tbl->attr_entries[oldtbllen - 1].ent_val = RPS_NULL_VALUE;
  old_tbl->zm_length = oldtbllen - 1;
  return old_tbl;
}				/* end rps_attr_table_remove */






/*****************************************************************
 * Objects
 *****************************************************************/
struct rps_object_bucket_st
{
  pthread_mutex_t obuck_mtx;
  unsigned obuck_card;		/* number of objects in the bucket */
  unsigned obuck_size;		/* allocated size of obuck_arr, some prime */
  RpsObject_t **obuck_arr;
};

struct rps_object_bucket_st rps_object_bucket_array[RPS_OID_MAXBUCKETS];
static pthread_mutexattr_t rps_objmutexattr;
void
rps_initialize_objects_machinery (void)
{
  static bool initialized;
  if (initialized)
    RPS_FATAL ("rps_initialize_objects_machinery called twice");
  if (pthread_mutexattr_init (&rps_objmutexattr))
    RPS_FATAL ("failed to init rps_objmutexattr");
  if (pthread_mutexattr_settype (&rps_objmutexattr, PTHREAD_MUTEX_RECURSIVE))
    RPS_FATAL ("failed to settype rps_objmutexattr");
  for (int bix = 0; bix < RPS_OID_MAXBUCKETS; bix++)
    {
      pthread_mutex_init (&rps_object_bucket_array[bix].obuck_mtx,
			  &rps_objmutexattr);
      rps_object_bucket_array[bix].obuck_card = 0;
      rps_object_bucket_array[bix].obuck_size = 0;
      rps_object_bucket_array[bix].obuck_arr = NULL;
    }
  initialized = true;
}				/* end rps_initialize_objects_machinery */

void
rps_initialize_objects_for_loading (RpsLoader_t * ld, unsigned nbglobroot)
{
  RPS_ASSERT (rps_is_valid_loader (ld));
  RPS_ASSERTPRINTF (nbglobroot > 2, "nbglobroot %u", nbglobroot);
  unsigned minbucksize =
    rps_prime_above (3 + nbglobroot / RPS_OID_MAXBUCKETS);
  for (int bix = 0; bix < RPS_OID_MAXBUCKETS; bix++)
    {
      struct rps_object_bucket_st *curbuck = rps_object_bucket_array + bix;
      pthread_mutex_lock (&curbuck->obuck_mtx);
      if (curbuck->obuck_arr == NULL)
	{
	  RPS_ASSERTPRINTF (curbuck->obuck_card == 0,
			    "empty bucket#%d corrupted cardinal %u", bix,
			    curbuck->obuck_card);
	  RPS_ASSERTPRINTF (curbuck->obuck_size == 0,
			    "empty bucket#%d corrupted size %u", bix,
			    curbuck->obuck_size);
	  curbuck->obuck_size = minbucksize;
	  curbuck->obuck_arr =
	    RPS_ALLOC_ZEROED (sizeof (RpsObject_t *) * minbucksize);
	}
      else
	RPS_ASSERTPRINTF (curbuck->obuck_size > 0,
			  "bucket#%d corrupted size %u", bix,
			  curbuck->obuck_size);
      pthread_mutex_unlock (&rps_object_bucket_array[bix].obuck_mtx);
    }
}				/* end rps_initialize_objects_for_loading */


// for qsort of objects
static int
rps_objptrqcmp (const void *p1, const void *p2)
{
  return rps_object_cmp (*(const RpsObject_t **) p1,
			 *(const RpsObject_t **) p2);
}				/* end rps_objptrqcmp */


int
rps_object_cmp (const RpsObject_t * ob1, const RpsObject_t * ob2)
{
  if (ob1 == ob2)
    return 0;
  if (ob1 == NULL)
    return -1;
  if (ob2 == NULL)
    return +1;
  return rps_oid_cmp (ob1->ob_id, ob2->ob_id);
}				/* end rps_object_cmp */


void
rps_object_array_qsort (const RpsObject_t ** arr, int size)
{
  if (arr != NULL && size > 0)
    qsort (arr, (size_t) size, sizeof (RpsObject_t *), rps_objptrqcmp);
}				/* end rps_object_array_qsort */


RpsObject_t *
rps_find_object_by_oid (const RpsOid_t oid)
{
  struct rps_object_bucket_st *curbuck = NULL;
  if (oid.id_hi == 0 || !rps_oid_is_valid (oid))
    return NULL;
  RpsObject_t *obres = NULL;
  unsigned bix = rps_oid_bucket_num (oid);
  pthread_mutex_lock (&rps_object_bucket_array[bix].obuck_mtx);
  curbuck = &rps_object_bucket_array[bix];
  if (curbuck->obuck_arr == NULL)
    goto end;
  unsigned cbucksiz = curbuck->obuck_size;
  RPS_ASSERTPRINTF (cbucksiz > 3, "bad bucket#%u size %u", bix, cbucksiz);
  RPS_ASSERTPRINTF (5 * curbuck->obuck_card < 4 * cbucksiz,
		    "bad bucket#%u size %u for cardinal %u",
		    bix, cbucksiz, curbuck->obuck_card);
  unsigned stix = (oid.id_hi ^ oid.id_lo) % cbucksiz;
  for (int ix = stix; ix < (int) cbucksiz; ix++)
    {
      RpsObject_t *curob = curbuck->obuck_arr[ix];
      if (NULL == curob)
	goto end;
      if (rps_oid_equal (curob->ob_id, oid))
	{
	  obres = curob;
	  goto end;
	}
    };
  for (int ix = 0; ix < (int) stix; ix++)
    {
      RpsObject_t *curob = curbuck->obuck_arr[ix];
      if (NULL == curob)
	goto end;
      if (rps_oid_equal (curob->ob_id, oid))
	{
	  obres = curob;
	  goto end;
	}
    };
end:
  if (curbuck)
    pthread_mutex_unlock (&curbuck->obuck_mtx);
  return obres;
}				/* end rps_find_object_by_oid */



static void
rps_add_object_to_locked_bucket (struct rps_object_bucket_st *buck,
				 RpsObject_t * obj)
{
  RPS_ASSERT (buck != NULL);
  RPS_ASSERT (obj != NULL);
  unsigned cbucksiz = buck->obuck_size;
  if (5 * buck->obuck_card + 2 > 4 * cbucksiz)
    {
      unsigned newsiz = rps_prime_above (4 * buck->obuck_card / 3 + 5);
      RpsObject_t **oldarr = buck->obuck_arr;
      buck->obuck_arr = RPS_ALLOC_ZEROED (sizeof (RpsObject_t *) * newsiz);
      buck->obuck_size = newsiz;
      buck->obuck_card = 0;
      for (int ix = 0; ix < (int) cbucksiz; ix++)
	{
	  RpsObject_t *oldobj = oldarr[ix];
	  if (oldobj)		/* this recursion happens once! */
	    rps_add_object_to_locked_bucket (buck, oldobj);
	};
      free (oldarr);
    };
  RPS_ASSERTPRINTF (cbucksiz > 3, "bad bucket size %u", cbucksiz);
  unsigned stix = (obj->ob_id.id_hi ^ obj->ob_id.id_lo) % cbucksiz;
  for (int ix = stix; ix < (int) cbucksiz; ix++)
    {
      RpsObject_t *curob = buck->obuck_arr[ix];
      if (NULL == curob)
	{
	  buck->obuck_arr[ix] = obj;
	  buck->obuck_card++;
	  return;
	}
      if (curob == obj)
	return;
    };
  for (int ix = 0; ix < (int) stix; ix++)
    {
      RpsObject_t *curob = buck->obuck_arr[ix];
      if (NULL == curob)
	{
	  buck->obuck_arr[ix] = obj;
	  buck->obuck_card++;
	  return;
	}
      if (curob == obj)
	return;
    };
}				/* end rps_add_object_to_locked_bucket */


RpsObject_t *
rps_get_loaded_object_by_oid (RpsLoader_t * ld, const RpsOid_t oid)
{
  struct rps_object_bucket_st *curbuck = NULL;
  RPS_ASSERT (rps_is_valid_loader (ld));
  if (rps_is_valid_creating_loader (ld))
    {
      /* we should allocate a new object, since it should not exist */
      unsigned bix = rps_oid_bucket_num (oid);
      RpsObject_t *obinfant =
	RPS_ALLOC_ZONE (sizeof (RpsObject_t), RPS_TYPE_OBJECT);
      pthread_mutex_init (&obinfant->ob_mtx, &rps_objmutexattr);
      obinfant->ob_id = oid;
      // the infant object has no class yet!
      pthread_mutex_lock (&rps_object_bucket_array[bix].obuck_mtx);
      curbuck = &rps_object_bucket_array[bix];
      if (!curbuck->obuck_arr)
	{
	  unsigned inibucksiz = 7;
	  curbuck->obuck_arr =
	    RPS_ALLOC_ZEROED (sizeof (RpsObject_t *) * inibucksiz);
	  curbuck->obuck_size = inibucksiz;
	};
      rps_add_object_to_locked_bucket (curbuck, obinfant);
      pthread_mutex_unlock (&curbuck->obuck_mtx);
      return obinfant;
    }
  else if (rps_is_valid_filling_loader (ld))
    {
      /* we need to find an existing object */
      return rps_find_object_by_oid (oid);
    }
}				/* end rps_get_loaded_object_by_oid */

/*************** end of file object_rps.c ****************/
