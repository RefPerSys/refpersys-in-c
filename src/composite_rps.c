/****************************************************************
 * file composite_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It supports composite values.
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

#include "kavl.h"

const RpsTupleOb_t *
rps_alloc_tuple_sized (unsigned arity, RpsObject_t ** arr)
{
  RpsTupleOb_t *tup = NULL;
  RpsHash_t htup = 0;
  unsigned long h1 = 0, h2 = rps_prime_above (3 * arity + 5);
  if (arr == NULL && arity > 0)
    return NULL;
  tup =
    RPS_ALLOC_ZONE (sizeof (RpsTupleOb_t) + arity * sizeof (RpsObject_t *),
		    RPS_TYPE_TUPLE);
  for (int ix = 0; ix < (int) arity; ix++)
    {
      RpsObject_t *curob = arr[ix];
      if (curob && rps_is_valid_object (curob))
	{
	  RpsHash_t curhash = curob->zv_hash;
	  tup->tuple_comp[ix] = (RpsObject_t *) curob;
	  if (ix % 2 == 0)
	    {
	      unsigned oldh1 = h1;
	      h1 = ((32059 * h1) ^ (curhash * 32083)) + ix;
	      h2 = ((oldh1 << 11) ^ curhash) + ((h2 >> 17) * 321073);
	    }
	  else
	    {
	      unsigned oldh2 = h2;
	      h1 = (32009 * h1) ^ ((curhash * 52069) + oldh2 - ix);
	      h2 = (oldh2 % 152063) ^ ((curhash << 5) + (541 * h2));
	    }
	}
    }
  htup = h1 ^ h2;
  if (!htup)
    htup = rps_prime_above ((h1 & 0xfffff) + (h2 & 0xffffff));
  tup->zv_hash = htup;
  tup->zm_length = arity;
  return tup;
}				/* end of rps_alloc_tuple_sized */


const RpsTupleOb_t *
rps_alloc_vtuple (unsigned arity, ...)
{
  va_list arglist;
  const RpsTupleOb_t *tup = NULL;
  RpsObject_t **obarr =
    RPS_ALLOC_ZEROED ((arity + 1) * sizeof (RpsObject_t *));
  va_start (arglist, arity);
  for (int ix = 0; ix < (int) arity; ix++)
    {
      obarr[ix] = va_arg (arglist, RpsObject_t *);
    }
  va_end (arglist);
  tup = rps_alloc_tuple_sized (arity, obarr);
  free (obarr);
  return tup;
}				/* end rps_alloc_vtuple */

unsigned
rps_vtuple_size (const RpsTupleOb_t * tup)
{
  if (tup == NULL || rps_value_type ((RpsValue_t) tup) != RPS_TYPE_TUPLE)
    return 0;
  return tup->zm_length;
}				/* end rps_vtuple_size */

RpsObject_t *
rps_vtuple_nth (const RpsTupleOb_t * tup, int rk)
{
  if (tup == NULL || rps_value_type ((RpsValue_t) tup) != RPS_TYPE_TUPLE)
    return NULL;
  unsigned sz = tup->zm_length;
  if (rk < 0)
    rk += (int) sz;
  if (rk >= 0 && rk < sz)
    return tup->tuple_comp[rk];
  return NULL;
}				/* end rps_vtuple_nth */





const RpsSetOb_t *
rps_alloc_set_sized (unsigned nbcomp, const RpsObject_t ** arr)
{
  RpsSetOb_t *set = NULL;
  if (!arr && nbcomp > 0)
    return NULL;
  /// some of the objects in arr could be NULL
  const RpsObject_t **arrcpy =
    RPS_ALLOC_ZEROED ((nbcomp + 1) * sizeof (RpsObject_t *));
  int nbob = 0;
  for (int ix = 0; ix < nbcomp; ix++)
    if (arr[ix] && rps_is_valid_object (arr[ix]))
      arrcpy[nbob++] = arr[ix];
  rps_object_array_qsort (arrcpy, (int) nbob);
  int card = 0;
  for (int ix = 0; ix < (int) nbob - 1; ix++)
    if (arrcpy[ix + 1] != arrcpy[ix])
      card++;
  set =
    RPS_ALLOC_ZONE (sizeof (RpsSetOb_t) + (card * sizeof (RpsObject_t *)),
		    RPS_TYPE_SET);
  set->zm_length = card;
  if (card > 0)
    {
      int setix = 0;
      for (int ix = 0; ix < (int) nbob - 1; ix++)
	{
	  if (arrcpy[ix + 1] != arrcpy[ix] && arrcpy[ix])
	    set->set_elem[setix++] = arrcpy[ix];
	};
      if (nbob > 1 && arrcpy[nbob - 1] != arrcpy[nbob - 2])
	set->set_elem[setix++] = arrcpy[nbob - 1];
      RPS_ASSERT (card == setix - 1);
    }
  free (arrcpy);
  return set;
}				/* end rps_alloc_set_sized */



const RpsSetOb_t *
rps_alloc_vset (unsigned card, /*objects */ ...)
{
  const RpsSetOb_t *set = NULL;
  va_list arglist;
  va_start (arglist, card);
  RpsObject_t **arrcpy =
    RPS_ALLOC_ZEROED ((card + 1) * sizeof (RpsObject_t *));
  for (int ix = 0; ix < (int) card; ix++)
    {
      arrcpy[ix] = va_arg (arglist, RpsObject_t *);
    }
  va_end (arglist);
  set = rps_alloc_set_sized (card, (const RpsObject_t **) arrcpy);
  free (arrcpy);
  return set;
}				/* end rps_alloc_vset */

int
rps_set_index (const RpsSetOb_t * setv, const RpsObject_t * ob)
{
  if (!setv || !ob)
    return -1;
  if (rps_value_type ((RpsValue_t) setv) != RPS_TYPE_SET)
    return -1;
  if (!rps_is_valid_object (ob))
    return -1;
  unsigned card = setv->zm_length;
  int lo = 0, hi = (int) card - 1;
  while (lo + 4 < hi)
    {
      int md = (lo + hi) / 2;
      const RpsObject_t *obmidelem = setv->set_elem[md];
      if (obmidelem == ob)
	return md;
      RPS_ASSERT (obmidelem && rps_is_valid_object (obmidelem));
      if (rps_object_less (ob, obmidelem))
	hi = md;
      else
	lo = md;
    }
  for (int ix = lo; ix < hi; ix++)
    {
      const RpsObject_t *obmidelem = setv->set_elem[ix];
      RPS_ASSERT (obmidelem && rps_is_valid_object (obmidelem));
      if (obmidelem == ob)
	return ix;
    };
  return -1;
}				/* end rps_set_index */

bool
rps_set_contains (const RpsSetOb_t * setv, const RpsObject_t * ob)
{
  return rps_set_index (setv, ob) >= 0;
}				/* end rps_set_index */

unsigned
rps_set_cardinal (const RpsSetOb_t * setv)
{
  if (rps_value_type ((RpsValue_t) setv) != RPS_TYPE_SET)
    return 0;
  unsigned card = setv->zm_length;
  return card;
}				/* end rps_set_cardinal */

const RpsObject_t *
rps_set_nth_member (const RpsSetOb_t * setv, int n)
{
  if (rps_value_type ((RpsValue_t) setv) != RPS_TYPE_SET)
    return NULL;
  unsigned card = setv->zm_length;
  if (n < 0)
    n += card;
  if (n >= 0 && n < card)
    return setv->set_elem[n];
  return NULL;
}				/* end rps_set_nth_member */

const RpsClosure_t *
rps_closure_array_make (RpsObject_t * conn, RpsValue_t meta, unsigned arity,
			RpsValue_t * cvalarr)
{
  RpsClosure_t *clos = NULL;
  if (conn == NULL || !rps_is_valid_object (conn))
    return NULL;
  RPS_ASSERT (arity == 0 || cvalarr != NULL);
  RPS_ASSERT (arity < RPS_CLOSURE_MAX_NB_VALUE);
  unsigned size = rps_prime_above (arity);
  int prix = rps_index_of_prime (size);
  clos =
    RPS_ALLOC_ZONE (sizeof (RpsClosure_t) + size * sizeof (RpsValue_t),
		    RPS_TYPE_SET);
  clos->zm_length = arity;
  clos->zm_xtra = prix;
  clos->clos_conn = conn;
  clos->clos_meta = meta;
  memcpy (clos->clos_val, cvalarr, arity * sizeof (RpsValue_t));
  return clos;
}				/* end rps_closure_array_make */

#define RPS_SMALL_SIZE 64
const RpsClosure_t *
rps_closure_make (RpsObject_t * conn, unsigned arity, ...)
{
  va_list arglist;
  const RpsClosure_t *clos = NULL;
  if (!conn)
    return NULL;
  va_start (arglist, arity);
  RPS_ASSERT (arity < RPS_CLOSURE_MAX_NB_VALUE);
  RpsValue_t *arr = NULL;
  if (arity < RPS_SMALL_SIZE)
    {
      RpsValue_t smallarr[RPS_SMALL_SIZE];
      memset (smallarr, 0, sizeof (smallarr));
      for (int ix = 0; ix < (int) arity; ix++)
	smallarr[ix] = va_arg (arglist, RpsValue_t);
      clos = rps_closure_array_make (conn, arity, RPS_NULL_VALUE, smallarr);
    }
  else
    {
      arr = RPS_ALLOC_ZEROED (sizeof (RpsValue_t) * (arity + 1));
      for (int ix = 0; ix < (int) arity; ix++)
	arr[ix] = va_arg (arglist, RpsValue_t);
      clos = rps_closure_array_make (conn, arity, RPS_NULL_VALUE, arr);
      free (arr);
    }
  va_end (arglist);
  return clos;
}				/* end rps_closure_array_make */

const RpsClosure_t *
rps_closure_meta_make (RpsObject_t * conn, RpsValue_t meta, unsigned arity,
		       ...)
{
  va_list arglist;
  const RpsClosure_t *clos = NULL;
  if (!conn)
    return NULL;
  va_start (arglist, arity);
  RPS_ASSERT (arity < RPS_CLOSURE_MAX_NB_VALUE);
  RpsValue_t *arr = NULL;
  if (arity < RPS_SMALL_SIZE)
    {
      RpsValue_t smallarr[RPS_SMALL_SIZE];
      memset (smallarr, 0, sizeof (smallarr));
      for (int ix = 0; ix < (int) arity; ix++)
	smallarr[ix] = va_arg (arglist, RpsValue_t);
      clos = rps_closure_array_make (conn, arity, RPS_NULL_VALUE, smallarr);
    }
  else
    {
      arr = RPS_ALLOC_ZEROED (sizeof (RpsValue_t) * (arity + 1));
      for (int ix = 0; ix < (int) arity; ix++)
	arr[ix] = va_arg (arglist, RpsValue_t);
      clos = rps_closure_array_make (conn, arity, RPS_NULL_VALUE, arr);
      free (arr);
    }
  va_end (arglist);
  return clos;
}				/* end rps_closure_meta_make */




RpsValue_t
rps_closure_apply_v (rps_callframe_t * callerframe, const RpsClosure_t * clos,
		     RpsValue_t arg0, RpsValue_t arg1, RpsValue_t arg2,
		     RpsValue_t arg3)
{
  RPS_ASSERT (callerframe == NULL || callerframe->calfr_descr == NULL
	      || callerframe->calfr_descr->calfrd_magic == RPS_CALLFRD_MAGIC);
  if (!clos || rps_value_type ((RpsValue_t) clos) != RPS_TYPE_CLOSURE)
    return RPS_NULL_VALUE;
  RpsObject_t *obconn = clos->clos_conn;
  if (!obconn || !rps_is_valid_object (obconn))
    return RPS_NULL_VALUE;
  RpsObject_t *obsig = NULL;
  void *routaddr = NULL;
  pthread_mutex_lock (&obconn->ob_mtx);
  obsig = obconn->ob_routsig;
  routaddr = obconn->ob_routaddr;
  pthread_mutex_unlock (&obconn->ob_mtx);
  if (!routaddr)
    return RPS_NULL_VALUE;
  /* We should check obsig and use routaddr suitably casted to (rps_apply_v_sigt*) */
#warning rps_closure_apply_v should check obsig
  /* A good optimizing GCC compiler would make a tail-recursive call. */
  return (*(rps_apply_v_sigt *) routaddr) (callerframe, clos, arg0, arg1,
					   arg2, arg3);
}				/* end rps_closure_apply_v */

RpsValueAndInt
rps_closure_apply_vi (rps_callframe_t * callerframe,
		      const RpsClosure_t * clos, RpsValue_t arg0,
		      RpsValue_t arg1, RpsValue_t arg2, RpsValue_t arg3)
{
  RPS_ASSERT (callerframe == NULL || callerframe->calfr_descr == NULL
	      || callerframe->calfr_descr->calfrd_magic == RPS_CALLFRD_MAGIC);
  if (!clos || rps_value_type ((RpsValue_t) clos) != RPS_TYPE_CLOSURE)
    return (RpsValueAndInt)
    {
    RPS_NULL_VALUE, 0};
  RpsObject_t *obconn = clos->clos_conn;
  if (!obconn || !rps_is_valid_object (obconn))
    return (RpsValueAndInt)
    {
    RPS_NULL_VALUE, 0};
  RpsObject_t *obsig = NULL;
  void *routaddr = NULL;
  pthread_mutex_lock (&obconn->ob_mtx);
  obsig = obconn->ob_routsig;
  routaddr = obconn->ob_routaddr;
  pthread_mutex_unlock (&obconn->ob_mtx);
  if (!routaddr)
    return (RpsValueAndInt)
    {
    RPS_NULL_VALUE, 0};
  /* We should check obsig and use routaddr suitably casted to (rps_apply_vi_sigt*) */
#warning rps_closure_apply_vi should check obsig
  /* A good optimizing GCC compiler would make a tail-recursive call. */
  return (*(rps_apply_vi_sigt *) routaddr) (callerframe, clos, arg0, arg1,
					    arg2, arg3);
}				/* end rps_closure_apply_vi */

RpsTwoValues
rps_closure_apply_twov (rps_callframe_t * callerframe,
			const RpsClosure_t * clos, RpsValue_t arg0,
			RpsValue_t arg1, RpsValue_t arg2, RpsValue_t arg3)
{
  RPS_ASSERT (callerframe == NULL || callerframe->calfr_descr == NULL
	      || callerframe->calfr_descr->calfrd_magic == RPS_CALLFRD_MAGIC);
  if (!clos || rps_value_type ((RpsValue_t) clos) != RPS_TYPE_CLOSURE)
    return (RpsTwoValues)
    {
    RPS_NULL_VALUE, RPS_NULL_VALUE};
  RpsObject_t *obconn = clos->clos_conn;
  if (!obconn || !rps_is_valid_object (obconn))
    return (RpsTwoValues)
    {
    RPS_NULL_VALUE, RPS_NULL_VALUE};
  RpsObject_t *obsig = NULL;
  void *routaddr = NULL;
  pthread_mutex_lock (&obconn->ob_mtx);
  obsig = obconn->ob_routsig;
  routaddr = obconn->ob_routaddr;
  pthread_mutex_unlock (&obconn->ob_mtx);
  if (!routaddr)
    return (RpsTwoValues)
    {
    RPS_NULL_VALUE, 0};
  /* We should check obsig and use routaddr suitably casted to (rps_apply_twov_sigt*) */
#warning rps_closure_apply_twov should check obsig
  /* A good optimizing GCC compiler would make a tail-recursive call. */
  return (*(rps_apply_twov_sigt *) routaddr) (callerframe, clos, arg0, arg1,
					      arg2, arg3);
}				/* end rps_closure_apply_twov */

/**** mutable set payload support *****/

static int
rps_mutable_set_ob_node_cmp (const struct internal_mutable_set_ob_node_rps_st
			     *left,
			     const struct internal_mutable_set_ob_node_rps_st
			     *right)
{
  RPS_ASSERT (left);
  RPS_ASSERT (right);
  if (left == right)
    return 0;
  const RpsObject_t *obleft = left->setobnodrps_obelem;
  const RpsObject_t *obright = right->setobnodrps_obelem;
  if (obleft == obright)
    return 0;
  if (obleft == NULL)
    return -1;
  if (obright == NULL)
    return 1;
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (obleft) == RPS_TYPE_OBJECT);
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (obright) == RPS_TYPE_OBJECT);
  return rps_oid_cmp (obleft->ob_id, obright->ob_id);
}				/* end rps_mutable_set_ob_node_cmp */

KAVL_INIT (rpsmusetob, struct internal_mutable_set_ob_node_rps_st,
	   setobnodrps_head, rps_mutable_set_ob_node_cmp);


/* This function returns true if `ob` was genuinely added into
   `paylmset`, and false otherwise, e.g. because it was already
   element of the mutable set `paylmset`. */
bool
rps_paylsetob_add_element (RpsMutableSetOb_t * paylmset,
			   const RpsObject_t * ob)
{
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (paylmset) == -RpsPyt_MutableSetOb);
  RPS_ASSERT (ob != NULL && rps_is_valid_object ((RpsObject_t *) ob));
  struct internal_mutable_set_ob_node_rps_st *newnod =
    RPS_ALLOC_ZEROED (sizeof (struct internal_mutable_set_ob_node_rps_st));
  newnod->setobnodrps_obelem = ob;
  struct internal_mutable_set_ob_node_rps_st *addednod =
    kavl_insert_rpsmusetob (&paylmset->muset_root, newnod, NULL);
  if (addednod == newnod)
    {
      paylmset->zm_length++;
      return true;
    }
  else
    {
      free (newnod);
      return false;
    }
}				/* end rps_paylsetob_add_element */


/* This function returns true if `ob` was genuinely removed into
   `paylmset`, and false otherwise, e.g. because it was not an element
   of the mutable set `paylmset`. */
bool
rps_paylsetob_remove_element (RpsMutableSetOb_t * paylmset,
			      const RpsObject_t * ob)
{
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (paylmset) == -RpsPyt_MutableSetOb);
  RPS_ASSERT (ob != NULL && rps_is_valid_object ((RpsObject_t *) ob));
  struct internal_mutable_set_ob_node_rps_st *newnod =
    RPS_ALLOC_ZEROED (sizeof (struct internal_mutable_set_ob_node_rps_st));
  newnod->setobnodrps_obelem = ob;
  struct internal_mutable_set_ob_node_rps_st *removednod =
    kavl_erase_rpsmusetob (&paylmset->muset_root, newnod, NULL);
  if (removednod)
    {
      free (removednod);
      free (newnod);
      paylmset->zm_length--;
      return true;
    }
  else
    {
      free (newnod);
      return false;
    };
}				/* end rps_paylsetob_remove_element */


/* loading mutable set of objects */
void
rpsldpy_setob (RpsObject_t * obj, RpsLoader_t * ld, const json_t * jv,
	       int spix)
{
  RpsMutableSetOb_t *paylsetob = NULL;
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_filling_loader (ld));
  paylsetob = RPS_ALLOC_ZONE (sizeof (RpsMutableSetOb_t),
			      -RpsPyt_MutableSetOb);
  json_t *jssetob = json_object_get (jv, "setob");
  if (jssetob && json_is_array (jssetob))
    {
      int card = (int) json_array_size (jssetob);
      for (int ix = 0; ix < card; ix++)
	{
	  json_t *jcurelem = json_array_get (jssetob, ix);
	  RpsObject_t *elemob = rps_loader_json_to_object (ld, jcurelem);
	  RPS_ASSERT (elemob != NULL);
	  if (!rps_paylsetob_add_element (paylsetob, elemob))
	    RPS_FATAL ("corrupted already element element#%d for json %s",
		       ix, json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
	};
    }
  rps_object_put_payload (obj, paylsetob);
}				/* end rpsldpy_setob */


void
rps_object_mutable_set_initialize (RpsObject_t * obj)
{
  RpsMutableSetOb_t *paylsetob = NULL;
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_object (obj));
  pthread_mutex_lock (&obj->ob_mtx);
  paylsetob = RPS_ALLOC_ZONE (sizeof (RpsMutableSetOb_t),
			      -RpsPyt_MutableSetOb);
  rps_object_put_payload (obj, paylsetob);
  pthread_mutex_unlock (&obj->ob_mtx);
}				/* end rps_object_mutable_set_initialize */

void
rps_object_mutable_set_add (RpsObject_t * obj, RpsValue_t val)
{
  RpsMutableSetOb_t *paylsetob = NULL;
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_object (obj));
  if (!val || val == RPS_NULL_VALUE || (val & 1))
    return;
  enum RpsType vtyp = rps_value_type (val);
  pthread_mutex_lock (&obj->ob_mtx);
  if (!obj->ob_payload)
    goto end;
  if (RPS_ZONED_MEMORY_TYPE (obj->ob_payload) == -RpsPyt_MutableSetOb)
    paylsetob = (RpsMutableSetOb_t *) (obj->ob_payload);
  else
    goto end;
  switch (vtyp)
    {
    case RPS_TYPE_TUPLE:
      {
	const RpsTupleOb_t *tup = (const RpsTupleOb_t *) val;
	unsigned sz = tup->zm_length;
	for (int ix = 0; ix < (int) sz; ix++)
	  if (tup->tuple_comp[ix] != NULL)
	    (void) rps_paylsetob_add_element (paylsetob, tup->tuple_comp[ix]);
      }
      break;
    case RPS_TYPE_SET:
      {
	const RpsSetOb_t *set = (const RpsSetOb_t *) val;
	unsigned sz = set->zm_length;
	for (int ix = 0; ix < (int) sz; ix++)
	  (void) rps_paylsetob_add_element (paylsetob, set->set_elem[ix]);
      }
      break;
    case RPS_TYPE_OBJECT:
      (void) rps_paylsetob_add_element (paylsetob, (RpsObject_t *) val);
      break;
    default:
      goto end;
    }
end:
  pthread_mutex_unlock (&obj->ob_mtx);
}				/* end rps_object_mutable_set_add */

const RpsSetOb_t *
rps_object_mutable_set_reify (RpsObject_t * obj)
{
  const RpsSetOb_t *vset = NULL;
  RpsMutableSetOb_t *paylsetob = NULL;
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_object (obj));
  pthread_mutex_lock (&obj->ob_mtx);
  if (!obj->ob_payload)
    goto end;
  if (RPS_ZONED_MEMORY_TYPE (obj->ob_payload) == -RpsPyt_MutableSetOb)
    paylsetob = (RpsMutableSetOb_t *) (obj->ob_payload);
  else
    goto end;
  unsigned card = paylsetob->zm_length;
  const RpsObject_t **arrob =
    RPS_ALLOC_ZEROED ((card + 1) * sizeof (RpsObject_t *));
  struct kavl_itr_rpsmusetob iter = { };
  int ix = 0;
  kavl_itr_first_rpsmusetob (paylsetob->muset_root, &iter);
  while (ix < (int) card)
    {
      arrob[ix++] = kavl_at (&iter)->setobnodrps_obelem;
      if (!kavl_itr_next_rpsmusetob (&iter))
	break;
    };
  RPS_ASSERT (ix == card);
  // this is inefficient, since sorting a second time an already
  // sorted array, but we don't care, since we hope to generate better
  // C code....
  vset = rps_alloc_set_sized (card, arrob);
  free (arrob);
end:
  pthread_mutex_unlock (&obj->ob_mtx);
  return vset;
}				/* end rps_object_mutable_set_reify */

void
rps_object_mutable_set_remove (RpsObject_t * obj, RpsValue_t val)
{
  RpsMutableSetOb_t *paylsetob = NULL;
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_object (obj));
  if (!val || val == RPS_NULL_VALUE || (val & 1))
    return;
  enum RpsType vtyp = rps_value_type (val);
  pthread_mutex_lock (&obj->ob_mtx);
  if (!obj->ob_payload)
    goto end;
  if (RPS_ZONED_MEMORY_TYPE (obj->ob_payload) == -RpsPyt_MutableSetOb)
    paylsetob = (RpsMutableSetOb_t *) (obj->ob_payload);
  else
    goto end;
  switch (vtyp)
    {
    case RPS_TYPE_TUPLE:
      {
	const RpsTupleOb_t *tup = (const RpsTupleOb_t *) val;
	unsigned sz = tup->zm_length;
	for (int ix = 0; ix < (int) sz; ix++)
	  if (tup->tuple_comp[ix] != NULL)
	    (void) rps_paylsetob_remove_element (paylsetob,
						 tup->tuple_comp[ix]);
      }
      break;
    case RPS_TYPE_SET:
      {
	const RpsSetOb_t *set = (const RpsSetOb_t *) val;
	unsigned sz = set->zm_length;
	for (int ix = 0; ix < (int) sz; ix++)
	  (void) rps_paylsetob_remove_element (paylsetob, set->set_elem[ix]);
      }
      break;
    case RPS_TYPE_OBJECT:
      (void) rps_paylsetob_remove_element (paylsetob, (RpsObject_t *) val);
      break;
    default:
      goto end;
    }
end:
  pthread_mutex_unlock (&obj->ob_mtx);
}				/* end rps_object_mutable_set_remove */





/*****************************************************************
 * string dictionnary payload
 ****************************************************************/

static int
rps_string_dict_node_cmp (const struct internal_string_dict_node_rps_st *left,
			  const struct internal_string_dict_node_rps_st
			  *right)
{

  RPS_ASSERT (left);
  RPS_ASSERT (right);
  if (left == right)
    return 0;
  const RpsString_t *strleft = left->strdicnodrps_name;
  const RpsString_t *strright = right->strdicnodrps_name;
  if (strleft == strright)
    return 0;
  return strcmp (strleft->cstr, strright->cstr);
}				/* end rps_string_dict_node_cmp */

KAVL_INIT (strdicnodrps, struct internal_string_dict_node_rps_st,
	   strdicnodrps_head, rps_string_dict_node_cmp);


void
rps_object_string_dictionary_initialize (RpsObject_t * ob)
{
  RPS_ASSERT (rps_is_valid_object (ob));
  RpsStringDictOb_t *paylstrdic = RPS_ALLOC_ZONE (sizeof (RpsStringDictOb_t),
						  -RpsPyt_StringDict);
  rps_object_put_payload (ob, paylstrdic);
}				/* end rps_object_string_dictionary_initialize */


void
rps_payl_string_dictionary_add_cstr (RpsStringDictOb_t * paylstrdic,
				     const char *cstr, const RpsValue_t val)
{
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (paylstrdic) == -RpsPyt_StringDict);
  RPS_ASSERT (cstr && g_utf8_validate (cstr, -1, NULL));
  RPS_ASSERT (val != RPS_NULL_VALUE);
  const RpsString_t *strv = rps_alloc_string (cstr);
  struct internal_string_dict_node_rps_st *newnod =
    RPS_ALLOC_ZEROED (sizeof (struct internal_string_dict_node_rps_st));
  newnod->strdicnodrps_name = strv;
  struct internal_string_dict_node_rps_st *addednod =
    kavl_insert_strdicnodrps (&paylstrdic->strdict_root, newnod, NULL);
  if (addednod == newnod)
    {
      paylstrdic->zm_length++;
      newnod->strdicnodrps_val = val;
    }
  else
    addednod->strdicnodrps_val = val;
}				/* end rps_payl_string_dictionary_add_cstr */


void
rps_payl_string_dictionary_add_valstr (RpsStringDictOb_t * paylstrdic,
				       const RpsString_t * strv,
				       const RpsValue_t val)
{
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (paylstrdic) == -RpsPyt_StringDict);
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (strv) == RPS_TYPE_STRING);
  RPS_ASSERT (val != RPS_NULL_VALUE);
  struct internal_string_dict_node_rps_st *newnod =
    RPS_ALLOC_ZEROED (sizeof (struct internal_string_dict_node_rps_st));
  newnod->strdicnodrps_name = strv;
  struct internal_string_dict_node_rps_st *addednod =
    kavl_insert_strdicnodrps (&paylstrdic->strdict_root, newnod, NULL);
  if (addednod == newnod)
    {
      paylstrdic->zm_length++;
      newnod->strdicnodrps_val = val;
    }
  else
    addednod->strdicnodrps_val = val;
}				/* end rps_payl_string_dictionary_add_valstr */


/* loading a payload associating strings to values */
void
rpsldpy_string_dictionary (RpsObject_t * obj, RpsLoader_t * ld,
			   const json_t * jv, int spix)
{
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_filling_loader (ld));
  char idbuf[32];
  memset (idbuf, 0, sizeof (idbuf));
  rps_oid_to_cbuf (obj->ob_id, idbuf);
  pthread_mutex_lock (&obj->ob_mtx);
  rps_object_string_dictionary_initialize (obj);
  RpsStringDictOb_t *paylstrdic = (RpsStringDictOb_t *) (obj->ob_payload);
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (paylstrdic) == -RpsPyt_StringDict);
  json_t *jsdict = json_object_get (jv, "dictionary");
  if (jsdict && json_is_array (jsdict))
    {
      unsigned nbent = json_array_size (jsdict);
      for (int ix = 0; ix < (int) nbent; ix++)
	{
	  json_t *jent = json_array_get (jsdict, ix);
	  if (!json_is_object (jent))
	    continue;
	  json_t *jstr = json_object_get (jent, "str");
	  if (json_is_string (jstr))
	    continue;
	  json_t *jval = json_object_get (jent, "val");
	  if (!jval)
	    continue;
	  const char *str = json_string_value (jstr);
	  RpsValue_t curval = rps_loader_json_to_value (ld, jval);
	  rps_payl_string_dictionary_add_cstr (paylstrdic, str, curval);
	}
    }
end:
  pthread_mutex_unlock (&obj->ob_mtx);
}				/* end rpsldpy_string_dictionary */


/* loading a payload for space */
void
rpsldpy_space (RpsObject_t * obj, RpsLoader_t * ld,
	       const json_t * jv, int spix)
{
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_filling_loader (ld));
  char idbuf[32];
  memset (idbuf, 0, sizeof (idbuf));
  rps_oid_to_cbuf (obj->ob_id, idbuf);
  pthread_mutex_lock (&obj->ob_mtx);
  RpsSpace_t *paylspace = RPS_ALLOC_ZONE (sizeof (RpsSpace_t), -RpsPyt_Space);
  json_t *jsspdata = json_object_get (jv, "space_data");
  if (jsspdata)
    paylspace->space_data = rps_loader_json_to_value (ld, jsspdata);
  rps_object_put_payload (obj, paylspace);
end:
  pthread_mutex_unlock (&obj->ob_mtx);
}				/* end rpsldpy_space */


/****************************************************************
 * Double-ended queue/linked-list payload for -RpsPyt_DequeOb
 ****************************************************************/
void
rps_object_deque_ob_initialize (RpsObject_t * obj)
{
  if (!obj)
    return;
  RPS_ASSERT (rps_is_valid_object (obj));
  char idbuf[32];
  memset (idbuf, 0, sizeof (idbuf));
  rps_oid_to_cbuf (obj->ob_id, idbuf);
  pthread_mutex_lock (&obj->ob_mtx);
  RpsDequeOb_t *payldeq =
    RPS_ALLOC_ZONE (sizeof (RpsDequeOb_t), -RpsPyt_DequeOb);
  rps_object_put_payload (obj, payldeq);
end:
  pthread_mutex_unlock (&obj->ob_mtx);
}				/* end of rps_object_deque_ob_initialize */

RpsDequeOb_t *
rps_deque_for_dumper (RpsDumper_t * du)
{
  RPS_ASSERT (rps_is_valid_dumper (du));
  RpsDequeOb_t *payldeq =
    RPS_ALLOC_ZONE (sizeof (RpsDequeOb_t), -RpsPyt_DequeOb);
  return payldeq;
}				/* end rps_deque_for_dumper */


RpsObject_t *
rps_payldeque_get_first (RpsDequeOb_t * payldeq)
{
  RpsObject_t *resob = NULL;
  if (!payldeq || RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    return NULL;
  struct rps_dequeob_link_st *firstlink = payldeq->deqob_first;
  if (!firstlink)
    return NULL;
  RPS_ASSERT (firstlink->dequeob_prev == NULL);
  for (int i = 0; i < RPS_DEQUE_CHUNKSIZE; i++)
    {
      resob = firstlink->dequeob_chunk[i];
      if (resob)
	break;
    }
  return resob;
}				/* end rps_payldeque_get_first */

RpsObject_t *
rps_object_deque_get_first (RpsObject_t * obj)
{
  RpsObject_t *resob = NULL;
  if (!obj)
    return NULL;
  RPS_ASSERT (rps_is_valid_object (obj));
  pthread_mutex_lock (&obj->ob_mtx);
  RpsDequeOb_t *payldeq = (RpsDequeOb_t *) obj->ob_payload;
  if (!payldeq || RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    goto end;
  struct rps_dequeob_link_st *firstlink = payldeq->deqob_first;
  if (!firstlink)
    goto end;
  RPS_ASSERT (firstlink->dequeob_prev == NULL);
  for (int i = 0; i < RPS_DEQUE_CHUNKSIZE; i++)
    {
      resob = firstlink->dequeob_chunk[i];
      if (resob)
	break;
    }
  RPS_ASSERT (resob != NULL);
end:
  pthread_mutex_unlock (&obj->ob_mtx);
  return resob;
}				/* end rps_object_deque_get_first */


RpsObject_t *
rps_payldeque_pop_first (RpsDequeOb_t * payldeq)
{
  RpsObject_t *resob = NULL;
  if (!payldeq || RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    return NULL;
#warning rps_payldeque_pop_first is probably buggy. Please code review
  struct rps_dequeob_link_st *firstlink = payldeq->deqob_first;
  if (!firstlink)
    goto end;
  RPS_ASSERT (payldeq->zm_length > 0);
  RPS_ASSERT (firstlink->dequeob_prev == NULL);
  for (int i = 0; i < RPS_DEQUE_CHUNKSIZE; i++)
    {
      resob = firstlink->dequeob_chunk[i];
      if (resob)
	{
	  firstlink->dequeob_chunk[i] = NULL;
	  payldeq->zm_length--;
	  if (i == RPS_DEQUE_CHUNKSIZE - 1)
	    {
	      payldeq->deqob_first = firstlink->dequeob_next;
	      if (!payldeq->deqob_first)
		{
		  RPS_ASSERT (payldeq->deqob_last == firstlink);
		  payldeq->deqob_last = NULL;
		}
	      free (firstlink);
	    }
	  break;
	}
    }
end:
  return resob;
}				/* end rps_payldeque_pop_first */

RpsObject_t *
rps_object_deque_pop_first (RpsObject_t * obj)
{
  RpsObject_t *resob = NULL;
  if (!obj)
    return NULL;
  RPS_ASSERT (rps_is_valid_object (obj));
  pthread_mutex_lock (&obj->ob_mtx);
  RpsDequeOb_t *payldeq = (RpsDequeOb_t *) obj->ob_payload;
  if (RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    goto end;
  struct rps_dequeob_link_st *firstlink = payldeq->deqob_first;
  if (!firstlink)
    goto end;
  RPS_ASSERT (payldeq->zm_length > 0);
  RPS_ASSERT (firstlink->dequeob_prev == NULL);
  for (int i = 0; i < RPS_DEQUE_CHUNKSIZE; i++)
    {
      resob = firstlink->dequeob_chunk[i];
      if (resob)
	{
	  firstlink->dequeob_chunk[i] = NULL;
	  payldeq->zm_length--;
	  if (i == RPS_DEQUE_CHUNKSIZE - 1)
	    {
	      payldeq->deqob_first = firstlink->dequeob_next;
	      if (!payldeq->deqob_first)
		{
		  RPS_ASSERT (payldeq->deqob_last == firstlink);
		  payldeq->deqob_last = NULL;
		}
	      free (firstlink);
	    }
	  break;
	}
    }
  RPS_ASSERT (resob != NULL);
end:
  pthread_mutex_unlock (&obj->ob_mtx);
  return resob;
}				/* end rps_object_deque_pop_first */

bool
rps_object_deque_push_first (RpsObject_t * obq, RpsObject_t * obelem)
{
  bool pushed = false;
  if (!obq)
    return false;
  if (!obelem)
    return false;
  RPS_ASSERT (rps_is_valid_object (obq));
  RPS_ASSERT (rps_is_valid_object (obelem));
  pthread_mutex_lock (&obq->ob_mtx);
  RpsDequeOb_t *payldeq = (RpsDequeOb_t *) obq->ob_payload;
  if (RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    goto end;
  struct rps_dequeob_link_st *firstlink = payldeq->deqob_first;
  if (!firstlink)
    {
      RPS_ASSERT (payldeq->zm_length == 0);
      struct rps_dequeob_link_st *newlink =	//
	RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
      newlink->dequeob_chunk[0] = obelem;
      payldeq->deqob_first = payldeq->deqob_last = newlink;
      payldeq->zm_length = 1;
      pushed = true;
      goto end;
    }
  int firstlcnt = 0;
  RpsObject_t *chunkarr[RPS_DEQUE_CHUNKSIZE];
  for (int ix = 0; ix < RPS_DEQUE_CHUNKSIZE; ix++)
    if (firstlink->dequeob_chunk[ix] != 0)
      chunkarr[firstlcnt++] = firstlink->dequeob_chunk[ix];
  if (firstlcnt == RPS_DEQUE_CHUNKSIZE)
    {
      // the chunk is full, we need to allocate a new one
      struct rps_dequeob_link_st *newlink =	//
	RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
      newlink->dequeob_chunk[0] = obelem;
      struct rps_dequeob_link_st *oldfirstlink = payldeq->deqob_first;
      RPS_ASSERT (oldfirstlink->dequeob_prev == NULL);
      oldfirstlink->dequeob_prev = newlink;
      newlink->dequeob_next = oldfirstlink;
      payldeq->deqob_first = newlink;
      payldeq->zm_length++;
      pushed = true;
      goto end;
    }
  else
    {
      firstlink->dequeob_chunk[0] = obelem;
      memcpy (firstlink->dequeob_chunk + 1, chunkarr,
	      firstlcnt * sizeof (RpsObject_t *));
      pushed = true;
      goto end;
    }
end:
  pthread_mutex_unlock (&obq->ob_mtx);
  return pushed;
}				/* end rps_object_deque_push_first */

bool
rps_payldeque_push_first (RpsDequeOb_t * deq, RpsObject_t * obelem)
{
  bool pushed = false;
  if (!deq || RPS_ZONED_MEMORY_TYPE (deq) != -RpsPyt_DequeOb)
    goto end;
  struct rps_dequeob_link_st *firstlink = deq->deqob_first;
  if (!firstlink)
    {
      RPS_ASSERT (deq->zm_length == 0);
      struct rps_dequeob_link_st *newlink =	//
	RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
      newlink->dequeob_chunk[0] = obelem;
      deq->deqob_first = deq->deqob_last = newlink;
      deq->zm_length = 1;
      pushed = true;
      goto end;
    }
  int firstlcnt = 0;
  RpsObject_t *chunkarr[RPS_DEQUE_CHUNKSIZE];
  for (int ix = 0; ix < RPS_DEQUE_CHUNKSIZE; ix++)
    if (firstlink->dequeob_chunk[ix] != 0)
      chunkarr[firstlcnt++] = firstlink->dequeob_chunk[ix];
  if (firstlcnt == RPS_DEQUE_CHUNKSIZE)
    {
      // the chunk is full, we need to allocate a new one
      struct rps_dequeob_link_st *newlink =	//
	RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
      newlink->dequeob_chunk[0] = obelem;
      struct rps_dequeob_link_st *oldfirstlink = deq->deqob_first;
      RPS_ASSERT (oldfirstlink->dequeob_prev == NULL);
      oldfirstlink->dequeob_prev = newlink;
      newlink->dequeob_next = oldfirstlink;
      deq->deqob_first = newlink;
      deq->zm_length++;
      pushed = true;
      goto end;
    }
  else
    {
      firstlink->dequeob_chunk[0] = obelem;
      memcpy (firstlink->dequeob_chunk + 1, chunkarr,
	      firstlcnt * sizeof (RpsObject_t *));
      pushed = true;
      goto end;
    }
end:
  return pushed;
}				/* end rps_payldeque_push_first */


RpsObject_t *
rps_object_deque_get_last (RpsObject_t * obj)
{
  RpsObject_t *resob = NULL;
  if (!obj)
    return NULL;
  RPS_ASSERT (rps_is_valid_object (obj));
  pthread_mutex_lock (&obj->ob_mtx);
  RpsDequeOb_t *payldeq = (RpsDequeOb_t *) obj->ob_payload;
  if (RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    goto end;
  struct rps_dequeob_link_st *lastlink = payldeq->deqob_last;
  if (!lastlink)
    goto end;
  RPS_ASSERT (lastlink->dequeob_next == NULL);
  for (int i = RPS_DEQUE_CHUNKSIZE - 1; i >= 0; i--)
    {
      resob = lastlink->dequeob_chunk[i];
      if (resob)
	break;
    }
  RPS_ASSERT (resob != NULL);
end:
  pthread_mutex_unlock (&obj->ob_mtx);
  return resob;
}				/* end rps_object_deque_get_last */

RpsObject_t *
rps_object_deque_pop_last (RpsObject_t * obj)
{
  RpsObject_t *resob = NULL;
  if (!obj)
    return NULL;
  RPS_ASSERT (rps_is_valid_object (obj));
  pthread_mutex_lock (&obj->ob_mtx);
  RpsDequeOb_t *payldeq = (RpsDequeOb_t *) obj->ob_payload;
  if (RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    goto end;
  struct rps_dequeob_link_st *lastlink = payldeq->deqob_last;
  if (!lastlink)
    goto end;
  RPS_ASSERT (lastlink->dequeob_next == NULL);
  for (int i = RPS_DEQUE_CHUNKSIZE - 1; i >= 0; i--)
    {
      resob = lastlink->dequeob_chunk[i];
      if (resob)
	{
	  if (i == 0)
	    {
	      payldeq->deqob_last = lastlink->dequeob_prev;
	      if (!payldeq->deqob_last)
		{
		  RPS_ASSERT (payldeq->deqob_first == lastlink);
		  payldeq->deqob_first = NULL;
		}
	      free (lastlink);
	    }
	  break;
	}
    }
  RPS_ASSERT (resob != NULL);
end:
  pthread_mutex_unlock (&obj->ob_mtx);
  return resob;
}				/* end rps_object_deque_pop_last */

bool
rps_object_deque_push_last (RpsObject_t * obq, RpsObject_t * obelem)
{
  bool pushed = false;
  if (!obq)
    return false;
  if (!obelem)
    return false;
  RPS_ASSERT (rps_is_valid_object (obq));
  RPS_ASSERT (rps_is_valid_object (obelem));
  pthread_mutex_lock (&obq->ob_mtx);
  RpsDequeOb_t *payldeq = (RpsDequeOb_t *) obq->ob_payload;
  if (RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    goto end;
  struct rps_dequeob_link_st *lastlink = payldeq->deqob_last;
  if (!lastlink)
    {
      RPS_ASSERT (payldeq->deqob_first == NULL);
      RPS_ASSERT (payldeq->zm_length == 0);
      struct rps_dequeob_link_st *newlink =	//
	RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
      newlink->dequeob_chunk[0] = obelem;
      payldeq->deqob_first = payldeq->deqob_last = newlink;
      payldeq->zm_length = 1;
      pushed = true;
      goto end;
    };
  RPS_ASSERT (lastlink->dequeob_next == NULL);
  int lastlcnt = 0;
  RpsObject_t *chunkarr[RPS_DEQUE_CHUNKSIZE];
  for (int ix = 0; ix < RPS_DEQUE_CHUNKSIZE; ix++)
    if (lastlink->dequeob_chunk[ix] != 0)
      chunkarr[lastlcnt++] = lastlink->dequeob_chunk[ix];
  if (lastlcnt < RPS_DEQUE_CHUNKSIZE)
    {
      if (lastlcnt > 0)
	memcpy (lastlink->dequeob_chunk, chunkarr,
		lastlcnt * sizeof (RpsObject_t *));
      lastlink->dequeob_chunk[lastlcnt] = obelem;
      payldeq->zm_length++;
      pushed = true;
      goto end;
    }
  struct rps_dequeob_link_st *newlink =	//
    RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
  lastlink->dequeob_next = newlink;
  newlink->dequeob_prev = lastlink;
  payldeq->deqob_last = newlink;
  newlink->dequeob_chunk[0] = obelem;
  payldeq->zm_length++;
  pushed = true;
  goto end;
end:
  pthread_mutex_unlock (&obq->ob_mtx);
  return pushed;
}				/* end rps_object_deque_push_last */

bool
rps_payldeque_push_last (RpsDequeOb_t * payldeq, RpsObject_t * obelem)
{
  bool pushed = false;
  if (!obelem)
    return false;
  if (!payldeq || RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    goto end;
  if (RPS_ZONED_MEMORY_TYPE (payldeq) != -RpsPyt_DequeOb)
    goto end;
  struct rps_dequeob_link_st *lastlink = payldeq->deqob_last;
  if (!lastlink)
    {
      RPS_ASSERT (payldeq->deqob_first == NULL);
      RPS_ASSERT (payldeq->zm_length == 0);
      struct rps_dequeob_link_st *newlink =	//
	RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
      newlink->dequeob_chunk[0] = obelem;
      payldeq->deqob_first = payldeq->deqob_last = newlink;
      payldeq->zm_length = 1;
      pushed = true;
      goto end;
    };
  RPS_ASSERT (lastlink->dequeob_next == NULL);
  int lastlcnt = 0;
  RpsObject_t *chunkarr[RPS_DEQUE_CHUNKSIZE];
  for (int ix = 0; ix < RPS_DEQUE_CHUNKSIZE; ix++)
    if (lastlink->dequeob_chunk[ix] != 0)
      chunkarr[lastlcnt++] = lastlink->dequeob_chunk[ix];
  if (lastlcnt < RPS_DEQUE_CHUNKSIZE)
    {
      if (lastlcnt > 0)
	memcpy (lastlink->dequeob_chunk, chunkarr,
		lastlcnt * sizeof (RpsObject_t *));
      lastlink->dequeob_chunk[lastlcnt] = obelem;
      payldeq->zm_length++;
      pushed = true;
      goto end;
    }
  struct rps_dequeob_link_st *newlink =	//
    RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
  lastlink->dequeob_next = newlink;
  newlink->dequeob_prev = lastlink;
  payldeq->deqob_last = newlink;
  newlink->dequeob_chunk[0] = obelem;
  payldeq->zm_length++;
  pushed = true;
  goto end;
end:
  return pushed;
}				/* end rps_payldeque_push_last */


pthread_mutex_t rps_rootob_mtx = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
RpsMutableSetOb_t rps_rootob_mutset = {.zm_atype = -RpsPyt_MutableSetOb };

void
rps_add_global_root_object (RpsObject_t * obj)
{
  if (!obj)
    return;
  RPS_ASSERT (rps_is_valid_object (obj));
  pthread_mutex_lock (&rps_rootob_mtx);
  rps_paylsetob_add_element (&rps_rootob_mutset, obj);
  pthread_mutex_unlock (&rps_rootob_mtx);
}				/* end rps_add_global_root_object  */

void
rps_remove_global_root_object (RpsObject_t * obj)
{
  if (!obj)
    return;
  RPS_ASSERT (rps_is_valid_object (obj));
  pthread_mutex_lock (&rps_rootob_mtx);
  rps_paylsetob_remove_element (&rps_rootob_mutset, obj);
  pthread_mutex_unlock (&rps_rootob_mtx);
}				/* end rps_remove_global_root_object */

unsigned
rps_nb_global_root_objects (void)
{
  unsigned nbglobroot = 0;
  pthread_mutex_lock (&rps_rootob_mtx);
  nbglobroot = rps_rootob_mutset.zm_length;
  pthread_mutex_unlock (&rps_rootob_mtx);
  return nbglobroot;
}				/* end rps_nb_global_root_objects */

const RpsSetOb_t *
rps_set_of_global_root_objects (void)
{
  const RpsSetOb_t *setv = NULL;
  int ix = 0;
  unsigned nbglobroot = 0;
  struct kavl_itr_rpsmusetob iter = { };
  pthread_mutex_lock (&rps_rootob_mtx);
  nbglobroot = rps_rootob_mutset.zm_length;
  const RpsObject_t **arrob =
    RPS_ALLOC_ZEROED ((nbglobroot + 1) * sizeof (RpsObject_t *));
  kavl_itr_first_rpsmusetob (rps_rootob_mutset.muset_root, &iter);
  while (ix < (int) nbglobroot)
    {
      arrob[ix++] = kavl_at (&iter)->setobnodrps_obelem;
      if (!kavl_itr_next_rpsmusetob (&iter))
	break;
    };
  RPS_ASSERT (ix == nbglobroot);
  setv = rps_alloc_set_sized (nbglobroot, arrob);
  free (arrob);
  pthread_mutex_unlock (&rps_rootob_mtx);
  return setv;
}				/* end rps_set_of_global_root_objects */


/****************************************************************
 * Hashtable of objects payload for -RpsPyt_HashTblObj
 ****************************************************************/
bool
rps_hash_tbl_is_valid (const RpsHashTblOb_t * htb)
{
  if (!htb)
    return false;
  if (RPS_ZONED_MEMORY_TYPE (htb) != -RpsPyt_HashTblObj)
    return false;
  if (htb->htbob_magic != RPS_HTBOB_MAGIC)
    return false;
  if (htb->zm_length > 0 && !htb->htbob_bucketarr)
    return false;
  return true;
}				/* end rps_hash_tbl_is_valid */

static inline unsigned
rps_hash_tbl_nb_buckets (unsigned capacity, int *pbix)
{
  unsigned nbbuck =		//
    rps_prime_above (4 + capacity / RPS_DEQUE_CHUNKSIZE + capacity / 32);
  if (pbix)
    *pbix = rps_index_of_prime (nbbuck);
  return nbbuck;
}				/* end of rps_hash_tbl_nb_buckets */


// create some unowned hash table of objects of a given initial capacity
RpsHashTblOb_t *
rps_hash_tbl_ob_create (unsigned capacity)
{
  RpsHashTblOb_t *htb = NULL;
  int prix = -1;
  unsigned nbbuck = rps_hash_tbl_nb_buckets (capacity, &prix);
  RPS_ASSERT (prix >= 0);
  htb = RPS_ALLOC_ZONE (sizeof (RpsHashTblOb_t), -RpsPyt_HashTblObj);
  htb->zm_xtra = prix;
  htb->zm_length = 0;
  htb->htbob_magic = RPS_HTBOB_MAGIC;
  htb->htbob_bucketarr =
    RPS_ALLOC_ZEROED (nbbuck * sizeof (struct rps_dequeob_link_st *));
  return htb;
}				/* end rps_hash_tbl_ob_create */


/// return true if ob is added, false if it was there...
static bool
rps_hash_tbl_ob_put1 (RpsHashTblOb_t * htb, RpsObject_t * ob)
{
  RPS_ASSERT (htb && htb->htbob_magic == RPS_HTBOB_MAGIC);
  RPS_ASSERT (ob && rps_is_valid_object (ob));
  int prix = htb->zm_xtra;
  unsigned curlen = htb->zm_length;
  unsigned siz = rps_prime_of_index (prix);
  RpsHash_t hob = ob->zv_hash;
  RPS_ASSERT (siz > 0);
  struct rps_dequeob_link_st **buckarr = htb->htbob_bucketarr;
  if (!buckarr[hob % siz])
    {
      struct rps_dequeob_link_st *newlink =	//
	RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
      newlink->dequeob_chunk[0] = ob;
      buckarr[hob % siz] = newlink;
      htb->zm_length++;
      return true;
    }
  RpsObject_t **slotptr = NULL;
  struct rps_dequeob_link_st *prevbuck = NULL;
  for (struct rps_dequeob_link_st * curbuck = buckarr[hob % siz];
       curbuck != NULL; curbuck = curbuck->dequeob_next)
    {
      for (int i = 0; i < RPS_DEQUE_CHUNKSIZE; i++)
	{
	  if (curbuck->dequeob_chunk[i] == ob)
	    return false;
	  if (!curbuck->dequeob_chunk[i]
	      || curbuck->dequeob_chunk[i] == RPS_HTB_EMPTY_SLOT)
	    if (!slotptr)
	      slotptr = &curbuck->dequeob_chunk[i];
	}
      prevbuck = curbuck;
    }
  if (slotptr)
    {
      *slotptr = ob;
      htb->zm_length++;
      return true;
    };
  struct rps_dequeob_link_st *newlink =	//
    RPS_ALLOC_ZEROED (sizeof (struct rps_dequeob_link_st));
  newlink->dequeob_chunk[0] = ob;
  prevbuck->dequeob_next = newlink;
  newlink->dequeob_prev = prevbuck;
  htb->zm_length++;
  return true;
}				/* end rps_hash_tbl_ob_put1 */


/// return true if ob was removed, false otherwise...
static bool
rps_hash_tbl_ob_remove1 (RpsHashTblOb_t * htb, RpsObject_t * ob)
{
  RPS_ASSERT (htb && htb->htbob_magic == RPS_HTBOB_MAGIC);
  RPS_ASSERT (ob && ob != RPS_HTB_EMPTY_SLOT && rps_is_valid_object (ob));
  int prix = htb->zm_xtra;
  unsigned curlen = htb->zm_length;
  if (curlen == 0)
    return false;
  unsigned siz = rps_prime_of_index (prix);
  RpsHash_t hob = ob->zv_hash;
  RPS_ASSERT (siz > 0 && curlen < siz);
  struct rps_dequeob_link_st **buckarr = htb->htbob_bucketarr;
  struct rps_dequeob_link_st *curbuck = buckarr[hob % siz];
  if (!curbuck)
    return false;
  RpsObject_t **slotptr = NULL;
  struct rps_dequeob_link_st *prevbuck = NULL;
  for (; curbuck != NULL; curbuck = curbuck->dequeob_next)
    {
      unsigned buckcap = 0;
      for (int i = 0; i < RPS_DEQUE_CHUNKSIZE; i++)
	{
	  if (curbuck->dequeob_chunk[i] == ob)
	    {
	      slotptr = &curbuck->dequeob_chunk[i];
	    }
	  else
	    buckcap++;
	};
      if (buckcap == 0)
	{			/* should remove this link */
	  struct rps_dequeob_link_st *nextbuck = curbuck->dequeob_next;
	  if (prevbuck)
	    {
	      prevbuck->dequeob_next = nextbuck;
	      if (nextbuck)
		nextbuck->dequeob_prev = prevbuck;
	    }
	  else
	    buckarr[hob % siz] = nextbuck;
	  free (curbuck);
	}
      else
	{
	  RPS_ASSERT (slotptr);
	  *slotptr = RPS_HTB_EMPTY_SLOT;
	  htb->zm_length--;
	  return true;
	}
      prevbuck = curbuck;
    };
  return false;
}				/* end rps_hash_tbl_ob_remove1 */



void
rps_hash_tbl_reorganize (RpsHashTblOb_t * htb)
{
  if (!htb)
    return;
  if (rps_zoned_memory_type (htb) != -RpsPyt_HashTblObj)
    return;
  RPS_ASSERT (htb->htbob_magic == RPS_HTBOB_MAGIC);
  int oldprix = htb->zm_xtra;
  unsigned curlen = htb->zm_length;
  unsigned oldnbbuck = rps_prime_of_index (oldprix);
  struct rps_dequeob_link_st **oldbuckarr = htb->htbob_bucketarr;
  int newprix = -1;
  unsigned newnbbuck = rps_hash_tbl_nb_buckets (curlen, &newprix);
  htb->htbob_bucketarr =
    RPS_ALLOC_ZEROED (newnbbuck * sizeof (struct rps_dequeob_link_st *));
  htb->zm_length = 0;
  htb->zm_xtra = newprix;
  for (int oldbix = 0; oldbix < (int) oldnbbuck; oldbix++)
    {
      struct rps_dequeob_link_st *nextbuck = NULL;
      struct rps_dequeob_link_st *firstbuck = oldbuckarr[oldbix];
      for (struct rps_dequeob_link_st * oldbuck = firstbuck;
	   oldbuck != NULL; oldbuck = nextbuck)
	{
	  nextbuck = oldbuck->dequeob_next;
	  for (int i = 0; i < RPS_DEQUE_CHUNKSIZE; i++)
	    {
	      const RpsObject_t *curob = oldbuck->dequeob_chunk[i];
	      if (!curob || curob == RPS_HTB_EMPTY_SLOT)
		continue;
	      rps_hash_tbl_ob_put1 (htb, (RpsObject_t *) curob);
	      free (oldbuck);
	    }
	}
      oldbuckarr[oldbix] = NULL;
    }
}				/* end rps_hash_tbl_reorganize */



// reserve space for NBEXTRA more objects, return true on success
// when NBEXTRA is 0, reorganize the hash table to its current size
bool
rps_hash_tbl_ob_reserve_more (RpsHashTblOb_t * htb, unsigned nbextra)
{
  if (!htb)
    return false;
  if (rps_zoned_memory_type (htb) != -RpsPyt_HashTblObj)
    return false;
  RPS_ASSERT (htb->htbob_magic == RPS_HTBOB_MAGIC);
  if (nbextra == 0)
    {
      rps_hash_tbl_reorganize (htb);
      return true;
    };
  struct rps_dequeob_link_st **oldbuckarr = htb->htbob_bucketarr;
  struct rps_dequeob_link_st **newbuckarr = NULL;
  int oldprix = htb->zm_xtra;
  unsigned curlen = htb->zm_length;
  unsigned oldsiz = rps_prime_of_index (oldprix);
  int newprix = -1;
  unsigned newsiz = rps_hash_tbl_nb_buckets (curlen + nbextra, &newprix);
  if (newsiz > oldsiz)
    {
      newbuckarr =		//
	RPS_ALLOC_ZEROED (newsiz * sizeof (struct rps_dequeob_link_st *));
      htb->htbob_bucketarr = newbuckarr;
      htb->zm_length = 0;
      for (int oix = 0; oix < oldsiz; oix++)
	{
	  if (oldbuckarr[oix] == NULL)
	    continue;
	  for (struct rps_dequeob_link_st * oldbuck = oldbuckarr[oix];
	       oldbuck != NULL; oldbuck = oldbuck->dequeob_next)
	    for (int bix = 0; bix < RPS_DEQUE_CHUNKSIZE; bix++)
	      {
		RpsObject_t *curob = oldbuck->dequeob_chunk[bix];
		if (curob != NULL && curob != RPS_HTB_EMPTY_SLOT)
		  {
		    if (!rps_hash_tbl_ob_put1 (htb, curob))
		      RPS_FATAL ("corrupted hash table with same object");
		  }
	      }
	}
      free (oldbuckarr);
      return true;
    }
  else
    return true;
}				/* end rps_hash_tbl_ob_reserve_more */


// add a new element, return true if it was absent
bool
rps_hash_tbl_ob_add (RpsHashTblOb_t * htb, RpsObject_t * obelem)
{
  if (!htb || !obelem)
    return false;
  if (rps_zoned_memory_type (htb) != -RpsPyt_HashTblObj)
    return false;
  RPS_ASSERT (htb->htbob_magic == RPS_HTBOB_MAGIC);
  RPS_ASSERT (rps_is_valid_object (obelem));
  int oldprix = htb->zm_xtra;
  unsigned curlen = htb->zm_length;
  unsigned oldsiz = rps_prime_of_index (oldprix);
  unsigned newsiz = rps_hash_tbl_nb_buckets (curlen + 1, NULL);
  if (newsiz != oldsiz)
    {
      if (!rps_hash_tbl_ob_reserve_more (htb, 1 + curlen / 8))
	RPS_FATAL ("failed to reserve more");
      oldprix = htb->zm_xtra;
      oldsiz = rps_prime_of_index (oldprix);
    }
  return rps_hash_tbl_ob_put1 (htb, obelem);
}				/* end rps_hash_tbl_ob_add */

// remove an element, return true if it was there
bool
rps_hash_tbl_ob_remove (RpsHashTblOb_t * htb, RpsObject_t * obelem)
{
  if (!htb || !obelem)
    return false;
  if (rps_zoned_memory_type (htb) != -RpsPyt_HashTblObj)
    return false;
  RPS_ASSERT (htb->htbob_magic == RPS_HTBOB_MAGIC);
  RPS_ASSERT (rps_is_valid_object (obelem));
  int oldprix = htb->zm_xtra;
  unsigned curlen = htb->zm_length;
  if (curlen == 0)
    return false;
  unsigned oldsiz = rps_prime_of_index (oldprix);
  /* We want to avoid oscillations and too frequent reorganizations
     when removing a few objects and adding the same number of other
     objects later. So we reorganize the table only when it is quite
     empty... */
  if (!rps_hash_tbl_ob_remove1 (htb, obelem))
    return false;
  if (oldsiz > 7
      && rps_hash_tbl_nb_buckets (curlen + 3 + curlen / 3, NULL) < oldsiz)
    {
      if (!rps_hash_tbl_ob_reserve_more (htb, 0))
	RPS_FATAL ("failed to reorganize");
    };
  return true;
}				/* end rps_hash_tbl_ob_remove */


// cardinal of an hash table of objects
unsigned
rps_hash_tbl_ob_cardinal (RpsHashTblOb_t * htb)
{
  if (!htb || rps_zoned_memory_type (htb) != -RpsPyt_HashTblObj)
    return false;
  RPS_ASSERT (htb->htbob_magic == RPS_HTBOB_MAGIC);
  return htb->zm_length;
}				/* end rps_hash_tbl_ob_cardinal */

unsigned
rps_hash_tbl_iterate (RpsHashTblOb_t * htb, rps_object_callback_sig_t * rout,
		      void *data)
{
  if (!htb || rps_zoned_memory_type (htb) != -RpsPyt_HashTblObj)
    return 0;
  if (!rout)
    return 0;
  RPS_ASSERT (htb->htbob_magic == RPS_HTBOB_MAGIC);
  unsigned curlen = htb->zm_length;
  if (!curlen)
    return 0;
  unsigned counter = 0;
  struct rps_dequeob_link_st **buckarr = htb->htbob_bucketarr;
  int prix = htb->zm_xtra;
  unsigned nbbuck = rps_prime_of_index (prix);
  for (unsigned ix = 0; ix < nbbuck; ix++)
    {
      for (struct rps_dequeob_link_st * curbuck = buckarr[ix];
	   curbuck != NULL; curbuck = curbuck->dequeob_next)
	{
	  for (int bix = 0; bix < RPS_DEQUE_CHUNKSIZE; bix++)
	    {
	      RpsObject_t *curob = curbuck->dequeob_chunk[bix];
	      if (curob != NULL && curob != RPS_HTB_EMPTY_SLOT)
		{
		  bool ok = (*rout) (curob, data);
		  // check that number of objects in hashtable did not change
		  RPS_ASSERT (htb->zm_length == curlen);
		  if (!ok)
		    return counter;
		  counter++;
		}
	    }
	};
    }
  return counter;
}				/* end rps_hash_tbl_iterate */

struct rps_hashtblelements_st
{
#define RPS_HTBEL_MAGIC 0x19b2475d	/*431114077 */
  unsigned htbel_magic_num;	/* should be RPS_HTBEL_MAGIC */
  unsigned htbel_maxcount;
  unsigned htbel_size;
  unsigned htbel_curix;
  const RpsObject_t *htbel_obarr[];	/* allocated size is htbel_size, a prime */
};

static rps_object_callback_sig_t rps_hash_tbl_iter_for_set;
static bool
rps_hash_tbl_iter_for_set (RpsObject_t * ob, void *data)
{
  struct rps_hashtblelements_st *htbel = data;
  RPS_ASSERT (htbel && htbel->htbel_magic_num == RPS_HTBEL_MAGIC);
  RPS_ASSERT (ob && rps_is_valid_object (ob));
  RPS_ASSERT (htbel->htbel_curix < htbel->htbel_size);
  RPS_ASSERT (htbel->htbel_curix < htbel->htbel_maxcount);
  htbel->htbel_obarr[htbel->htbel_curix++] = ob;
  return true;
}				/* end rps_hash_tbl_iter_for_set */

// make a set from the elements of an hash table
const RpsSetOb_t *
rps_hash_tbl_set_elements (RpsHashTblOb_t * htb)
{
  const RpsSetOb_t *setv = NULL;
  if (!htb || rps_zoned_memory_type (htb) != -RpsPyt_HashTblObj)
    return NULL;
  RPS_ASSERT (htb->htbob_magic == RPS_HTBOB_MAGIC);
  unsigned curlen = htb->zm_length;
  unsigned primsiz = rps_prime_above (curlen + 1);
  struct rps_hashtblelements_st *htbel =
    RPS_ALLOC_ZEROED (sizeof (struct rps_hashtblelements_st) +
		      primsiz * sizeof (RpsObject_t *));
  htbel->htbel_magic_num = RPS_HTBEL_MAGIC;
  htbel->htbel_maxcount = curlen;
  htbel->htbel_size = primsiz;
  htbel->htbel_curix = 0;
  unsigned nbiter =
    rps_hash_tbl_iterate (htb, rps_hash_tbl_iter_for_set, (void *) htbel);
  RPS_ASSERT (nbiter == curlen);
  setv = rps_alloc_set_sized (nbiter, htbel->htbel_obarr);
  free (htbel);
  return setv;
}				/* end rps_hash_tbl_set_elements */


/***************** end of file composite_rps.c from refpersys.org **********/
