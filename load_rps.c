/****************************************************************
 * file load_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It contains the initial loading machinery.
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

#define RPS_LOADER_MAGIC 0x156e62d5	/*359555797 */

struct RpsPayl_Loader_st
{
  RPSFIELDS_ZONED_VALUE;
  unsigned ld_magic;		/* always RPS_LOADER_MAGIC */
};

bool
rps_is_valid_loader (RpsLoader_t * ld)
{
  if (!ld)
    return false;
  return ld->ld_magic == RPS_LOADER_MAGIC;
}				/* end rps_is_valid_loader */

void
rps_load_initial_heap (void)
{
#warning rps_load_initial_heap needs to be coded
}				/* end rps_load_initial_heap */
