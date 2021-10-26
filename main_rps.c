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
bool rps_showing_version;
bool rps_with_gui;
struct backtrace_state *rps_backtrace_common_state;
const char *rps_progname;
void *rps_dlhandle;
const char *rps_load_directory;
GOptionEntry rps_gopt_entries[] = {
  {"load-directory", 'L', 0, G_OPTION_ARG_FILENAME, &rps_load_directory,
   "load persistent heap from directory DIR", "DIR"},
  {"batch", 'B', 0, G_OPTION_ARG_NONE, &rps_running_in_batch,
   "run in batch mode, without user interface", NULL},
  {"version", 0, 0, G_OPTION_ARG_NONE, &rps_showing_version,
   "show version information and default options", NULL},
  {NULL}
};

pthread_t rps_main_thread_handle;

void
rps_backtrace_error_cb (void *data, const char *msg, int errnum)
{
#warning rps_backtrace_error_cb needs a lot of improvement
  fprintf (stderr, "Refpersys Backtrace Error #%d: %s\n", errnum, msg);
}

const char *
rps_hostname (void)
{
  static char hnambuf[64];
  if (!hnambuf[0])
    gethostname (hnambuf, sizeof (hnambuf) - 1);
  return hnambuf;
}				// end rps_hostname

void
rps_show_version_info (int argc, char **argv)
{
  struct utsname uts = { };
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
  printf ("\t git remote origin URL: %s\n", rps_gitremoteoriginurl);
  printf ("\t md5sum of files: %s\n", rps_md5sum);
  printf ("\t build makefile: %s\n", rps_makefile);
  printf ("\t built with compiler: %s\n", rps_c_compiler_version);
  if (!rps_running_in_batch)
    printf ("\t GTK version: %d.%d.%d (see gtk.org)\n",
	    gtk_get_major_version (), gtk_get_minor_version (),
	    gtk_get_micro_version ());
  printf ("\t GNU libc version: %s (see www.gnu.org/software/libc)\n",
	  gnu_get_libc_version ());
  printf ("\t Jansson library version: %s (see digip.org/jansson/)\n",
	  jansson_version_str ());
  printf ("\t libcurl version: %s (see curl.se/libcurl)\n", curl_version ());
  printf ("\t " __FILE__ " was compiled on " __DATE__ " at " __TIME__ "\n");
  printf ("\t Subdirectories:\n");
  for (int sdix = 0; rps_subdirectories[sdix] != NULL; sdix++)
    printf ("\t   %s\n", rps_subdirectories[sdix]);
  printf ("\t Files:\n");
  for (int fix = 0; rps_files[fix] != NULL; fix++)
    printf ("\t   %s\n", rps_files[fix]);
  printf ("\t Current host: %s\n", rps_hostname ());
  if (!uname (&uts))
    printf ("\t This OS: %s, release %s, version %s\n",
	    uts.sysname, uts.release, uts.version);
}				/* end rps_show_version_info */

void
rps_fatal_stop_at (const char *fil, int lineno)
{
  char thnambuf[16];
  memset (thnambuf, 0, sizeof (thnambuf));
  pthread_getname_np (pthread_self (), thnambuf, sizeof (thnambuf));
  fprintf (stderr, "** FATAL STOP %s:%d (tid#%d/%s)\n",
	   fil ? fil : "???", lineno, (int) rps_gettid (), thnambuf);
  fflush (stderr);
  rps_abort ();
}				/* end rps_fatal_stop */

pid_t
rps_gettid (void)
{
  return syscall (SYS_gettid, 0L);
}				/* end rps_gettid */


double
rps_clocktime (clockid_t clid)
{
  struct timespec ts = { 0, 0 };
  if (clock_gettime (clid, &ts))
    return NAN;
  return (double) ts.tv_sec + 1.0e-9 * ts.tv_nsec;
}

void
rps_abort (void)
{
  abort ();
}				/* end rps_abort */

int
main (int argc, char **argv)
{
  rps_progname = argv[0];
  rps_main_thread_handle = pthread_self ();
  pthread_setname_np (rps_main_thread_handle, "rps-main");
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
      fprintf (stderr, "%s failed to whole-program dlopen (%s).\n",
	       rps_progname, dlerror ());
      exit (EXIT_FAILURE);
    };
  curl_global_init (CURL_GLOBAL_ALL);
  GError *argperr = NULL;
  rps_with_gui =
    gtk_init_with_args (&argc, &argv,
			"\n** RefPerSys - a symbolic artificial intelligence system. See refpersys.org **",
			rps_gopt_entries, NULL, &argperr);
  if (argperr)
    {
      fprintf (stderr, "%s: failed to parse program arguments (#%d) : %s\n",
	       rps_progname, argperr->code, argperr->message);
      exit (EXIT_FAILURE);
    }
  if (rps_showing_version)
    {
      rps_show_version_info (argc, argv);
      exit (EXIT_SUCCESS);
    };
  rps_initialize_objects_machinery ();
  if (!rps_load_directory)
    rps_load_directory = rps_topdirectory;
  rps_load_initial_heap ();
}				/* end of main function */



/************** end of file main_rps.c ****************/
