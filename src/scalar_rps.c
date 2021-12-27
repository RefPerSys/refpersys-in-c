/****************************************************************
 * file scalar_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It supports scalar values.
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

RpsHash_t
rps_hash_cstr (const char *s)
{
  if (s == NULL)
    return 0;
  long ll = strlen (s);
  int l = ll;
  const char *str = s;
  RpsHash_t h1 = l % 13, h2 = l, h = 0;
  while (l > 4)
    {
      h1 =
	(509 * h2 +
	 307 * ((signed char *) str)[0]) ^ (1319 * ((signed char *) str)[1]);
      h2 =
	(17 * l + 5 + 5309 * h2) ^ ((3313 * ((signed char *) str)[2]) +
				    9337 * ((signed char *) str)[3] + 517);
      l -= 4;
      str += 4;
    }
  if (l > 0)
    {
      h1 = (h1 * 7703) ^ (503 * ((signed char *) str)[0]);
      if (l > 1)
	{
	  h2 = (h2 * 7717) ^ (509 * ((signed char *) str)[1]);
	  if (l > 2)
	    {
	      h1 = (h1 * 9323) ^ (11 + 523 * ((signed char *) str)[2]);
	      if (l > 3)
		{
		  h2 =
		    (h2 * 7727 + 127) ^ (313 +
					 547 * ((signed char *) str)[3]);
		}
	    }
	}
    }
  h = (h1 * 29311 + 59) ^ (h2 * 7321 + 120501);
  if (!h)
    {
      h = h1;
      if (!h)
	{
	  h = h2;
	  if (!h)
	    h = (ll & 0xffffff) + 11;
	}
    }
  return h;
}				/* end rps_hash_cstr */


const char *
rps_type_str (int ty)
{
  switch (ty)
    {
    case RPS_TYPE__NONE:
      return "?None?";
    case RPS_TYPE_INT:
      return "Int";
    case RPS_TYPE_DOUBLE:
      return "Double";
    case RPS_TYPE_STRING:
      return "String";
    case RPS_TYPE_JSON:
      return "Json";
    case RPS_TYPE_GTKWIDGET:
      return "GtkWidget";
    case RPS_TYPE_TUPLE:
      return "Tuple";
    case RPS_TYPE_SET:
      return "Set";
    case RPS_TYPE_CLOSURE:
      return "Closure";
    case RPS_TYPE_OBJECT:
      return "Object";
    case -RpsPyt_CallFrame:
      return "/CallFrame";
    case -RpsPyt_Loader:
      return "/Loader";
    case -RpsPyt_AttrTable:
      return "/AttrTable";
    case -RpsPyt_StringBuf:
      return "/StringBuf";
    case -RpsPyt_Symbol:
      return "/Symbol";
    case -RpsPyt_ClassInfo:
      return "/ClassInfo";
    case -RpsPyt_MutableSetOb:
      return "/MutableSetOb";
    case -RpsPyt_DequeOb:
      return "/DequeOb";
    case -RpsPyt_Tasklet:
      return "/Tasklet";
    case -RpsPyt_Agenda:
      return "/Agenda";
    case -RpsPyt_StringDict:
      return "/StringDict";
    case -RpsPyt_HashTblObj:
      return "/HashTblObj";
    case -RpsPyt_Space:
      return "/Space";
    case -RpsPyt_Dumper:
      return "/Dumper";
    default:
      {
	static _Thread_local char buf[16];
	memset (buf, 0, sizeof (buf));
	snprintf (buf, sizeof (buf), "??ty%d?", ty);
	return buf;
      }
    }
}				/* end rps_type_str */

RpsHash_t
rps_hash_double (double x)
{
  RpsHash_t h = 17;
  double f = 0.0;
  int e = 0;
  if (isnan (x))
    RPS_FATAL ("cannot hash NAN");
  f = frexp (x, &e);
  h = (unsigned long) (f * 1000001537) ^ (17 * e + 93);
  if (h <= 4)
    h += 10223;
  return h;
}				/* end rps_hash_double */

const RpsDouble_t *
rps_alloc_boxed_double (double x)
{
  RpsDouble_t *dblv = NULL;
  if (isnan (x))
    RPS_FATAL ("cannot allocate boxed NAN");
  dblv = RPS_ALLOC_ZONE (sizeof (RpsDouble_t), RPS_TYPE_DOUBLE);
  dblv->dbl_val = x;
  dblv->zv_hash = rps_hash_double (x);
  return dblv;
}				/* end rps_alloc_boxed_double */

RpsValue_t
rps_tagged_integer_value (intptr_t i)
{
  return (RpsValue_t) ((i << 1) | 1);
}				/* end rps_tagged_integer_value */

bool
rps_is_tagged_integer (const RpsValue_t v)
{
  return ((intptr_t) v % 2 != 0);
}				/* end rps_is_tagged_integer */

intptr_t
rps_value_to_integer (const RpsValue_t v)	/* gives 0 for a non-tagged integer */
{
  if ((intptr_t) v % 2 != 0)
    return ((intptr_t) v) >> 1;
  else
    return 0;
}				/* end rps_value_to_integer */

/// internal compare function
static int
rps_compare_cstr_ptr (const void *p1, const void *p2)
{
  const char **s1 = (const char **) p1;
  const char **s2 = (const char **) p2;
  RPS_ASSERT (*s1 != NULL);
  RPS_ASSERT (*s2 != NULL);
  return strcmp (*s1, *s2);
}				/* end rps_compare_cstr_ptr */

/// internal recursive functon to compute two hashes
static void
rps_compute_json_two_hash (int depth, const json_t * js, long *pl1, long *pl2)
{
  if (!js)
    return;
  RPS_ASSERT (pl1 != NULL);
  RPS_ASSERT (pl2 != NULL);
  switch (json_typeof (js))
    {
    case JSON_OBJECT:
      {
	size_t jsz = json_object_size (js) + 1;
	const char **arrkey = RPS_ALLOC_ZEROED (jsz * sizeof (const char *));
	int cnt = 0;
	{
	  const char *key = NULL;
	  json_t *valj = NULL;
	  json_object_foreach ((json_t *) js, key, valj) arrkey[cnt++] = key;
	}
	qsort (arrkey, jsz, sizeof (const char *), rps_compare_cstr_ptr);
	for (int ix = 0; ix < cnt; ix++)
	  {
	    const char *key = arrkey[ix];
	    json_t *valj = json_object_get (js, key);
	    long sl1 = 0, sl2 = 0;
	    long oldl1 = *pl1;
	    long oldl2 = *pl2;
	    rps_compute_json_two_hash (depth + 1, valj, &sl1, &sl2);
	    if (ix % 2 == 0)
	      {
		*pl1 ^= 17 * ix + sl1 - 37 * sl2 + (oldl1 ^ oldl2 + depth);
		*pl2 += ix + 31 * sl1 ^ 43 * sl2 + (17 * oldl1 - 49 * oldl2);
	      }
	    else
	      {
		*pl1 += 5 * ix - 7 * sl1 + 1553 * sl2 + oldl2;
		*pl2 ^=
		  depth - 5 * ix + sl1 * 1597 - 31 * oldl1 + 1523 * oldl2;
	      }
	  }
	free (arrkey);
      }
      return;
    case JSON_ARRAY:
      {
	size_t jsz = json_array_size (js);
	for (size_t ix = 0; ix < jsz; ix++)
	  {
	    json_t *valj = json_array_get (js, ix);
	    long sl1 = 0, sl2 = 0;
	    long oldl1 = *pl1;
	    long oldl2 = *pl2;
	    rps_compute_json_two_hash (depth + 1, valj, &sl1, &sl2);
	    if (depth + ix % 2 == 0)
	      {
		*pl1 ^= 11 * oldl1 + 17 * oldl2 + sl2 - 7 * sl2;
		*pl2 += 13 * oldl1 + 2549 * sl1 - 17 * sl2 + oldl2 + ix;
	      }
	    else
	      {
		*pl1 -=
		  2557 * sl1 + 1567 * oldl1 + 13 * oldl2 - 409 * sl2 - depth;
		*pl2 ^=
		  17 * ix ^ 419 * sl1 - 353 * oldl1 + 17 * oldl2 - 439 * sl2 +
		  ix;
	      }
	  }
      }
      return;
    case JSON_STRING:
      {
	const char *sv = json_string_value (js);
	RpsHash_t h = rps_hash_cstr (sv);
	if (depth % 2 == 0)
	  {
	    *pl1 ^= 439 * h + depth * 17;
	    *pl2 += 353 * h;
	  }
	else
	  {
	    *pl1 ^= 433 * h - depth * 11;
	    *pl2 += depth - h % 439;
	  }
      }
      return;
    case JSON_INTEGER:
      {
	intptr_t i = json_integer_value (js);
	*pl1 ^= i;
	*pl2 += 11 * depth ^ i % 31;
      }
      return;
    case JSON_REAL:
      {
	double d = json_real_value (js);
	if (isnan (d))
	  {
	    *pl1 += 37;
	    *pl2 ^= depth + 11;
	  }
	else
	  {
	    *pl1 += rps_hash_double (d);
	    *pl2 ^= depth % 13;
	  }
      }
      return;
    case JSON_TRUE:
      {
	*pl1 = ~*pl1;
	*pl2 ^= depth;
      }
      return;
    case JSON_FALSE:
      {
	*pl1 += 3;
	*pl2 ^= depth;
      }
      return;
    case JSON_NULL:
      {
	*pl1 += (*pl2 % 65171);
	*pl2 -= 11 * depth;
      }
      return;
    default:
      RPS_FATAL ("unimplemented rps_compute_json_two_hash jsonty#%d",
		 json_typeof (js));
      return;
    }
}				/* end rps_compute_json_two_hash */

RpsHash_t
rps_json_hash (const json_t * js)
{
  RpsHash_t h = 0;
  long l1 = 15017, l2 = 65183;
  if (!js)
    return 0;
  rps_compute_json_two_hash (0, js, &l1, &l2);
  h = (l1 ^ l2) + ((l1 - l2) >> 31);
  if (h == 0)
    h = ((l1 & 0xfffffff) % 65167 + (l2 & 0xffffff) % 15187) + 10;
  RPS_ASSERT (h != 0);
  return h;
}				/* end rps_json_hash */



const RpsJson_t *
rps_alloc_json (const json_t * js)
{
  RpsJson_t *vj = NULL;
  if (!js)
    return NULL;
  vj = RPS_ALLOC_ZONE (sizeof (RpsJson_t), RPS_TYPE_JSON);
  vj->zv_hash = rps_json_hash (js);
  vj->json = js;
  return vj;
}				/* end rps_alloc_json */

const RpsJson_t *
rps_load_json (const json_t * js, RpsLoader_t * ld)
{
  json_t *jv = json_object_get (js, "json");
  RPS_ASSERT (rps_is_valid_filling_loader (ld));
  RPS_ASSERT (jv);
  return rps_alloc_json (jv);
}				/* end rps_load_json */

const json_t *
rps_json_value (RpsValue_t val)
{
  if (rps_value_type (val) != RPS_TYPE_JSON)
    return NULL;
  const RpsJson_t *vj = (const RpsJson_t *) val;
  return vj->json;
}				/* end rps_json_value */

const RpsGtkWidget_t *
rps_alloc_gtk_widget (GtkWidget * widg)
{
  RpsGtkWidget_t *vw = NULL;
  RpsHash_t h = 0;
  if (widg == NULL)
    return NULL;
  vw = RPS_ALLOC_ZONE (sizeof (RpsGtkWidget_t), RPS_TYPE_GTKWIDGET);
  h = 17 + (((uintptr_t) widg) % 45000931);
  if (h == 0)
    h = ((((uintptr_t) widg) & 0xffffff) + 540773);
  RPS_ASSERT (h != 0);
  vw->zv_hash = h;
  vw->gtk_widget = widg;
  return vw;
}				/* end rps_alloc_gtk_widget */


const RpsString_t *
rps_alloc_string (const char *str)
{
  RpsString_t *res = NULL;
  if (!str)
    return NULL;
  int slen = strlen (str);
  if (!g_utf8_validate (str, slen, NULL))
    RPS_FATAL ("bad non-UTF string of %d char to rps_alloc_string", slen);
  int rndslen = ((slen + 3) | 7) + 1;	// round-up allocation length to multiple of eight bytes
  res = RPS_ALLOC_ZONE (sizeof (RpsString_t) + rndslen, RPS_TYPE_STRING);
  res->zv_hash = rps_hash_cstr (str);
  memcpy (res->cstr, str, slen);
  res->zm_length =		// number of UTF8 characters
    g_utf8_strlen (str, slen);
  return res;
}				/* end of rps_alloc_string */

const char *
rps_stringv_utf8bytes (RpsValue_t v)
{
  if (rps_value_type (v) != RPS_TYPE_STRING)
    return NULL;
  const RpsString_t *strv = (const RpsString_t *) v;
  return strv->cstr;
}				/* end rps_string_utf8bytes */

unsigned
rps_stringv_utf8length (RpsValue_t v)
{
  if (rps_value_type (v) != RPS_TYPE_STRING)
    return 0;
  const RpsString_t *strv = (const RpsString_t *) v;
  return strv->zm_length;
}				/* end rps_stringv_utf8length */

RpsHash_t
rps_stringv_hash (RpsValue_t v)
{
  if (rps_value_type (v) != RPS_TYPE_STRING)
    return 0;
  const RpsString_t *strv = (const RpsString_t *) v;
  return strv->zv_hash;
}				/* end rps_stringv_hash */

/********************* end of file scalar_rps.c ***************/
