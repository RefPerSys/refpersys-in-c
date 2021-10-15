/****************************************************************
 * file main_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It has the main function and related, program option parsing,
 *      code.
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

/********* global variables ********/
bool rps_running_in_batch;

struct backtrace_state *rps_backtrace_common_state;
const char *rps_progname;
void *rps_dlhandle;

void
rps_backtrace_error_cb (void *data, const char *msg, int errnum)
{
#warning rps_backtrace_error_cb needs a lot of improvement
  fprintf (stderr, "Refpersys Backtrace Error #%d: %s\n", errnum, msg);
}

int
main (int argc, char **argv)
{
  bool show_version_info = false;
  rps_progname = argv[0];
  rps_backtrace_common_state =
    backtrace_create_state (rps_progname, (int) true,
			    rps_backtrace_error_cb, NULL);
  if (!rps_backtrace_common_state)
    {
      fprintf (stderr, "%s failed to make backtrace state.\n", rps_progname);
      exit (EXIT_FAILURE);
    };
  rps_dlhandle = dlopen (NULL, RTLD_NOW);
  if (!rps_dlhandle)
    {
      fprintf (stderr, "%s failed to whole-program dlopen.\n", rps_progname);
      exit (EXIT_FAILURE);
    };
  for (int aix = 1; aix < argc; aix++)
    {
      if (!strcmp (argv[aix], "--batch") || !strcmp (argv[aix], "-B"))
	rps_running_in_batch = true;
      else if (!strcmp (argv[aix], "--version"))
	show_version_info = true;
    }
  pthread_setname_np (pthread_self (), "rps-main");
  if (!rps_running_in_batch)
    gtk_init (&argc, &argv);
  curl_global_init (CURL_GLOBAL_ALL);
  if (show_version_info)
    {
      printf ("%s - a Reflexive Persistent System - see refpersys.org\n",
	      rps_progname);
      printf
	("... is an open source symbolic artificial intelligence project.\n");
      printf ("\t email contact: <team@refpersys.org>\n");
      printf ("\t build timestamp: %s (%ld)\n", rps_timestamp, rps_timelong);
      printf ("\t top directory: %s\n", rps_topdirectory);
      printf ("\t short git id: %s\n", rps_shortgitid);
      printf ("\t full git id: %s\n", rps_gitid);
      printf ("\t last git tag: %s\n", rps_lastgittag);
      printf ("\t last git commit: %s\n", rps_lastgitcommit);
      printf ("\t md5sum of files: %s\n", rps_md5sum);
      printf ("\t build makefile: %s\n", rps_makefile);
      printf ("\t built with compiler: %s\n", rps_c_compiler_version);
      if (!rps_running_in_batch)
	printf ("\t GTK version: %d.%d.%d (see gtk.org)\n",
		gtk_get_major_version (), gtk_get_minor_version (),
		gtk_get_micro_version ());
      printf ("\t GNU libc version: %s (see www.gnu.org/software/libc)\n",
	      gnu_get_libc_version ());
      printf ("\t libcurl version: %s (see curl.se/libcurl)\n",
	      curl_version ());
      printf ("\t " __FILE__ " was compiled on " __DATE__ " at " __TIME__
	      "\n");
      printf ("\t Subdirectories:\n");
      for (int sdix = 0; rps_subdirectories[sdix] != NULL; sdix++)
	printf ("\t   %s\n", rps_subdirectories[sdix]);
      printf ("\t Files:\n");
      for (int fix = 0; rps_files[fix] != NULL; fix++)
	printf ("\t   %s\n", rps_files[fix]);
    }
}				/* end of main function */
