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

bool
rps_is_valid_object (RpsObject_t * obj)
{
  if (!obj)
    return false;
  if (RPS_ZONED_MEMORY_TYPE (obj) != RPS_TYPE_OBJECT)
    return false;
  pthread_mutex_lock (&obj->ob_mtx);
  if (obj->ob_class == NULL)
    {
      RpsOid oid = obj->ob_id;
      char oidbuf[32];
      memset (oidbuf, 0, sizeof (oidbuf));
      rps_oid_to_cbuf (oid, oidbuf);
      RPS_FATAL ("invalid classless object %s @%p", oidbuf, obj);
    }
  pthread_mutex_unlock (&obj->ob_mtx);
  return true;
}				/* end rps_is_valid_object */

RpsValue_t
rps_get_object_attribute (RpsObject_t * obj, RpsObject_t * obattr)
{
  if (!obj)
    return RPS_NULL_VALUE;
  if (!obattr)
    return RPS_NULL_VALUE;
  RPS_ASSERT (rps_is_valid_object (obj));
  RPS_ASSERT (rps_is_valid_object (obattr));
  RpsValue_t res = RPS_NULL_VALUE;
  pthread_mutex_lock (&obj->ob_mtx);
  if (obattr == RPS_ROOT_OB (_41OFI3r0S1t03qdB2E))	//class∈class
    {
      res = (RpsValue_t) (obj->ob_class);
      goto end;
    }
  if (obattr == RPS_ROOT_OB (_2i66FFjmS7n03HNNBx)	//space∈class
      || obattr == RPS_ROOT_OB (_9uwZtDshW4401x6MsY)	//space∈symbol
    )
    {
      res = (RpsValue_t) (obj->ob_space);
      goto end;
    };
  RpsAttrTable_t *atbl = obj->ob_attrtable;
  if (!atbl)
    goto end;
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (atbl) == -RpsPyt_AttrTable);
  res = rps_attr_table_find (atbl, obattr);
end:
  pthread_mutex_unlock (&obj->ob_mtx);
  return res;
}				/* end rps_get_object_attribute */

RpsValue_t
rps_get_object_component (RpsObject_t * obj, int ix)
{
  RpsValue_t res = RPS_NULL_VALUE;
  if (!obj)
    return RPS_NULL_VALUE;
  RPS_ASSERT (rps_is_valid_object (obj));
  pthread_mutex_lock (&obj->ob_mtx);
  unsigned nbc = obj->ob_nbcomp;
  RPS_ASSERT (nbc <= obj->ob_compsize);
  if (ix < 0)
    ix += (int) nbc;
  if (ix >= 0 && ix < nbc)
    {
      RPS_ASSERT (obj->ob_comparr);
      res = obj->ob_comparr[ix];
    };
end:
  pthread_mutex_unlock (&obj->ob_mtx);
  return res;
}				/* end rps_get_object_component */

void
rps_put_object_attribute (RpsObject_t * obj, RpsObject_t * obattr,
			  RpsValue_t val)
{
  if (!obj || !obattr)
    return;
  RPS_ASSERT (rps_is_valid_object (obj));
  RPS_ASSERT (rps_is_valid_object (obattr));
  if (val == RPS_NULL_VALUE)
    return;
  pthread_mutex_lock (&obj->ob_mtx);
  if (obattr == RPS_ROOT_OB (_41OFI3r0S1t03qdB2E))	//class∈class
    {
      if (rps_value_type (val) == RPS_TYPE_OBJECT)
	{
	  /// need an test that the value has some payload...
	  obj->ob_class = (RpsObject_t *) val;
	}
      goto end;
    };
  if (obattr == RPS_ROOT_OB (_2i66FFjmS7n03HNNBx)	//space∈class
      || obattr == RPS_ROOT_OB (_9uwZtDshW4401x6MsY)	//space∈symbol
    )
    {
      if (rps_value_type (val) == RPS_TYPE_OBJECT)
	{
	  /// need an test that the value has some payload...
	  obj->ob_space = (RpsObject_t *) val;
	}
      goto end;
    }
  obj->ob_attrtable = rps_attr_table_put (obj->ob_attrtable, obattr, val);
end:
  pthread_mutex_unlock (&obj->ob_mtx);
}				/* end rps_put_object_attribute */

void
rps_object_reserve_components (RpsObject_t * obj, unsigned nbcomp)
{
  if (!obj)
    return;
  if (nbcomp > RPS_MAX_NB_OBJECT_COMPONENTS)
    {
      char obidbuf[32];
      memset (obidbuf, 0, sizeof (obidbuf));
      rps_oid_to_cbuf (obj->ob_id, obidbuf);
      RPS_FATAL ("too many components %u for object %s", nbcomp, obidbuf);
    };
  pthread_mutex_lock (&obj->ob_mtx);
  unsigned oldnbcomp = obj->ob_nbcomp;
  unsigned oldcompsize = obj->ob_compsize;
  RPS_ASSERT (oldnbcomp <= oldcompsize);
  RPS_ASSERT (oldcompsize < RPS_MAX_NB_OBJECT_COMPONENTS);
  if ((nbcomp + 2) >= oldcompsize)
    {
      unsigned newcompsize = rps_prime_above (nbcomp + oldnbcomp / 3
					      + nbcomp / 8 + 3);
      RPS_ASSERTPRINTF (newcompsize > 0
			&& newcompsize < RPS_MAX_NB_OBJECT_COMPONENTS
			&& newcompsize > nbcomp,
			"nbcomp=%u newcompsize=%u oldcompsize=%u",
			nbcomp, newcompsize, oldcompsize);
      RpsValue_t *newcomparr =
	RPS_ALLOC_ZEROED (sizeof (RpsValue_t) * newcompsize);
      for (unsigned ix = 0; ix < oldnbcomp; ix++)
	newcomparr[ix] = obj->ob_comparr[ix];
      free (obj->ob_comparr);
      obj->ob_comparr = newcomparr;
      obj->ob_compsize = newcompsize;
    }
end:
  pthread_mutex_unlock (&obj->ob_mtx);
}				/* end rps_object_reserve_components */

void *
rps_get_object_payload_of_type (RpsObject_t * obj, int paylty)
{
  struct rps_owned_payload_st *payl = NULL;
  if (!rps_is_valid_object (obj))
    return NULL;
  pthread_mutex_lock (&obj->ob_mtx);
  {
    struct rps_owned_payload_st *obpayl = obj->ob_payload;
    if (obpayl)
      {
	if (paylty == 0)
	  payl = obpayl;
	else if (paylty != 0 && rps_zoned_memory_type (obpayl) == paylty)
	  payl = obpayl;
      }
  }
end:
  pthread_mutex_unlock (&obj->ob_mtx);
  return payl;
}				/* end rps_get_object_payload_of_type */


bool
rps_object_less (RpsObject_t * ob1, RpsObject_t * ob2)
{
  if (ob1 == ob2)
    return false;
  if (!ob1)
    return true;
  if (!ob2)
    return false;
  if (RPS_ZONED_MEMORY_TYPE (ob1) != RPS_TYPE_OBJECT)
    RPS_FATAL ("non-object ob1 @%p", ob1);
  if (RPS_ZONED_MEMORY_TYPE (ob2) != RPS_TYPE_OBJECT)
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
		    (primsiz * sizeof (struct rps_attrentry_st)),
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
  if (RPS_ZONED_MEMORY_TYPE (tbl) != -RpsPyt_AttrTable)
    return RPS_NULL_VALUE;
  if (!rps_is_valid_object (obattr))
    return RPS_NULL_VALUE;
  intptr_t tblsiz = rps_prime_of_index (tbl->zm_xtra);
  unsigned tbllen = tbl->zm_length;
  int lo = 0, hi = (int) tbllen - 1;
  while (lo + 4 < hi)
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
			  RpsValue_t val)
{
  RPS_ASSERT (tbl != NULL);
  intptr_t tblsiz = rps_prime_of_index (tbl->zm_xtra);
  unsigned tbllen = tbl->zm_length;
  RPS_ASSERT (obattr != NULL);
  RPS_ASSERT (val != RPS_NULL_VALUE);
  RPS_ASSERT (tbllen < tblsiz);
  if (tbllen == 0)
    {
      tbl->attr_entries[0].ent_attr = obattr;
      tbl->attr_entries[0].ent_val = val;
      tbl->zm_length = 1;
      return true;
    };
  int lo = 0, hi = (int) tbllen - 1;
  while (lo + 4 < hi)
    {
      int mi = (lo + hi) / 2;
      RpsObject_t *curattr = tbl->attr_entries[mi].ent_attr;
      if (curattr == obattr)
	{
	  tbl->attr_entries[mi].ent_val = val;
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
	  tbl->attr_entries[ix].ent_val = val;
	  return true;
	}
      else if (rps_object_less (curattr, obattr))
	{
	  memmove (tbl->attr_entries + ix, tbl->attr_entries + ix + 1,
		   (tbllen - ix) * sizeof (struct rps_attrentry_st));
	  tbl->attr_entries[ix].ent_attr = obattr;
	  tbl->attr_entries[ix].ent_val = val;
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
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (tbl) == -RpsPyt_AttrTable);
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
  /// we don't free the old_tbl, it will be garbage collected...
  return new_tbl;
}				/* end rps_attr_table_put */


RpsAttrTable_t *
rps_attr_table_remove (RpsAttrTable_t * tbl, RpsObject_t * obattr)
{
  if (!obattr || !rps_is_valid_object (obattr))
    return tbl;
  if (!tbl)
    return NULL;
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (tbl) == -RpsPyt_AttrTable);
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
			    (newprimsiz * sizeof (struct rps_attrentry_st)),
			    -RpsPyt_AttrTable);
	  new_tbl->zm_xtra = newprimix;
	  new_tbl->zm_length = 0;
	  for (int ix = 0; ix < pos; ix++)
	    new_tbl->attr_entries[ix] = old_tbl->attr_entries[ix];
	  for (int ix = pos + 1; ix < oldtbllen; ix++)
	    new_tbl->attr_entries[ix] = old_tbl->attr_entries[ix - 1];
	  new_tbl->zm_length = oldtbllen - 1;
	  //// we don't free the old_tbl, it will be later garbage collected
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


unsigned
rps_attr_table_size (const RpsAttrTable_t * tbl)
{
  if (!tbl)
    return 0;
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (tbl) == -RpsPyt_AttrTable);
  return tbl->zm_length;
}				/* end rps_attr_table_size */

unsigned
rps_attr_table_iterate (const RpsAttrTable_t * tbl,
			rps_object_callback_sig_t * routattr,
			rps_value_callback_sig_t * routval, void *data)
{
  if (!tbl)
    return NULL;
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (tbl) == -RpsPyt_AttrTable);
  unsigned tsiz = rps_attr_table_size (tbl);
  int cnt = 0;
  unsigned nbiter = 0;
  for (int eix = 0; eix < (int) tsiz; eix++)
    {
      if (tbl->attr_entries[eix].ent_attr != (RpsObject_t *) NULL
	  && tbl->attr_entries[eix].ent_attr != RPS_HTB_EMPTY_SLOT)
	{
	  if (routattr)
	    if (!(*routattr) (tbl->attr_entries[eix].ent_attr, data))
	      break;
	  if (routval)
	    {
	      if (!(*routval) (tbl->attr_entries[eix].ent_val, data))
		break;
	    };
	  nbiter++;
	}
    };
  return nbiter;
}				/* end of rps_attr_table_iterate */

unsigned
rps_attr_table_dump_scan (RpsDumper_t * du, const RpsAttrTable_t * tbl,
			  unsigned depth)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  if (!tbl)
    return 0;
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (tbl) == -RpsPyt_AttrTable);
  unsigned tsiz = rps_attr_table_size (tbl);
  int cnt = 0;
  unsigned nbiter = 0;
  for (int eix = 0; eix < (int) tsiz; eix++)
    {
      if (tbl->attr_entries[eix].ent_attr != (RpsObject_t *) NULL
	  && tbl->attr_entries[eix].ent_attr != RPS_HTB_EMPTY_SLOT)
	{
	  rps_dumper_scan_object (du, tbl->attr_entries[eix].ent_attr);
	  rps_dumper_scan_value (du, tbl->attr_entries[eix].ent_val,
				 depth + 1);
	  nbiter++;
	}
    };
  return nbiter;
}				/* end rps_attr_table_dump_scan */

const RpsSetOb_t *
rps_attr_table_set_of_attributes (const RpsAttrTable_t * tbl)
{
  RpsSetOb_t *setv = NULL;
  if (!tbl)
    return NULL;
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (tbl) == -RpsPyt_AttrTable);
  unsigned tsiz = rps_attr_table_size (tbl);
  RpsObject_t **obarr =
    RPS_ALLOC_ZEROED ((tsiz + 1) * sizeof (RpsObject_t *));
  int cnt = 0;
  for (int eix = 0; eix < (int) tsiz; eix++)
    {
      if (tbl->attr_entries[eix].ent_attr != (RpsObject_t *) NULL
	  && tbl->attr_entries[eix].ent_attr != RPS_HTB_EMPTY_SLOT)
	obarr[cnt++] = tbl->attr_entries[eix].ent_attr;
    };
  setv = rps_alloc_set_sized (cnt, obarr);
  free (obarr);
  return setv;
}				/* end  rps_attr_table_set_of_attributes */



/*****************************************************************
 * Objects.
 *
 * We need to quickly and concurrently be able to find an object from
 * its oid.  For that we have an array of buckets, named
 * rps_object_bucket_array, each one owning a mutex to enable parallel
 * access in several threads.  Each bucket is an hashtable of object
 * pointers.  That bucket hashtable needs to be no more than two
 * third full, otherwise finding an object in its bucket could take
 * too much time!
 *****************************************************************/
struct rps_object_bucket_st
{
  pthread_mutex_t obuck_mtx;
  unsigned obuck_card;		/* number of objects in the bucket */
  unsigned obuck_capacity;	/* allocated size of obuck_arr, some prime */
  RpsObject_t **obuck_arr;
};

enum rps_bucket_grow_en
{
  RPS_BUCKET_FIXED,
  RPS_BUCKET_GROWING
};

bool rps_object_bucket_is_nearly_full (struct rps_object_bucket_st *buck);

/* If a bucket is full, return a larger capacity for later resizing;
   but if the bucket is not nearly full, return 0 */
unsigned rps_object_bucket_perhaps_increased_capacity (struct
						       rps_object_bucket_st
						       *buck);

static void
rps_add_object_to_locked_bucket (struct rps_object_bucket_st *buck,
				 RpsObject_t * obj,
				 enum rps_bucket_grow_en growmod);
struct rps_object_bucket_st rps_object_bucket_array[RPS_OID_MAXBUCKETS];
static pthread_mutexattr_t rps_objmutexattr;


void
rps_initialize_objects_machinery (void)
{
  static bool initialized;
  static const int initialbucksize = 7;
  if (initialized)
    RPS_FATAL ("rps_initialize_objects_machinery called twice");
  if (pthread_mutexattr_init (&rps_objmutexattr))
    RPS_FATAL ("failed to init rps_objmutexattr");
  if (pthread_mutexattr_settype (&rps_objmutexattr, PTHREAD_MUTEX_RECURSIVE))
    RPS_FATAL ("failed to settype rps_objmutexattr");
  for (int bix = 0; bix < RPS_OID_MAXBUCKETS; bix++)
    {
      struct rps_object_bucket_st *curbuck = rps_object_bucket_array + bix;
      pthread_mutex_init (&curbuck->obuck_mtx, &rps_objmutexattr);
      curbuck->obuck_card = 0;
      curbuck->obuck_capacity = initialbucksize;
      curbuck->obuck_arr =
	RPS_ALLOC_ZEROED (sizeof (RpsObject_t *) * initialbucksize);
    }
  initialized = true;
  printf
    ("did rps_initialize_objects_machinery initialbucksize=%u RPS_OID_MAXBUCKETS=%u (%s:%d)\n",
     initialbucksize, RPS_OID_MAXBUCKETS, __FILE__, __LINE__);
}				/* end rps_initialize_objects_machinery */


void
rps_check_all_objects_buckets_are_valid (void)
{
  for (int bix = 0; bix < RPS_OID_MAXBUCKETS; bix++)
    {
      struct rps_object_bucket_st *curbuck = rps_object_bucket_array + bix;
      pthread_mutex_lock (&curbuck->obuck_mtx);
      RPS_ASSERTPRINTF (curbuck->obuck_capacity > 2,
			"bucket#%d wrong capacity %u", bix,
			curbuck->obuck_capacity);
      RPS_ASSERTPRINTF (curbuck->obuck_arr != NULL, "bucket#%d missing array",
			bix);
      RPS_ASSERTPRINTF (curbuck->obuck_card < curbuck->obuck_capacity,
			"bucket#%d bad cardinal %u for capacity %u", bix,
			curbuck->obuck_card, curbuck->obuck_capacity);
      RPS_ASSERTPRINTF (!rps_object_bucket_is_nearly_full (curbuck),
			"nearly full bucket#%u capacity %u for cardinal %u",
			bix, curbuck->obuck_capacity, curbuck->obuck_card);
      pthread_mutex_unlock (&curbuck->obuck_mtx);
    }
}				/* end rps_check_all_objects_buckets_are_valid */



void
rps_initialize_objects_for_loading (RpsLoader_t * ld, unsigned totnbobj)
{
  RPS_ASSERT (rps_is_valid_loader (ld));
  /// we have at least two objects, and when we have a million of
  /// them, this code should have been regenerated automatically.
  RPS_ASSERTPRINTF (totnbobj > 2, "totnbobj %u", totnbobj);
  RPS_ASSERTPRINTF (totnbobj < 1000000, "totnbobj %u", totnbobj);
  /// A bucket is nearly full if less than a third of the slots are
  /// empty.  See code of rps_object_bucket_is_nearly_full below. We
  /// preallocate each of them for more than twice the total number of
  /// objects on average... So each of them should be less than half
  /// full on average.
  unsigned minbucksize =
    rps_prime_above (5 + (2 * totnbobj + totnbobj / 4) / RPS_OID_MAXBUCKETS);
  printf
    ("rps_initialize_objects_for_loading totnbobj=%u minbucksize=%u (%s:%d)\n",
     totnbobj, minbucksize, __FILE__, __LINE__);
  for (int bix = 0; bix < RPS_OID_MAXBUCKETS; bix++)
    {
      struct rps_object_bucket_st *curbuck = rps_object_bucket_array + bix;
      pthread_mutex_lock (&curbuck->obuck_mtx);
      if (curbuck->obuck_arr == NULL)
	{
	  RPS_ASSERTPRINTF (curbuck->obuck_card == 0,
			    "empty bucket#%d corrupted cardinal %u", bix,
			    curbuck->obuck_card);
	  RPS_ASSERTPRINTF (curbuck->obuck_capacity == 0,
			    "empty bucket#%d corrupted capacity %u", bix,
			    curbuck->obuck_capacity);
	  curbuck->obuck_capacity = minbucksize;
	  curbuck->obuck_card = 0;
	  curbuck->obuck_arr =
	    RPS_ALLOC_ZEROED (sizeof (RpsObject_t *) * minbucksize);
	}
      else
	RPS_ASSERTPRINTF (curbuck->obuck_capacity > 0,
			  "bucket#%d corrupted capacity %u", bix,
			  curbuck->obuck_capacity);
      pthread_mutex_unlock (&rps_object_bucket_array[bix].obuck_mtx);
    }
  //printf
  //  ("rps_initialize_objects_for_loading ending totnbobj=%u minbucksize=%u (%s:%d)\n",
  //   totnbobj, minbucksize, __FILE__, __LINE__);
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
rps_find_object_by_oid (const RpsOid oid)
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
  unsigned cbucksiz = curbuck->obuck_capacity;
  RPS_ASSERTPRINTF (cbucksiz > 3, "bad bucket#%u capacity %u", bix, cbucksiz);
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


/* An object bucket is nearly full if less than a third of slots are
   empty, and we need two empty slots... */
bool
rps_object_bucket_is_nearly_full (struct rps_object_bucket_st *buck)
{
  RPS_ASSERT (buck != NULL);
  RPS_ASSERT (buck >= rps_object_bucket_array
	      && buck < rps_object_bucket_array + RPS_OID_MAXBUCKETS);
  if (buck->obuck_capacity == 0)
    {
      RPS_ASSERT (buck->obuck_card == 0);
      RPS_ASSERT (buck->obuck_arr == NULL);
      return true;
    };
  RPS_ASSERT (buck->obuck_capacity >= buck->obuck_card);
  RPS_ASSERT (buck->obuck_capacity > 0);
  RPS_ASSERT (buck->obuck_arr != NULL);
  // at least two empty slots ....
  if (buck->obuck_card + 2 > buck->obuck_capacity)
    return true;
  // otherwise, a fourth of them should be empty ...
  return (4 * (buck->obuck_capacity - buck->obuck_card) <
	  buck->obuck_capacity);
}				/* end rps_object_bucket_is_nearly_full */


// return 0 if growing the bucket is not necessary, but a larger
// capacity if so needed.  The increased capacity is some prime
// number...
unsigned
rps_object_bucket_perhaps_increased_capacity (struct rps_object_bucket_st
					      *buck)
{
  RPS_ASSERT (buck != NULL);
  RPS_ASSERT (buck >= rps_object_bucket_array
	      && buck < rps_object_bucket_array + RPS_OID_MAXBUCKETS);
  if (buck->obuck_capacity == 0)
    return 7;
  RPS_ASSERT (buck->obuck_arr != NULL);
  if (buck->obuck_card + 2 > buck->obuck_capacity)
    return rps_prime_above (3 * buck->obuck_card / 2 +
			    buck->obuck_capacity / 8 + 6);
  RPS_ASSERT (buck->obuck_card < buck->obuck_capacity);
  if (3 * (buck->obuck_capacity - buck->obuck_card) >
      buck->obuck_capacity + 2)
    // resize not needed, so...
    return 0;
  return rps_prime_above (3 * buck->obuck_card / 2 +
			  buck->obuck_capacity / 8 + 6);
}				/* end rps_object_bucket_perhaps_increased_capacity */

static void
rps_add_object_to_locked_bucket (struct rps_object_bucket_st *buck,
				 RpsObject_t * obj,
				 enum rps_bucket_grow_en growmode)
{
  RPS_ASSERT (buck != NULL);
  RPS_ASSERT (obj != NULL);
  unsigned newsiz = 0;
  int buckix = buck - rps_object_bucket_array;
  RPS_ASSERTPRINTF (buckix >= 0 && buckix < RPS_OID_MAXBUCKETS,
		    "bad bucket index #%d", buckix);
  static int addcnt;
  if (addcnt % 8 == 0)
    {
      rps_check_all_objects_buckets_are_valid ();
      //   if (addcnt % 32 == 0)
      //     printf
      //     ("rps_add_object_to_locked_bucket bucket#%d addcnt#%d %s (%s:%d)\n",
      //      buckix, addcnt,
      //      (growmode == RPS_BUCKET_FIXED) ? "fixed" : "growing",
      //      __FILE__, __LINE__);
    }
  addcnt++;
  unsigned cbucksiz = buck->obuck_capacity;
  RPS_ASSERTPRINTF (cbucksiz > 0, "bucket#%d zerosized", buckix);
  RPS_ASSERTPRINTF (buck->obuck_capacity > 0
		    && buck->obuck_capacity > buck->obuck_card,
		    "bucket#%d corrupted capacity %u for cardinal %u",
		    buckix, buck->obuck_capacity, buck->obuck_card);
  newsiz = rps_object_bucket_perhaps_increased_capacity (buck);
  if (newsiz > 0)
    {
      /// So less than a third of slots is empty.... See above code of
      /// rps_object_bucket_is_nearly_full
      RPS_ASSERTPRINTF (growmode == RPS_BUCKET_GROWING,
			"bad growmode#%d for buckix#%d", (int) growmode,
			buckix);
      RPS_ASSERTPRINTF (newsiz > cbucksiz + 3, "bad newsiz %u for buckix#%d",
			newsiz, buckix);
      RPS_ASSERTPRINTF (3 * newsiz > 2 * cbucksiz,
			"bad newsiz %u cbucksiz %u for buckix#%d", newsiz,
			cbucksiz, buckix);
      RpsObject_t **oldarr = buck->obuck_arr;
      buck->obuck_arr = RPS_ALLOC_ZEROED (sizeof (RpsObject_t *) * newsiz);
      buck->obuck_capacity = newsiz;
      buck->obuck_card = 0;
      for (int ix = 0; ix < (int) cbucksiz; ix++)
	{
	  RpsObject_t *oldobj = oldarr[ix];
	  if (oldobj)		/* this recursion happens once! */
	    rps_add_object_to_locked_bucket (buck, oldobj, RPS_BUCKET_FIXED);
	};
      free (oldarr);
      cbucksiz = newsiz;
    };
  RPS_ASSERTPRINTF (buck->obuck_capacity > 0
		    && buck->obuck_capacity > buck->obuck_card,
		    "corrupted bucket#%d capacity %u card %u", buckix,
		    buck->obuck_capacity, buck->obuck_card);
  RPS_ASSERTPRINTF (rps_object_bucket_perhaps_increased_capacity (buck) == 0,
		    "could be increased bucket#%d capacity %u card %u",
		    buckix, buck->obuck_capacity, buck->obuck_card);
  RPS_ASSERTPRINTF (!rps_object_bucket_is_nearly_full (buck),
		    "nearly full bucket#%d capacity %u card %u", buckix,
		    buck->obuck_capacity, buck->obuck_card);
  RPS_ASSERTPRINTF (cbucksiz > 3,
		    "bad bucket#%d (max %u) capacity %u card %u array %p addcnt#%d",
		    buckix, RPS_OID_MAXBUCKETS, cbucksiz, buck->obuck_card,
		    buck->obuck_arr, addcnt);
  RPS_ASSERTPRINTF (buck->obuck_capacity > buck->obuck_card,
		    "bucket#%d oversized capacity:%d card:%d", buckix,
		    buck->obuck_capacity, buck->obuck_card);
  unsigned stix = (obj->ob_id.id_hi ^ obj->ob_id.id_lo) % cbucksiz;
  for (int ix = stix; ix < (int) cbucksiz; ix++)
    {
      RpsObject_t *curob = buck->obuck_arr[ix];
      if (NULL == curob)
	{
	  buck->obuck_arr[ix] = obj;
	  buck->obuck_card++;
	  RPS_ASSERTPRINTF (!rps_object_bucket_is_nearly_full (buck),
			    "wrongly full bucket#%d of card %u capacity %u",
			    buckix, buck->obuck_card, buck->obuck_capacity);
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
	  RPS_ASSERTPRINTF (!rps_object_bucket_is_nearly_full (buck),
			    "wrongly full bucket#%d of card %u size %u",
			    buckix, buck->obuck_card, buck->obuck_capacity);
	  return;
	}
      if (curob == obj)
	return;
    };
}				/* end rps_add_object_to_locked_bucket */


RpsObject_t *
rps_get_loaded_object_by_oid (RpsLoader_t * ld, const RpsOid oid)
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
      // the infant object temporary class is the object class, which
      // might not exist yet ...
      obinfant->ob_class = RPS_ROOT_OB (_5yhJGgxLwLp00X0xEQ);	//object∈class
      // see also routine rps_load_initialize_root_objects
      pthread_mutex_lock (&rps_object_bucket_array[bix].obuck_mtx);
      curbuck = &rps_object_bucket_array[bix];
      if (!curbuck->obuck_arr)
	{
	  unsigned inibucksiz =
	    rps_prime_above (4 + ((rps_loader_nb_globals (ld) +
				   rps_loader_nb_constants (ld))
				  / RPS_OID_MAXBUCKETS));
	  curbuck->obuck_arr =
	    RPS_ALLOC_ZEROED (sizeof (RpsObject_t *) * inibucksiz);
	  curbuck->obuck_capacity = inibucksiz;
	  curbuck->obuck_card = 0;
	};
      RPS_ASSERT (!rps_object_bucket_is_nearly_full (curbuck));
      rps_add_object_to_locked_bucket (curbuck, obinfant, RPS_BUCKET_GROWING);
      pthread_mutex_unlock (&curbuck->obuck_mtx);
      return obinfant;
    }
  else if (rps_is_valid_filling_loader (ld))
    {
      /* we need to find an existing object */
      return rps_find_object_by_oid (oid);
    }
}				/* end rps_get_loaded_object_by_oid */


/// function to dump object attributes, has a signature compatible with rps_apply_dumpj_sigt
RpsValue_t
rpscloj_dump_object_attributes (rps_callframe_t * callerframe,
				const RpsClosure_t * clos,
				RpsDumper_t * du,
				RpsValue_t dumpedobv, json_t * js)
{
  /* The dumped object is locked by caller... */
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (rps_value_type (dumpedobv) == RPS_TYPE_OBJECT);
  RpsObject_t *obdump = (RpsObject_t *) dumpedobv;
  RPS_ASSERT (json_is_object (js));
  if (!obdump->ob_attrtable)
    return (RpsValue_t) obdump;
  const RpsSetOb_t *setattrs =
    rps_attr_table_set_of_attributes (obdump->ob_attrtable);
  json_t *jsarr = json_array ();
  unsigned nbattrs = rps_set_cardinal (setattrs);
  if (nbattrs == 0)
    return obdump;
  for (int aix = 0; aix < (int) nbattrs; aix++)
    {
      const RpsObject_t *obattr = rps_set_nth_member (setattrs, aix);
      if (!rps_is_dumpable_object (du, obattr))
	continue;
      RpsValue_t curval = rps_attr_table_find (obdump->ob_attrtable, obattr);
      if (!rps_is_dumpable_value (du, curval))
	continue;
      json_t *jent = json_object ();
      json_object_set (jent, "at", rps_dump_json_for_object (du, obattr));
      json_object_set (jent, "va", rps_dump_json_for_value (du, curval, 1));
      json_array_append_new (jsarr, jent);
    }
  // setattrs should not leak, we can free it explicitly
  memset (setattrs, 0, sizeof (setattrs));
  free (setattrs);
  json_object_set (js, "attrs", jsarr);
  return (RpsValue_t) obdump;
}				/* end of rpscloj_dump_object_attributes */


/// function to dump object components, has a signature compatible with rps_apply_dumpj_sigt
RpsValue_t
rpscloj_dump_object_components (rps_callframe_t * callerframe,
				const RpsClosure_t * clos,
				RpsDumper_t * du,
				RpsValue_t dumpedobv, json_t * js)
{
  /* The dumped object is locked by caller... */
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (rps_value_type (dumpedobv) == RPS_TYPE_OBJECT);
  RpsObject_t *obdump = (RpsObject_t *) dumpedobv;
  RPS_ASSERT (json_is_object (js));
  unsigned nbc = obdump->ob_nbcomp;
  if (nbc == 0)
    return (RpsValue_t) obdump;
  json_t *jsarr = json_array ();
  for (int cix = 0; cix < (int) nbc; cix++)
    {
      RpsValue_t compv = rps_get_object_component (obdump, cix);
      json_t *jva = rps_dump_json_for_value (du, compv, 0);
      json_array_append_new (jsarr, jva);
    }
  json_object_set (js, "comps", jsarr);
  return (RpsValue_t) obdump;
}				/* end of rpscloj_dump_object_attributes */


static pthread_mutex_t rps_payload_mtx =
  PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static rps_payload_remover_t
  * rps_payload_removing_rout_arr[RPS_MAX_PAYLOAD_TYPE_INDEX];
static void *rps_payload_removing_data_arr[RPS_MAX_PAYLOAD_TYPE_INDEX];
static rps_payload_dump_scanner_t
  * rps_payload_dump_scanning_rout_arr[RPS_MAX_PAYLOAD_TYPE_INDEX];
static void *rps_payload_dump_scanning_data_arr[RPS_MAX_PAYLOAD_TYPE_INDEX];
static rps_payload_dump_serializer_t
  * rps_payload_dump_serializing_rout_arr[RPS_MAX_PAYLOAD_TYPE_INDEX];
static void
  *rps_payload_dump_serializing_data_arr[RPS_MAX_PAYLOAD_TYPE_INDEX];

void
rps_register_payload_removal (int paylty, rps_payload_remover_t * rout,
			      void *data)
{
  pthread_mutex_lock (&rps_payload_mtx);
  if (paylty == 0 || paylty >= RPS_MAX_PAYLOAD_TYPE_INDEX)
    {
      Dl_info routinfo = { };
      dladdr ((void *) rout, &routinfo);
      const char *routname = routinfo.dli_sname;
      if (!routname)
	routname = "???";
      RPS_FATAL
	("payload type#%d invalid for payload removal routine %p / %s",
	 paylty, (void *) rout, routname);
    };
  if ((void *) rout == NULL && data != NULL)
    {
      RPS_FATAL
	("payload type#%d without removal routine, but with some removal data @%p",
	 paylty, data);
    }
  rps_payload_removing_rout_arr[paylty] = rout;
  rps_payload_removing_data_arr[paylty] = data;
end:
  pthread_mutex_unlock (&rps_payload_mtx);
}				/* end rps_register_payload_removal */

void
rps_register_payload_dump_scanner (int paylty, rps_payload_dump_scanner_t * rout,	//
				   void *data)
{
  pthread_mutex_lock (&rps_payload_mtx);
  if (paylty == 0 || paylty >= RPS_MAX_PAYLOAD_TYPE_INDEX)
    {
      Dl_info routinfo = { };
      dladdr ((void *) rout, &routinfo);
      const char *routname = routinfo.dli_sname;
      if (!routname)
	routname = "???";
      RPS_FATAL
	("payload type#%d invalid for payload dump scanner routine %p / %s",
	 paylty, (void *) rout, routname);
    };
  if ((void *) rout == NULL && data != NULL)
    {
      RPS_FATAL
	("payload type#%d without dump scanner routine, but with some removal data @%p",
	 paylty, data);
    }
  rps_payload_dump_scanning_rout_arr[paylty] = rout;
  rps_payload_dump_scanning_data_arr[paylty] = data;
end:
  pthread_mutex_unlock (&rps_payload_mtx);
}				/* end rps_register_payload_dump_scanner */


/* this function is called with the object locked */
void
rps_dump_scan_object_payload (RpsDumper_t * du, RpsObject_t * ob)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (rps_is_valid_object (ob));
  struct rps_owned_payload_st *payl =
    (struct rps_owned_payload_st *) (ob->ob_payload);
  RPS_ASSERT (payl != NULL);
  RPS_ASSERT (payl->payl_owner == ob);
  int8_t paylty = atomic_load (&payl->zm_atype);
  RPS_ASSERT (paylty < 0 && paylty > -RpsPyt__LAST);
  rps_payload_dump_scanner_t *scanrout = NULL;
  void *scandata = NULL;
  {
    pthread_mutex_lock (&rps_payload_mtx);
    scanrout = rps_payload_dump_scanning_rout_arr[-paylty];
    scandata = rps_payload_dump_scanning_data_arr[-paylty];
    pthread_mutex_unlock (&rps_payload_mtx);
  }
  if (scanrout)
    (*scanrout) (du, payl, scandata);
  else
    RPS_DEBUG_PRINTF (DUMP,
		      "object %-1O of class %-1O has payload of type #%d without scanning routine !!!",
		      ob, ob->ob_class, (int) paylty);
}				/* end rps_dump_scan_object_payload */

void
rps_register_payload_dump_serializer (int paylty, rps_payload_dump_serializer_t * rout,	//
				      void *data)
{
  pthread_mutex_lock (&rps_payload_mtx);
  if (paylty == 0 || paylty >= RPS_MAX_PAYLOAD_TYPE_INDEX)
    {
      Dl_info routinfo = { };
      dladdr ((void *) rout, &routinfo);
      const char *routname = routinfo.dli_sname;
      if (!routname)
	routname = "???";
      RPS_FATAL
	("payload type#%d invalid for payload dump scanner routine %p / %s",
	 paylty, (void *) rout, routname);
    };
  if ((void *) rout == NULL && data != NULL)
    {
      RPS_FATAL
	("payload type#%d without dump scanner routine, but with some removal data @%p",
	 paylty, data);
    }
  rps_payload_dump_serializing_rout_arr[paylty] = rout;
  rps_payload_dump_serializing_data_arr[paylty] = data;
end:
  pthread_mutex_unlock (&rps_payload_mtx);
}				/* end rps_register_payload_dump_serializer */

/* this function is called with the object locked */
void
rps_dump_serialize_object_payload (RpsDumper_t * du, RpsObject_t * ob,
				   json_t * jsob)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (rps_is_valid_object (ob));
  RPS_ASSERT (jsob && json_is_object (jsob));
  struct rps_owned_payload_st *payl =
    (struct rps_owned_payload_st *) (ob->ob_payload);
  RPS_ASSERT (payl != NULL);
  RPS_ASSERT (payl->payl_owner == ob);
  int8_t paylty = atomic_load (&payl->zm_atype);
  RPS_ASSERT (paylty < 0 && paylty > -RpsPyt__LAST);
  rps_payload_dump_serializer_t *serirout = NULL;
  void *seridata = NULL;
  {
    pthread_mutex_lock (&rps_payload_mtx);
    serirout = rps_payload_dump_serializing_rout_arr[-paylty];
    seridata = rps_payload_dump_serializing_data_arr[-paylty];
    pthread_mutex_unlock (&rps_payload_mtx);
  }
  if (serirout)
    (*serirout) (du, payl, jsob, seridata);
}				/* end rps_dump_serialize_object_payload */

void
rps_object_put_payload (RpsObject_t * obj, void *payl)
{
  if (!obj)
    return;
  RPS_ASSERT (rps_is_valid_object (obj));
  struct rps_owned_payload_st *newpayl = payl;
  int newptype = 0;
  if (newpayl)
    {
      newptype = -RPS_ZONED_MEMORY_TYPE (newpayl);
      RPS_ASSERT (newptype > 0 && newptype < RPS_MAX_PAYLOAD_TYPE_INDEX);
    }
  pthread_mutex_lock (&obj->ob_mtx);
  struct rps_owned_payload_st *oldpayl = obj->ob_payload;
  if (oldpayl)
    {
      rps_payload_remover_t *oldremover = NULL;
      void *oldremdata = NULL;
      int oldptype = RPS_ZONED_MEMORY_TYPE (oldpayl);
      if (oldptype < 0 && oldptype > -RPS_MAX_PAYLOAD_TYPE_INDEX)
	{
	  pthread_mutex_lock (&rps_payload_mtx);
	  oldremover = rps_payload_removing_rout_arr[-oldptype];
	  oldremdata = rps_payload_removing_data_arr[-oldptype];
	  pthread_mutex_unlock (&rps_payload_mtx);
	}
      if (oldremover)
	(*oldremover) (obj, oldpayl, oldremdata);
    }
  obj->ob_payload = newpayl;
  newpayl->payl_owner = obj;
end:
  pthread_mutex_unlock (&obj->ob_mtx);
}				/* end of rps_object_put_payload */





void
rpsldpy_classinfo (RpsObject_t * obj, RpsLoader_t * ld,
		   const json_t * jv, int spacix)
{
  // should be compatible with rps_classinfo_payload_dump_serializer 
  char idbuf[32];
  memset (idbuf, 0, sizeof (idbuf));
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_object (obj));
  rps_oid_to_cbuf (obj->ob_id, idbuf);
  RPS_ASSERT (rps_is_valid_loader (ld));
  RPS_ASSERT (jv != NULL && json_is_object (jv));
  RPS_ASSERT (spacix >= 0);
  json_t *jsclassmethdict = json_object_get (jv, "class_methodict");
  json_t *jsclassname = json_object_get (jv, "class_name");
  json_t *jsclasssuper = json_object_get (jv, "class_super");
  json_t *jsclasssymb = json_object_get (jv, "class_symb");
  RPS_ASSERT (jsclassmethdict != NULL);
  RPS_ASSERT (jsclassname != NULL);
  RPS_ASSERT (jsclasssuper != NULL);
  RpsClassInfo_t *clinf =
    RPS_ALLOC_ZONE (sizeof (RpsClassInfo_t), -RpsPyt_ClassInfo);
  if (json_is_array (jsclassmethdict))
    {
      int nbmeth = json_array_size (jsclassmethdict);
      RpsAttrTable_t *methdict =
	rps_alloc_empty_attr_table (nbmeth + nbmeth / 8 + 2);
      for (int mix = 0; mix < nbmeth; mix++)
	{
	  json_t *jsmethent = json_array_get (jsclassmethdict, mix);
	  RPS_ASSERT (json_is_object (jsmethent));
	  json_t *jsmethosel = json_object_get (jsmethent, "methosel");
	  json_t *jsmethclos = json_object_get (jsmethent, "methclos");
	  RpsObject_t *methselob = rps_loader_json_to_object (ld, jsmethosel);
	  RpsValue_t methclos = rps_loader_json_to_value (ld, jsmethclos);
	  methdict = rps_attr_table_put (methdict, methselob, methclos);
	}
      clinf->pclass_methdict = methdict;
    }
  if (jsclasssuper != NULL)
    clinf->pclass_super = rps_loader_json_to_object (ld, jsclasssuper);
  if (jsclasssymb != NULL)
    clinf->pclass_symbol = rps_loader_json_to_object (ld, jsclasssymb);
  clinf->pclass_magic = RPS_CLASSINFO_MAGIC;
  rps_object_put_payload (obj, clinf);
}				/* end rpsldpy_classinfo */


bool
rps_is_valid_classinfo (const RpsClassInfo_t * clinf)
{
  return clinf != NULL && rps_zoned_memory_type (clinf) == -RpsPyt_ClassInfo;
}				/* end rps_is_valid_classinfo */

RpsObject_t *
rps_classinfo_super (const RpsClassInfo_t * clinf)
{
  if (!rps_is_valid_classinfo (clinf))
    return NULL;
  if (clinf->pclass_magic != RPS_CLASSINFO_MAGIC)
    return NULL;
  return clinf->pclass_super;
}				/* end rps_classinfo_super */


RpsObject_t *
rps_classinfo_symbol (const RpsClassInfo_t * clinf)
{
  if (!rps_is_valid_classinfo (clinf))
    return NULL;
  if (clinf->pclass_magic != RPS_CLASSINFO_MAGIC)
    return NULL;
  return clinf->pclass_symbol;
}				/* end rps_classinfo_symbol */

RpsAttrTable_t *
rps_classinfo_methdict (const RpsClassInfo_t * clinf)
{
  if (!rps_is_valid_classinfo (clinf))
    return NULL;
  if (clinf->pclass_magic != RPS_CLASSINFO_MAGIC)
    return NULL;
  return clinf->pclass_methdict;
}				/* end rps_classinfo_methdict */

RpsClosure_t *
rps_classinfo_get_method (const RpsClassInfo_t * clinf, RpsObject_t * selob)
{
  if (!rps_is_valid_classinfo (clinf))
    return NULL;
  if (!rps_is_valid_object (selob))
    return NULL;
  if (!clinf->pclass_methdict)
    return NULL;
  RpsValue_t valmeth = rps_attr_table_find (clinf->pclass_methdict, selob);
  if (!valmeth)
    return NULL;
  if (rps_value_type (valmeth) != RPS_TYPE_CLOSURE)
    return NULL;
  return (RpsClosure_t *) valmeth;
}				/* end rps_classinfo_get_method */

RpsObject_t *
rps_obclass_super (RpsObject_t * obcla)
{
  RpsObject_t *obres = NULL;
  if (!obcla || !rps_is_valid_object (obcla))
    return NULL;
  pthread_mutex_lock (&obcla->ob_mtx);
  RpsClassInfo_t *clinf =
    rps_get_object_payload_of_type (obcla, -RpsPyt_ClassInfo);
  if (!clinf)
    goto end;
  obres = rps_classinfo_super (clinf);
end:
  pthread_mutex_unlock (&obcla->ob_mtx);
  return obres;
}				/* end rps_obclass_super */

RpsObject_t *
rps_obclass_symbol (RpsObject_t * obcla)
{
  RpsObject_t *obres = NULL;
  if (!obcla || !rps_is_valid_object (obcla))
    return NULL;
  pthread_mutex_lock (&obcla->ob_mtx);
  RpsClassInfo_t *clinf =
    rps_get_object_payload_of_type (obcla, -RpsPyt_ClassInfo);
  if (!clinf)
    goto end;
  obres = rps_classinfo_symbol (clinf);
end:
  pthread_mutex_unlock (&obcla->ob_mtx);
  return obres;
}				/* end rps_obclass_symbol */

RpsAttrTable_t *
rps_obclass_methdict (RpsObject_t * obcla)
{
  RpsAttrTable_t *atbl = NULL;
  if (!obcla || !rps_is_valid_object (obcla))
    return NULL;
  pthread_mutex_lock (&obcla->ob_mtx);
  RpsClassInfo_t *clinf =
    rps_get_object_payload_of_type (obcla, -RpsPyt_ClassInfo);
  if (!clinf)
    goto end;
  atbl = rps_classinfo_methdict (clinf);
end:
  pthread_mutex_unlock (&obcla->ob_mtx);
  return atbl;
}				/* end rps_obclass_methdict */

RpsClosure_t *
rps_obclass_get_method (RpsObject_t * obcla, RpsObject_t * selob)
{
  RpsClosure_t *clores = NULL;
  if (!obcla || !rps_is_valid_object (obcla))
    return NULL;
  if (!selob || !rps_is_valid_object (selob))
    return NULL;
  pthread_mutex_lock (&obcla->ob_mtx);
  RpsClassInfo_t *clinf =
    rps_get_object_payload_of_type (obcla, -RpsPyt_ClassInfo);
  if (!clinf)
    goto end;
  clores = rps_classinfo_get_method (clinf, selob);
end:
  pthread_mutex_unlock (&obcla->ob_mtx);
  return clores;
}				/* end rps_obclass_get_method */


/// rps_classinfo_payload_remover is a rps_payload_remover_t for classinfo
/// it has been registered (in main) by rps_register_payload_removal
void
rps_classinfo_payload_remover (RpsObject_t * ob,
			       struct rps_owned_payload_st *clpayl,
			       void *data)
{
  /// the ob has been locked...
  RPS_ASSERT (ob && rps_is_valid_object (ob));
  RPS_ASSERT (clpayl && rps_zoned_memory_type (clpayl) == -RpsPyt_ClassInfo);
  RpsClassInfo_t *clinf = (RpsClassInfo_t *) clpayl;
  RPS_ASSERT (clinf->pclass_magic == RPS_CLASSINFO_MAGIC);
  /// we cannot free clinf, the garbage collection should later do it.
  /// we clear the magic number and the owner
  clinf->pclass_magic = 0;
  clinf->payl_owner = NULL;
  clinf->pclass_super = NULL;	// will be garbage collected.
  clinf->pclass_methdict = NULL;	// will be garbage collected
  clinf->pclass_symbol = NULL;
#warning rps_classinfo_payload_remover need a code review
  /// TODO: should we also clear the zm_length, zm_xtra fields?
}				/* end rps_classinfo_payload_remover */


void
rps_classinfo_payload_dump_scanner (RpsDumper_t * du,
				    struct rps_owned_payload_st *payl,
				    void *data)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (payl && rps_zoned_memory_type (payl) == -RpsPyt_ClassInfo);
  RpsClassInfo_t *clinf = (RpsClassInfo_t *) payl;
  RPS_ASSERT (clinf->pclass_magic == RPS_CLASSINFO_MAGIC);
  if (clinf->pclass_super)
    rps_dumper_scan_object (du, clinf->pclass_super);
  if (clinf->pclass_methdict)
    {
      RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (clinf->pclass_methdict) ==
		  -RpsPyt_AttrTable);
      rps_attr_table_dump_scan (du, clinf->pclass_methdict, 0);
    }
  if (clinf->pclass_symbol)
    rps_dumper_scan_object (du, clinf->pclass_symbol);
}				/* end rps_classinfo_payload_dump_scanner */

void
rps_classinfo_payload_dump_serializer (RpsDumper_t * du,
				       struct rps_owned_payload_st *payl,
				       json_t * json, void *data)
{
  // should be compatible with rpsldpy_classinfo
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (payl && rps_zoned_memory_type (payl) == -RpsPyt_ClassInfo);
  RpsClassInfo_t *clinf = (RpsClassInfo_t *) payl;
  RPS_ASSERT (clinf->pclass_magic == RPS_CLASSINFO_MAGIC);
  RpsObject_t *classob = clinf->payl_owner;
  RPS_ASSERT (rps_is_valid_object (classob));
  RPS_DEBUG_PRINTF (DUMP, "serializing classinfo classob %-1O", classob);
  json_object_set (json, "payload", json_string ("classinfo"));
  RpsObject_t *superob = clinf->pclass_super;
  json_object_set (json, "class_super",
		   rps_dump_json_for_object (du, superob));
  RpsObject_t *symbob = clinf->pclass_symbol;
  json_object_set (json, "class_symb", rps_dump_json_for_object (du, symbob));
  {
    pthread_mutex_lock (&symbob->ob_mtx);
    RpsSymbol_t *paylsycla = rps_get_object_payload_of_type (symbob,
							     -RpsPyt_Symbol);
    if (paylsycla)
      {
	const RpsString_t *syname = paylsycla->symb_name;
	if (rps_value_type ((RpsValue_t) syname) == RPS_TYPE_STRING)
	  {
	    json_t *jname =	//
	      json_string (rps_stringv_utf8bytes ((RpsValue_t) syname));
	    json_object_set (json, "class_name", jname);
	  }
      };
    pthread_mutex_unlock (&symbob->ob_mtx);
  }
  RpsAttrTable_t *methdict = clinf->pclass_methdict;
  if (methdict)
    {
      const RpsSetOb_t *setattr = rps_attr_table_set_of_attributes (methdict);
      unsigned nbattr = rps_set_cardinal (setattr);
      json_t *jarrmeth = json_array ();
      for (int aix = 0; aix < (int) nbattr; aix++)
	{
	  const RpsObject_t *curselob = rps_set_nth_member (setattr, aix);
	  RPS_ASSERT (rps_is_valid_object ((RpsObject_t *) curselob));
	  if (rps_is_dumpable_object (du, (RpsObject_t *) curselob))
	    {
	      RpsValue_t curmethv =	//
		rps_attr_table_find (methdict, (RpsObject_t *) curselob);
	      RPS_DEBUG_PRINTF (DUMP,
				"classob %-1O curselob %-1O curmethv %V",
				classob, curselob, curmethv);
	      if (rps_value_type (curmethv) == RPS_TYPE_CLOSURE
		  && rps_is_dumpable_value (du, curmethv))
		{
		  json_t *jent = json_object ();
		  json_object_set (jent, "methosel",
				   rps_dump_json_for_object (du, curselob));
		  json_object_set (jent, "methclos",
				   rps_dump_json_for_value (du, curmethv, 0));
		  json_array_append_new (jarrmeth, jent);
		}
	    }
	}
      json_object_set (json, "class_methodict", jarrmeth);
    }
}				/* end rps_classinfo_payload_dump_serializer  */



/// rps_symbol_payload_remover is a rps_payload_remover_t for symbol
/// it has been registered (in main) by rps_register_payload_removal
void
rps_symbol_payload_remover (RpsObject_t * ob,
			    struct rps_owned_payload_st *clpayl, void *data)
{
  /// the ob has been locked...
  RPS_ASSERT (ob && rps_is_valid_object (ob));
  RPS_ASSERT (clpayl && rps_zoned_memory_type (clpayl) == -RpsPyt_Symbol);
  RpsSymbol_t *symb = (RpsSymbol_t *) clpayl;
  symb->payl_owner = NULL;
  symb->symb_name = NULL;	// will be garbage collected.
  symb->symb_value = RPS_NULL_VALUE;
#warning rps_symbol_payload_remover need a code review
  /// TODO: should we also clear the zm_length, zm_xtra fields?
}				/* end rps_classinfo_payload_remover */


void
rps_symbol_payload_dump_scanner (RpsDumper_t * du,
				 struct rps_owned_payload_st *payl,
				 void *data)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (payl && rps_zoned_memory_type (payl) == -RpsPyt_Symbol);
  RPS_ASSERT (payl->payl_owner != NULL);
  RpsSymbol_t *symb = (RpsSymbol_t *) payl;
  if (symb->symb_name)
    rps_dumper_scan_value (du, (RpsValue_t) symb->symb_name, 0);
  if (symb->symb_value)
    rps_dumper_scan_value (du, symb->symb_value, 0);
}				/* end rps_symbol_payload_dump_scanner */

void
rps_symbol_payload_dump_serializer (RpsDumper_t * du,
				    struct rps_owned_payload_st *payl,
				    json_t * json, void *data)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  RPS_ASSERT (payl && rps_zoned_memory_type (payl) == -RpsPyt_Symbol);
  RPS_ASSERT (payl->payl_owner != NULL);
  json_object_set (json, "payload", json_string ("symbol"));
  RpsSymbol_t *symb = (RpsSymbol_t *) payl;
  if (symb->symb_name)
    {
      json_t *jsname =		//
	rps_dump_json_for_value (du, (RpsValue_t) symb->symb_name, 0);
      json_object_set (json, "symb_name", jsname);
    };
  if (symb->symb_value)
    {
      json_t *jssyval =		//
	rps_dump_json_for_value (du, (RpsValue_t) symb->symb_value, 0);
      json_object_set (json, "symb_val", jssyval);
    };
}				/* end rps_symbol_payload_dump_serializer  */



/*************** end of file object_rps.c ****************/
