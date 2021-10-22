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

const RpsTupleOb_t *
rps_alloc_tuple_sized (unsigned arity, const RpsObject_t ** arr)
{
  RpsTupleOb_t *tup = NULL;
  if (arr == NULL && arity > 0)
    return NULL;
  tup =
    RPS_ALLOC_ZONE (sizeof (RpsTupleOb_t) + arity * sizeof (RpsObject_t *),
		    RpsTy_TupleOb);
  for (int ix = 0; ix < (int) arity; ix++)
    {
      if (arr[ix] && rps_is_valid_object (arr[ix]))
	tup->tuple_comp[ix] = arr[ix];
    }
#warning incomplete rps_alloc_tuple_sized
  RPS_FATAL ("incomplete rps_alloc_tuple_sized arity %u", arity);
}				/* end of rps_alloc_tuple_sized */
