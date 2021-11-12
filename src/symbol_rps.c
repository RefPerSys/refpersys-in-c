/****************************************************************
 * file load_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It manages the global symbol table
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

struct internal_symbol_node_rps_st
{
  RpsSymbol_t *synodrps_symbol;
    KAVL_HEAD (struct internal_symbol_node_rps_st) synodrps_head;
};

static int
rps_symbol_node_cmp (const struct internal_symbol_node_rps_st *left,
		     const struct internal_symbol_node_rps_st *right)
{
  RPS_ASSERT (left);
  RPS_ASSERT (right);
  RpsSymbol_t *syleft = left->synodrps_symbol;
  RpsSymbol_t *syright = right->synodrps_symbol;
  RPS_ASSERT (syleft != NULL && syleft->zm_type == -RpsPyt_Symbol);
  RPS_ASSERT (syright != NULL && syright->zm_type == -RpsPyt_Symbol);
  if (syleft == syright)
    return 0;
  const RpsString_t *namleft = syleft->symb_name;
  const RpsString_t *namright = syright->symb_name;
  RPS_ASSERT (namleft != NULL && namleft->zm_type == RPS_TYPE_STRING);
  RPS_ASSERT (namright != NULL && namright->zm_type == RPS_TYPE_STRING);
  return strcmp (namleft->cstr, namright->cstr);
}				/* end rps_symbol_node_cmp */

KAVL_INIT (rpsynod, struct internal_symbol_node_rps_st, synodrps_head,
	   rps_symbol_node_cmp)
     void rpsldpy_symbol (RpsObject_t * obj, RpsLoader_t * ld,
			  const json_t * jv, int spix)
{
  char idbuf[32];
  memset (idbuf, 0, sizeof (idbuf));
  RPS_ASSERT (rps_is_valid_object (obj));
  rps_oid_to_cbuf (obj->ob_id, idbuf);
  json_t *jsymbname = json_object_get (jv, "symb_name");
  json_t *jsymbvalue = json_object_get (jv, "symb_value");
  if (!json_is_string (jsymbname))
    {
      RPS_FATAL ("invalid symb_name for %s in space#%d\n... json %s", idbuf, spix,	//
		 json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
    };
  RpsSymbol_t *pysymb = RPS_ALLOC_ZONE (sizeof (RpsSymbol_t), -RpsPyt_Symbol);
  pysymb->symb_name = rps_alloc_string (json_string_value (jsymbname));
  if (jsymbvalue)
    pysymb->symb_value = rps_loader_json_to_value (ld, jsymbvalue);
  pysymb->payl_owner = obj;
  obj->ob_payload = pysymb;
  RPS_FATAL ("missing code to register the symbol payload of %s named %s",
	     idbuf, json_string_value (jsymbname));
}				/* end rpsldpy_symbol */
