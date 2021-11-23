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

/// Read the important comment about pthreads below....


/*****************************************************************
 *            ABOUT PTHREADS IN RefPerSys
 *
 * We have a fixed amount of pthreads in RefPerSys, and their number
 * is called the number of jobs NJ, and should be settable by some
 * program option, e.g. --jobs=4; that number of jobs should be more
 * than two and less than twenty and less than one more than the
 * number of cores in the CPU.
 *
 * + The main thread, which is loading the heap, and later running the
 * GTK event loop. That main thread can use RPS_ALLOC_ZONE. The GTK
 * event loop accept some GTK updating requests on some pipe from
 * agenda threads.  So we should use one pipe(7) to accept "requests"
 * from agenda threads which wants to display or draw some GTK widget.
 *
 * + Perhaps extra hidden GTK related threads.  These cannot use
 * RPS_ALLOC_ZONE at all.  These threads are not known to RefPerSys,
 * and are supposed to be idle most of the time; they are started and
 * stopped by some internal GTK code.  It might be possible that some
 * versions of GTK are using hidden threads for the clipboard or for
 * large copy/paste into GtkTextView etc...  The reader is requested to
 * dive into the source code of GTK.
 *
 * + The NJ agenda threads.  These threads can use RPS_ALLOC_ZONE but
 * cannot use any GTK routines directly. For any GTK updating
 * requests, an agenda thread should write on the pipe(7) used by the
 * main thread and known by GTK.
 *****************************************************************/


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
    RPS_FATAL
      ("rpsldpy_agenda obj %s already called %d times spix#%d\n..jv=%s",
       idbuf, count, spix, json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
  RpsAgenda_t *agenpayl		//
    = RPS_ALLOC_ZONE (sizeof (RpsAgenda_t), -RpsPyt_Agenda);
  rps_object_put_payload (obj, agenpayl);
#warning rpsldpy_agenda incomplete
  printf ("incomplete rpsldpy_agenda obj %s spix#%d (%s:%d) \n..jv=%s\n",
	  idbuf, spix, __FILE__, __LINE__,
	  json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
}				/* end rpsldpy_agenda */



void
rps_run_agenda(int nbthreads)
{
  #warning rps_run_agenda unimplemented
  RPS_FATAL("unimplemented rps_run_agenda %d threads", nbthreads);
} /* end rps_run_agenda */

