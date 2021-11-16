/****************************************************************
 * file agenda_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      Implementation of the agenda
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

void
rpsldpy_agenda (RpsObject_t * obj, RpsLoader_t * ld, const json_t * jv,
		int spix)
{
  char idbuf[32];
  memset (idbuf, 0, sizeof (idbuf));
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (rps_is_valid_object (obj));
  rps_oid_to_cbuf (obj->ob_id, idbuf);
  RPS_ASSERT (rps_is_valid_filling_loader (ld));
  RPS_ASSERT (json_is_object (jv));
  RPS_ASSERT (spix >= 0);
  static int count;
  if (count++ > 0)
    RPS_FATAL("rpsldpy_agenda obj %s already called %d times spix#%d\n..jv=%s",
	      idbuf, count,
	      spix, json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
  RpsAgenda_t*agenpayl //
    = RPS_ALLOC_ZONE(sizeof(RpsAgenda_t), -RpsPyt_Agenda);
  rps_object_put_payload(obj, agenpayl);
#warning rpsldpy_agenda incomplete
  printf ("incomplete rpsldpy_agenda obj %s spix#%d (%s:%d) \n..jv=%s\n",
	  idbuf, spix, __FILE__, __LINE__,
	  json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
}				/* end rpsldpy_agenda */
