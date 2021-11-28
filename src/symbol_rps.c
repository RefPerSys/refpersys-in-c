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


static int
rps_symbol_node_cmp (const struct internal_symbol_node_rps_st *left,
		     const struct internal_symbol_node_rps_st *right)
{
  RPS_ASSERT (left);
  RPS_ASSERT (right);
  RpsSymbol_t *syleft = left->synodrps_symbol;
  RpsSymbol_t *syright = right->synodrps_symbol;
  if (syleft == syright)
    return 0;
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (syleft) == -RpsPyt_Symbol);
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (syright) == -RpsPyt_Symbol);
  const RpsString_t *namleft = syleft->symb_name;
  const RpsString_t *namright = syright->symb_name;
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (namleft) == RPS_TYPE_STRING);
  RPS_ASSERT (RPS_ZONED_MEMORY_TYPE (namright) == RPS_TYPE_STRING);
  return strcmp (namleft->cstr, namright->cstr);
}				/* end rps_symbol_node_cmp */

KAVL_INIT (rpsynod, struct internal_symbol_node_rps_st, synodrps_head,
	   rps_symbol_node_cmp);

static pthread_mutex_t rps_symbol_mtx =
  PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static struct internal_symbol_node_rps_st *rps_symbol_node_root;

RpsSymbol_t *
rps_register_symbol (const char *name)
{
  RpsSymbol_t *symb = NULL;
  RPS_ASSERT (name != NULL);
  pthread_mutex_lock (&rps_symbol_mtx);
  const RpsString_t *namestr = rps_alloc_string (name);
  RpsSymbol_t pseudosymb =	//
  {.zm_atype = -RpsPyt_Symbol,.zm_gcmark = 1,.symb_name = namestr };
  struct internal_symbol_node_rps_st pseudonode	//
  = {.synodrps_symbol = &pseudosymb };
  struct internal_symbol_node_rps_st *nod =
    kavl_find_rpsynod (rps_symbol_node_root, &pseudonode, NULL);
  if (!nod)
    {
      nod = RPS_ALLOC_ZEROED (sizeof (struct internal_symbol_node_rps_st));
      symb = RPS_ALLOC_ZONE (sizeof (RpsSymbol_t), -RpsPyt_Symbol);
      symb->symb_name = namestr;
      nod->synodrps_symbol = symb;
      kavl_insert_rpsynod (&rps_symbol_node_root, nod, NULL);
    }
  else
    symb = nod->synodrps_symbol;
end:
  pthread_mutex_unlock (&rps_symbol_mtx);
  return symb;
}				/* end rps_register_symbol */

RpsSymbol_t *
rps_find_symbol (const char *name)
{
  RpsSymbol_t *symb = NULL;
  RPS_ASSERT (name != NULL);
  pthread_mutex_lock (&rps_symbol_mtx);
  const RpsString_t *namestr = rps_alloc_string (name);
  RpsSymbol_t pseudosymb =	//
  {.zm_atype = -RpsPyt_Symbol,	//
    .zm_gcmark = 1,.		//
      symb_name = namestr	//
  };
  struct internal_symbol_node_rps_st pseudonode = {.synodrps_symbol =
      &pseudosymb
  };
  struct internal_symbol_node_rps_st *nod =
    kavl_find_rpsynod (rps_symbol_node_root, &pseudonode, NULL);
  if (nod && nod != &pseudonode)
    symb = nod->synodrps_symbol;
end:
  pthread_mutex_unlock (&rps_symbol_mtx);
  return symb;
}				/* end rps_find_symbol */

void
rpsldpy_symbol (RpsObject_t * obj, RpsLoader_t * ld,
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
  RpsSymbol_t *pysymb = rps_register_symbol (json_string_value (jsymbname));
  if (jsymbvalue)
    pysymb->symb_value = rps_loader_json_to_value (ld, jsymbvalue);
  pysymb->payl_owner = obj;
  obj->ob_payload = pysymb;
}				/* end rpsldpy_symbol */


/*************** end of file symbol_rps.c ***********/
