/****************************************************************
 * file load_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It contains the heap dumping machinery.
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

#define RPS_DUMPER_MAGIC 0x2501f5e3	/*620885475 */

struct RpsPayl_Dumper_st
{
  RPSFIELDS_ZONED_VALUE;
  unsigned du_magic;		/* always RPS_DUMPER_MAGIC */
};


bool
rps_is_valid_dumper (RpsDumper_t * du)
{
  if (!du)
    return false;
  if (du->du_magic != RPS_DUMPER_MAGIC)
    return false;
}				/* end rps_is_valid_dumper */

void
rps_dump_heap (void)
{
  RPS_FATAL ("unimplemented rps_dump_heap to %s", rps_dump_directory);
}				/* end rps_dump_heap */

#warning a lot of dumping routines are missing here
