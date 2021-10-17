/****************************************************************
 * file oid_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It contains the support of object ids.
 *
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      Abhishek Chakravarti <abhishek@taranjali.org>
 *      Nimesh Neema <nimeshneema@gmail.com>
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


const char rps_sb62digits[] = RPS_B62DIGITS;

bool
rps_oid_is_null (const RpsOid_t oid)
{
  return oid.id_hi == 0 && oid.id_lo == 0;
}				/* end rps_oid_is_null */

bool
rps_oid_is_valid (const RpsOid_t oid)
{
  return oid.id_hi >= RPS_MIN_OID_HI && oid.id_hi < RPS_MAX_OID_HI
    && oid.id_lo >= RPS_MIN_OID_LO && oid.id_lo < RPS_MAX_OID_LO;
}				/* end rps_oid_is_valid */

void
rps_oid_to_cbuf (const RpsOid_t oid, char cbuf[RPS_OIDBUFLEN])
{
  if (!cbuf)
    return;
  if (rps_oid_is_null (oid))
    {
      strcpy (cbuf, "__");
      return;
    };
  if (!rps_oid_is_valid (oid))
    return;
  /// example cbuf = "_0abcdefghijABCDEFG"
  ///                  |0         |11    |19
  assert (sizeof ("_0abcdefghijABCDEFG") - 1 ==
	  RPS_NBDIGITS_OID_HI + RPS_NBDIGITS_OID_LO);
  memset (cbuf, 0, RPS_OIDBUFLEN);
  char *last = cbuf + RPS_NBDIGITS_OID_HI;
  char *pc = last;
  cbuf[0] = '_';
  uint64_t n = oid.id_hi;
  do
    {
      unsigned d = n % RPS_OIDBASE;
      n = n / RPS_OIDBASE;
      *pc = rps_sb62digits[d];
      pc--;
    }
  while (pc > cbuf);
  char *start = cbuf + RPS_NBDIGITS_OID_HI;
  last = start + RPS_NBDIGITS_OID_LO;
  pc = last;
  n = oid.id_lo;
  do
    {
      unsigned d = n % RPS_OIDBASE;
      n = n / RPS_OIDBASE;
      *pc = rps_sb62digits[d];
      pc--;
    }
  while (pc > start);
}				/* end rps_oid_to_cbuf */



/******************* end of file oid_rps.c *****************/
