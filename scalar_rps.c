/****************************************************************
 * file main_rps.c
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
  dblv = RPS_ALLOC_ZONE (sizeof (RpsDouble_t), RpsTy_Double);
  dblv->dbl_val = x;
  dblv->zv_hash = rps_hash_double (x);
  return dblv;
}				/* end rps_alloc_boxed_double */
