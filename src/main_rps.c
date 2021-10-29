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
bool rps_showing_types;
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
  {"show-types", 0, 0, G_OPTION_ARG_NONE, &rps_showing_types,
   "show type information", NULL},
  {NULL}
};

extern void rps_show_types_info (void);

//////////////////////////////////////////////////////////////////
/// C code can refer to root objects
#define RPS_INSTALL_ROOT_OB(Oid) RpsObject_t* RPS_ROOT_OB(Oid);
#include "generated/rps-roots.h"

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
  printf ("\t full git id: %s\n", RPS_GIT_ID);
  printf ("\t last git tag: %s\n", RPS_GIT_LASTTAG);
  printf ("\t last git commit: %s\n", RPS_GIT_LASTCOMMIT);
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
#if JANSSON_VERSION_HEX > 0x020d00
  printf ("\t Jansson library runtime version: %s (see digip.org/jansson/)\n",
	  jansson_version_str ());
#else
  printf ("\t Jansson library version: %s (see digip.org/jansson/)\n",
	  JANSSON_VERSION);
#endif
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
rps_show_types_info (void)
{
  printf ("\n *** types information %s:%d gitid %s *** \n",
	  __FILE__, __LINE__, rps_shortgitid);
  printf
    (" RPS_OIDBUFLEN=%d, RPS_NBDIGITS_OID_HI=%d, RPS_NBDIGITS_OID_LO=%d\n",
     RPS_OIDBUFLEN, RPS_NBDIGITS_OID_HI, RPS_NBDIGITS_OID_LO);
  printf ("sizeof \"%s\" = %zd\n", "_0cSUtWqTYdZ00mjeNS",
	  sizeof ("_0cSUtWqTYdZ00mjeNS"));
  printf ("strlen \"%s\" = %zd\n", "_0cSUtWqTYdZ00mjeNS",
	  strlen ("_0cSUtWqTYdZ00mjeNS"));
  printf ("sizeof \"_0abcdefghijABCDEFG\" = %zd\n",
	  sizeof ("_0abcdefghijABCDEFG"));
#define TYPEFMT_rps "%-58s:"
  printf (TYPEFMT_rps "   size  align   (bytes)\n", "**TYPE**");
#define EXPLAIN_TYPE(Ty) printf(TYPEFMT_rps " %5d %5d\n", #Ty,		\
				(int)sizeof(Ty), (int)__alignof__(Ty))

#define EXPLAIN_TYPE2(Ty1,Ty2) printf(TYPEFMT_rps " %5d %5d\n",	\
				      #Ty1 "," #Ty2,		\
				      (int)sizeof(Ty1,Ty2),	\
				      (int)__alignof__(Ty1,Ty2))

#define EXPLAIN_TYPE3(Ty1,Ty2,Ty3) printf(TYPEFMT_rps " %5d %5d\n",	\
					  #Ty1 "," #Ty2 "," #Ty3,	\
					  (int)sizeof(Ty1,Ty2,Ty3),	\
					  (int)__alignof__(Ty1,Ty2,Ty3))
#define EXPLAIN_TYPE4(Ty1,Ty2,Ty3,Ty4) printf(TYPEFMT_rps " %5d %5d\n",	\
					      #Ty1 "," #Ty2 "," #Ty3 "," #Ty4, \
					      (int)sizeof(Ty1,Ty2,Ty3,Ty4), \
					      (int)__alignof__(Ty1,Ty2,Ty3,Ty4))
  EXPLAIN_TYPE (int);
  EXPLAIN_TYPE (intptr_t);
  EXPLAIN_TYPE (short);
  EXPLAIN_TYPE (long);
  EXPLAIN_TYPE (float);
  EXPLAIN_TYPE (double);
  EXPLAIN_TYPE (long double);
  EXPLAIN_TYPE (char);
  EXPLAIN_TYPE (bool);
  EXPLAIN_TYPE (void *);
  EXPLAIN_TYPE (pthread_mutex_t);
  EXPLAIN_TYPE (pthread_cond_t);
  EXPLAIN_TYPE (RpsObject_t);
  EXPLAIN_TYPE (RpsOid_t);
  EXPLAIN_TYPE (RpsAttrTable_t);
#undef EXPLAIN_TYPE4
#undef EXPLAIN_TYPE3
#undef EXPLAIN_TYPE
#undef TYPEFMT_rps
  putchar ('\n');
  fflush (NULL);
  {
    RpsOid_t oidr = rps_random_valid_oid ();
    char idrbuf[32];
    memset (idrbuf, 0, sizeof (idrbuf));
    rps_oid_to_cbuf (oidr, idrbuf);
    printf ("random id {id_hi=%ld,id_lo=%ld} %s (%s:%d)\n",
	    oidr.id_hi, oidr.id_lo, idrbuf, __FILE__, __LINE__);
    const char *end = NULL;
    RpsOid_t oidrbis = rps_cstr_to_oid (idrbuf, &end);
    char idbisbuf[32];
    memset (idbisbuf, 0, sizeof (idbisbuf));
    rps_oid_to_cbuf (oidrbis, idbisbuf);
    printf ("oidrbis   {id_hi=%ld,id_lo=%ld} %s (%s:%d)\n",
	    oidr.id_hi, oidr.id_lo, idbisbuf, __FILE__, __LINE__);
  }
  {
    const char idstr1[] = "_0J1C39JoZiv03qA2H9";
    const char *end = NULL;
    printf
      ("\"%s\" : strlen=%ld, size=%zd, RPS_OIDBUFLEN=%d, RPS_NBDIGITS_OID_HI=%d, RPS_NBDIGITS_OID_LO=%d\n",
       idstr1, strlen (idstr1), sizeof (idstr1), RPS_OIDBUFLEN,
       RPS_NBDIGITS_OID_HI, RPS_NBDIGITS_OID_LO);
    RpsOid_t id1 = rps_cstr_to_oid (idstr1, &end);
    assert (end && *end == 0);
    char idbuf1[32];
    memset (idbuf1, 0, sizeof (idbuf1));
    rps_oid_to_cbuf (id1, idbuf1);
    printf ("idstr1=%s id1:{id_hi=%ld,id_lo=%ld} hash %u idbuf1=%s\n",
	    idstr1, id1.id_hi, id1.id_lo, rps_oid_hash (id1), idbuf1);
  }
}				/* end rps_show_types_info */




/// nearly copied from Ian Lance Taylor's libbacktrace/print.c
/// see https://github.com/ianlancetaylor/libbacktrace
struct rps_print_backtrace_data_st
{
  struct backtrace_state *state;
  FILE *f;
};
/* Print one level of a backtrace.  */

static int
rps_printbt_callback (void *data, uintptr_t pc, const char *filename,
		      int lineno, const char *function)
{
  struct rps_print_backtrace_data_st *pdata
    = (struct rps_print_backtrace_data_st *) data;

  const char *funame = function;
  char nambuf[80];
  memset (nambuf, 0, sizeof (nambuf));
  if (funame)
    {
      fprintf (pdata->f, "0x%lx %s\n", (unsigned long) pc, funame);
    }
  else				/* no funame */
    {
      Dl_info di;
      memset (&di, 0, sizeof (di));
      if (dladdr ((void *) pc, &di))
	{
	  if (di.dli_sname)
	    {
	      fprintf (pdata->f, "0x%lx @%s+%#lx\n",
		       (unsigned long) pc, di.dli_sname,
		       (char *) pc - (char *) di.dli_saddr);
	    }
	  else if (di.dli_fname)
	    {
	      fprintf (pdata->f, "0x%lx @@%s+%#lx\n",
		       (unsigned long) pc, basename (di.dli_fname),
		       (char *) pc - (char *) di.dli_fbase);
	    }
	  else
	    fprintf (pdata->f, "0x%lx ?-?\n", (unsigned long) pc);
	}
      else
	fprintf (pdata->f, "0x%lx ???\n", (unsigned long) pc);
    }
  if (filename)
    fprintf (pdata->f, "\t%s:%d\n", basename (filename), lineno);
  return 0;
}				/* end rps_printbt_callback */

/* Print errors to stderr.  */

static void
rps_errorbt_callback (void *data_
		      __attribute__((unused)), const char *msg, int errnum)
{
  //  struct print_data_BM *pdata = (struct print_data_BM *) data;

  //if (pdata->state->filename != NULL)
  //  fprintf (stderr, "%s: ", pdata->state->filename);
  if (errnum > 0)
    fprintf (stderr, "libbacktrace:: %s (%s)", msg, strerror (errnum));
  else
    fprintf (stderr, "libbacktrace: %s", msg);
  fflush (stderr);
}				/* end rps_errorbt_callback   */

void
rps_backtrace_print (struct backtrace_state *state, int skip, FILE * f)
{
  struct rps_print_backtrace_data_st data;

  data.state = state;
  data.f = f;
  backtrace_full (state, skip + 1, rps_printbt_callback, rps_errorbt_callback,
		  (void *) &data);
}				/* end backtrace_print_BM */

void
rps_fatal_stop_at (const char *fil, int lineno)
{
  char thnambuf[16];
  memset (thnambuf, 0, sizeof (thnambuf));
  pthread_getname_np (pthread_self (), thnambuf, sizeof (thnambuf));
  fprintf (stderr, "** FATAL STOP %s:%d (tid#%d/%s) - shortgitid %s\n",
	   fil ? fil : "???", lineno, (int) rps_gettid (), thnambuf,
	   rps_shortgitid);
  fflush (stderr);
  if (rps_backtrace_common_state)
    {
      rps_backtrace_print (rps_backtrace_common_state, 1, stderr);
    }
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
  if (rps_showing_types)
    rps_show_types_info ();
  rps_initialize_objects_machinery ();
  if (!rps_load_directory)
    rps_load_directory = rps_topdirectory;
  rps_load_initial_heap ();
}				/* end of main function */



/************** end of file main_rps.c ****************/