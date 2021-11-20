/****************************************************************
 * file include/assert_rps.h
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *      Defines the object ID interface.
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

#ifndef __REFPERSYS_INCLUDE_OID_RPS_H_INCLUDED__
#define __REFPERSYS_INCLUDE_OID_RPS_H_INCLUDED__


typedef struct _RpsOid
{
  uint64_t id_hi, id_lo;
} RpsOid;

#define RPS_B62DIGITS \
  "0123456789"				  \
  "abcdefghijklmnopqrstuvwxyz"		  \
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

#define RPS_OID_NULL ((RpsOid){id_hi:0, id_lo:0})

#define RPS_OID_BUFLEN 24

#define RPS_OID_BASE (sizeof(RPS_B62DIGITS)-1)

#define RPS_OID_HI_MIN (62*62*62)
 
/* 8392993658683402240, about 8.392994e+18 */
#define RPS_OID_HI_MAX \
((uint64_t)10 * 62 * (62 * 62 * 62) * (62 * 62 * 62) * (62 * 62 * 62))

#define RPS_OID_HI_NBDIGITS 11

#define RPS_DELTA_OID_HI (RPS_OID_HI_MAX - RPS_ID_HI_MIN)
#define RPS_MIN_OID_LO (62*62*62)
#define RPS_MAX_OID_LO /* about 3.52161e+12 */ \
  ((uint64_t)62 * (62L * 62 * 62) * (62 * 62 * 62))
#define RPS_DELTA_OID_LO (RPS_MAX_OID_LO - RPS_MIN_OID_LO)
#define RPS_NBDIGITS_OID_LO 8
#define RPS_OID_NBCHARS (RPS_NBDIGITS_OID_HI+RPS_NBDIGITS_OID_LO+1)
#define RPS_OID_MAXBUCKETS (10*62)


extern bool rps_oid_is_null (const RpsOid oid);
extern bool rps_oid_is_valid (const RpsOid oid);
extern int rps_oid_cmp (const RpsOid oid1, const RpsOid oid2);
extern void rps_oid_to_cbuf (const RpsOid oid, char cbuf[RPS_OID_BUFLEN]);
extern RpsOid rps_cstr_to_oid (const char *cstr, const char **pend);
extern unsigned rps_oid_bucket_num (const RpsOid oid);
extern RpsHash_t rps_oid_hash (const RpsOid oid);


/* Checks if two object IDs are equal. */
inline bool
rps_oid_equal (RpsOid lhs, RpsOid rhs)
{
  return rps_oid_cmp (lhs, rhs) == 0;
}


/* Checks if an object ID is less than another. */
inline bool
rps_oid_less_than (RpsOid lhs, RpsOid rhs)
{
  return rps_oid_cmp (lhs, rhs) == -1;
}


/* Checks if an object ID is less than or equal to another. */
inline bool
rps_oid_less_than_equal (RpsOid lhs, RpsOid rhs)
{
  return rps_oid_cmp (lhs, rhs) <= 0;
}


/* Checks if an object ID is greater than another. */
inline bool
rps_oid_greater_than (RpsOid lhs, RpsOid rhs)
{
  return rps_oid_cmp (lhs, rhs) == 1;
}


/* Checks if an object ID is greater than or equal to another. */
inline bool
rps_oid_greater_than_equal (RpsOid lhs, RpsOid rhs)
{
  return rps_oid_cmp (lhs, rhs) >= 0;
}



// compute a random and valid oid
extern RpsOid rps_oid_random (void);

#endif /* __REFPERSYS_INCLUDE_OID_RPS_H_INCLUDED__ */

