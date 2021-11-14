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


const RpsSetOb_t *
rps_alloc_set_sized (unsigned nbcomp, RpsObject_t ** arr)
{
  RpsSetOb_t *set = NULL;
  if (!arr && nbcomp > 0)
    return NULL;
  const RpsObject_t **arrcpy =
    RPS_ALLOC_ZEROED ((nbcomp + 1) * sizeof (RpsObject_t *));
  memcpy (arrcpy, arr, nbcomp * sizeof (RpsObject_t *));
  rps_object_array_qsort (arrcpy, (int) nbcomp);
  int card = 0;
  for (int ix = 0; ix < (int) nbcomp - 1; ix++)
    if (arrcpy[ix + 1] != arrcpy[ix] && arrcpy[ix])
      card++;
  set =
    RPS_ALLOC_ZONE (sizeof (RpsSetOb_t) + card * sizeof (RpsObject_t *),
		    RPS_TYPE_SET);
  set->zm_length = card;
  int nbel = 0;
  for (int ix = 0; ix < (int) nbcomp - 1; ix++)
    if (arrcpy[ix + 1] != arrcpy[ix] && arrcpy[ix])
      set->set_elem[nbel++] = arrcpy[ix];
  if (set->set_elem[nbel] != arrcpy[nbcomp - 1] && arrcpy[nbcomp - 1])
    set->set_elem[nbel++] = arrcpy[nbcomp - 1];
  free (arrcpy);
  RPS_ASSERT (card == nbel);
  return set;
}				/* end rps_alloc_set_sized */

const RpsSetOb_t *
rps_alloc_vset (unsigned card, /*objects */ ...)
{
  RpsSetOb_t *set = NULL;
  va_list arglist;
  va_start (arglist, card);
  RpsObject_t **arrcpy =
    RPS_ALLOC_ZEROED ((card + 1) * sizeof (RpsObject_t *));
  for (int ix = 0; ix < (int) card; ix++)
    {
      arrcpy[ix] = va_arg (arglist, RpsObject_t *);
    }
  va_end (arglist);
  set = rps_alloc_set_sized (card, arrcpy);
  free (arrcpy);
  return set;
}				/* end rps_alloc_vset */

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
  RpsClosure_t *clos = NULL;
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
  RpsClosure_t *clos = NULL;
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

struct internal_mutable_set_ob_node_st
{
  RpsObject_t *setob_elem;
    KAVL_HEAD (struct internal_mutable_set_ob_node_st) setobrps_head;
};

static int
rps_mutable_set_ob_node_cmp (const struct internal_mutable_set_ob_node_st
			     *left,
			     const struct internal_mutable_set_ob_node_st
			     *right)
{
  RPS_ASSERT (left);
  RPS_ASSERT (right);
  RpsObject_t *obleft = left->setob_elem;
  RpsObject_t *obright = right->setob_elem;
  RPS_ASSERT (obleft != NULL && obleft->zm_type == RPS_TYPE_OBJECT);
  RPS_ASSERT (obright != NULL && obright->zm_type == RPS_TYPE_OBJECT);
  if (obleft == obright)
    return 0;
  return rps_oid_cmp (obleft->ob_id, obright->ob_id);
}				/* end rps_mutable_set_ob_node_cmp */

KAVL_INIT (rpsmusetob, struct internal_mutable_set_ob_node_st, setobrps_head,
	   rps_mutable_set_ob_node_cmp);


/* loading mutable set of objects */
void
rpsldpy_setob (RpsObject_t * obj, RpsLoader_t * ld, const json_t * jv,
	       int spix)
{
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_filling_loader (ld));
  json_t *jssetob = json_object_get (jv, "setob");
  if (jssetob && json_is_array (jssetob))
    {
      int card = (int) json_array_size (jssetob);
      for (int ix = 0; ix < card; ix++)
	{
	  json_t *jcurelem = json_array_get (jssetob, ix);
	  if (json_is_string (jcurelem))
	    {
	    }
	}
    }
  RPS_FATAL ("unimplemented rpsldpy_setob spix#%d jv\n..%s",
	     spix, json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
}				/* end rpsldpy_setob */
