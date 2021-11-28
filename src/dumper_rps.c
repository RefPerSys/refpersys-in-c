/****************************************************************
 * file dumper_rps.c
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
  /* The dumper probably needs to contain a big hash table of visited
     objects; a first pass is scanning the heap, starting from global
     roots including the agenda. During the dump only one pthread should
     be running, every other pthread should be blocked. The agenda should
     be "idle".
   */
  RpsString_t *du_dirnam;
  RpsHashTblOb_t *du_visitedht;
  RpsMutableSetOb_t *du_dumpedset;
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
rps_dump_heap (const char *dirn)
{
  if (!dirn)
    dirn = rps_dump_directory;
  if (rps_agenda_is_running ())
    RPS_FATAL ("cannot dump heap into %s while agenda is running", dirn);
  /* TODO: Do we need some temporary dumper object, owning the dumper
     payload below? */
  RpsDumper_t *dumper =		//    
    RPS_ALLOC_ZONE (sizeof (RpsDumper_t),
		    -RpsPyt_Dumper);
  dumper->du_magic = RPS_DUMPER_MAGIC;
  dumper->du_dirnam = rps_alloc_string (dirn);
  dumper->du_visitedht =	//
    rps_hash_tbl_ob_create (16 + 3 * rps_nb_global_root_objects ());
  RPS_FATAL ("unimplemented rps_dump_heap to %s", rps_dump_directory);
}				/* end rps_dump_heap */

#warning a lot of dumping routines are missing here
