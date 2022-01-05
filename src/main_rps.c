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
/* for mallopt: */
#include <malloc.h>
/* https://www.gnu.org/software/libc/manual/html_node/Customizing-Printf.html */
#include <printf.h>
/* for KLIB,  Adelson-Velsky and Landis generic balanced trees from
    http://attractivechaos.github.io/klib/ see also
    https://en.wikipedia.org/wiki/AVL_tree */
#include "kavl.h"
/********* global variables ********/
bool rps_running_in_batch;
bool rps_showing_version;
bool rps_showing_types;
bool rps_showing_debug_help;
bool rps_with_gui;

/* The following terminal globals are declared in include/terminal_rps.h */
bool rps_terminal_is_escaped;
bool rps_terminal_has_stderr;
bool rps_terminal_has_stdout;
unsigned rps_debug_flags;

struct backtrace_state *rps_backtrace_common_state;
const char *rps_progname;
void *rps_dlhandle;
const char *rps_load_directory;
const char *rps_dump_directory;
const char *rps_debug_str_load;
const char *rps_debug_str_after;

int rps_nb_threads;
GOptionEntry rps_gopt_entries[] = {
  {"load-directory", 'L', 0, G_OPTION_ARG_FILENAME, &rps_load_directory,
   "load persistent heap from directory DIR", "DIR"},
  {"batch", 'B', 0, G_OPTION_ARG_NONE, &rps_running_in_batch,
   "run in batch mode, without user interface", NULL},
  {"version", 0, 0, G_OPTION_ARG_NONE, &rps_showing_version,
   "show version information and default options", NULL},
  {"show-types", 0, 0, G_OPTION_ARG_NONE, &rps_showing_types,
   "show type information and more (some random oids)", NULL},
  {"dump", 'D', 0, G_OPTION_ARG_FILENAME, &rps_dump_directory,
   "dump heap into directory DIR", "DIR"},
  {"nb-threads", 'T', 0, G_OPTION_ARG_INT, &rps_nb_threads,
   "set number of agenda threads to NBTHREADS", "NBTHREADS"},
  {"debug-load", 0, 0, G_OPTION_ARG_STRING, &rps_debug_str_load,
   "set debugging flags for loading to DBGFLAGS", "DBGFLAGS"},
  {"debug-after", 0, 0, G_OPTION_ARG_STRING, &rps_debug_str_after,
   "set debugging flags after loading to DBGFLAGS", "DBGFLAGS"},
  {"debug-help", 0, 0, G_OPTION_ARG_NONE, &rps_showing_debug_help,
   "show possible debug levels", NULL},
  {"gui", 'G', 0, G_OPTION_ARG_NONE, &rps_with_gui,
   "start a graphical interface with GTK", NULL},
  {NULL}
};

extern void rps_show_types_info (void);



//////////////////////////////////////////////////////////////////
#define RPS_PRINT_MAX_DEPTH 6
//// Our printf customization: %V prints a value
//// See www.gnu.org/software/libc/manual/html_node/Customizing-Printf.html
////
//// An internal recursive printing function
int
rps_rec_print_value (FILE * outf, const struct printf_info *info,
		     RpsValue_t val, unsigned depth)
{
  if (val == RPS_NULL_VALUE)
    {
      if (fputs ("__", outf) < 0)
	return -1;
      else
	return 2;
    };
  if (depth > RPS_PRINT_MAX_DEPTH)
    return fputs ("?...?", outf);
  switch (rps_value_type (val))
    {
    case RPS_TYPE_INT:		/* tagged int */
      {
	intptr_t iv = rps_tagged_integer_value (val);
	int ln = fprintf (outf, "%lld", (long long) iv);
	return ln;
      }
    case RPS_TYPE_DOUBLE:
      {
	double x = rps_double_value (val);
	int ln = fprintf (outf, "#%g", x);
	return ln;
      }
    case RPS_TYPE_STRING:
      {
	const char *str = rps_stringv_utf8bytes (val);
	RPS_ASSERT (g_utf8_validate (str, -1, NULL));
	if (fputc ('"', outf) < 0)
	  return -1;
	int ln = 1;
	for (const char *pc = str; *pc; pc = g_utf8_next_char (pc))
	  {
	    gunichar uc = g_utf8_get_char (pc);
	    switch (uc)
	      {
	      case '\"':
		if (fputs ("\\\"", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      case '\'':
		if (fputs ("\\'", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      case '\\':
		if (fputs ("\\\\", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      case '\n':
		if (fputs ("\\n", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      case '\r':
		if (fputs ("\\r", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      case '\t':
		if (fputs ("\\t", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      case '\v':
		if (fputs ("\\v", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      case '\f':
		if (fputs ("\\f", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      case '\b':
		if (fputs ("\\b", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      case '\e':
		if (fputs ("\\e", outf) < 0)
		  return -1;
		ln += 2;
		break;
	      default:
		if (uc < 127)
		  {
		    if (fputc ((char) uc, outf) < 0)
		      return -1;
		    ln += 1;
		  }
		else
		  {
		    char cbuf[16];
		    memset (cbuf, 0, sizeof (cbuf));
		    int nby = g_utf8_next_char (pc) - pc;
		    strncpy (cbuf, pc, nby);
		    int lc = fputs (cbuf, outf);
		    if (lc < 0)
		      return -1;
		    ln += lc;
		  }
		break;
	      }
	  }
	if (fputc ('"', outf) < 0)
	  return -1;
	return ln + 1;
      }
    case RPS_TYPE_JSON:
      {
	const json_t *js = rps_json_value (val);
	RPS_ASSERT (js != NULL);
	char *jbuf =
	  json_dumps (js, JSON_COMPACT | JSON_SORT_KEYS | JSON_ENCODE_ANY);
	int ln = fprintf (outf, "JSON %s", jbuf);
	free (jbuf);
	return ln;
      }
    case RPS_TYPE_GTKWIDGET:
      {
	GtkWidget *widg = rps_gtk_widget_value (val);
	RPS_ASSERT (widg != NULL);
	GtkWidgetClass *wcla = GTK_WIDGET_GET_CLASS (widg);
	RPS_ASSERT (wcla != NULL);
	char buf[64];
	memset (buf, 0, sizeof (buf));
	snprintf (buf, sizeof (buf), "%s@%p",
		  gtk_widget_class_get_css_name (wcla), widg);
	int ln = fprintf (outf, "GTKWIDGET %s", buf);
	return ln;
      }
    case RPS_TYPE_TUPLE:
      {
	const RpsTupleOb_t *tupv = (const RpsTupleOb_t *) val;
	int ln = 1;
	if (fputc ('[', outf) < 0)
	  return -1;
	int tsiz = (int) rps_vtuple_size (tupv);
	for (int tix = 0; tix < tsiz; tix++)
	  {
	    const RpsObject_t *compob = rps_vtuple_nth (tupv, tix);
	    if (tix > 0)
	      {
		if (fputc (',', outf) < 0)
		  return ln;
		ln++;
	      };
	    if (!compob)
	      {
		if (fputc ('_', outf) < 0)
		  return ln;
		ln++;
	      }
	    else
	      {
		char bufid[32];
		memset (bufid, 0, sizeof (bufid));
		rps_oid_to_cbuf (compob->ob_id, bufid);
		if (fputs (bufid, outf) < 0)
		  return ln;
		ln += strlen (bufid);
	      }
	  }			/* end for tix in tupv */
	if (fputc (']', outf) < 0)
	  return -1;
	return ln + 1;
      }
    case RPS_TYPE_SET:
      {
	const RpsSetOb_t *setv = (const RpsSetOb_t *) val;
	int card = (int) rps_set_cardinal (setv);
	int ln = 1;
	if (fputc ('{', outf) < 0)
	  return -1;
	for (int eix = 0; eix < card; eix++)
	  {
	    const RpsObject_t *obelem = rps_set_nth_member (setv, eix);
	    RPS_ASSERT (obelem != NULL
			&& rps_is_valid_object ((RpsObject_t *) obelem));
	    if (eix > 0)
	      {
		if (fputc (';', outf) < 0)
		  return ln;
		ln++;
	      };
	    char bufid[32];
	    memset (bufid, 0, sizeof (bufid));
	    rps_oid_to_cbuf (obelem->ob_id, bufid);
	    if (fputs (bufid, outf) < 0)
	      return ln;
	    ln += strlen (bufid);
	  }
	if (fputc ('}', outf) < 0)
	  return -1;
	return ln + 1;
      }
    case RPS_TYPE_CLOSURE:
      {
	const RpsClosure_t *closv = (const RpsClosure_t *) val;
	const RpsObject_t *connob = rps_closure_connective (val);
	RpsValue_t metav = rps_closure_meta (val);
	int csiz = (int) rps_closure_size (val);
	RPS_ASSERT (rps_is_valid_object ((RpsObject_t *) connob));
	char bufid[32];
	memset (bufid, 0, sizeof (bufid));
	rps_oid_to_cbuf (connob->ob_id, bufid);
	int ln = fprintf (outf, "CLOSURE %s", bufid);
	if (ln < 0)
	  return -1;
	if (metav != RPS_NULL_VALUE)
	  {
	    if (fputs ("µ", outf) > 0)
	      ln += strlen ("µ");
	    else
	      return -1;
	    int lmeta = rps_rec_print_value (outf, info, metav, 2 + depth);
	    if (lmeta > 0)
	      ln += lmeta;
	  };
	if (fputs ("(", outf) > 0)
	  ln += 1;
	else
	  return -1;
	for (int cix = 0; cix < csiz; cix++)
	  {
	    RpsValue_t clv = rps_closure_get_closed_value (val, cix);
	    if (cix > 0)
	      if (fputs (",", outf) > 0)
		ln += 1;
	      else
		return -1;
	    int lclo = rps_rec_print_value (outf, info, clv, 1 + depth);
	    if (lclo < 0)
	      return -1;
	    ln += lclo;
	  }
	if (fputs (")", outf) > 0)
	  ln += 1;
	else
	  return -1;
	return ln;
      }
    case RPS_TYPE_OBJECT:
      {
	const RpsObject_t *ob = (const RpsObject_t *) val;
	char bufid[32];
	memset (bufid, 0, sizeof (bufid));
	rps_oid_to_cbuf (ob->ob_id, bufid);
	int lob = fputs (bufid, outf);
	if (lob < 0)
	  return -1;
	return lob;
      }
    case RPS_TYPE_FILE:
      {
	const RpsFile_t *filv = (const RpsFile_t *) val;
	FILE *fh = filv->fileh;
	int fd = fileno (fh);
	if (fd > 0)
	  return fprintf (outf, "FILE#%d", fd);
	else
	  return fprintf (outf, "FILE@%p", filv);
      }
    default:
      return fprintf (outf, "BOGUS %p", (void *) val);
    }
}				/* end rps_rec_print_value */

//// Our printf customization: %V prints a value
//// See www.gnu.org/software/libc/manual/html_node/Customizing-Printf.html
int
rps_custom_print_value (FILE * outf, const struct printf_info *info,
			const void *const *args)
{
  RpsValue_t val = (*(const RpsValue_t **) (args[0]));
  return rps_rec_print_value (outf, info, val, 0);
}

int
rps_custom_arginfo_value (const struct printf_info *info, size_t n,
			  int *argtypes)
{
  if (n > 0)			/* for intptr_t */
    argtypes[0] = PA_INT | PA_FLAG_PTR;
  return 1;
}				/* end rps_custom_arginfo_value */

//// printf customization: %O prints an object by its oid
int
rps_custom_print_object (FILE * outf, const struct printf_info *info,
			 const void *const *args)
{
  RpsObject_t *ob = (*(const RpsObject_t **) (args[0]));
  if (!ob)
    {
      fputs ("__", outf);
      return 2;
    }
  else if (ob == RPS_HTB_EMPTY_SLOT)
    {
      fputs ("_?", outf);
      return 2;
    };
  RPS_ASSERT (rps_is_valid_object (ob));
  char idbuf[32];
  memset (idbuf, 0, sizeof (idbuf));
  rps_oid_to_cbuf (ob->ob_id, idbuf);
  fputs (idbuf, outf);
  int ln = strlen (idbuf);
#warning we may want to add into rps_custom_print_object code to print name of objects with %*O
  return ln;
}				/* end rps_custom_print_object */

int
rps_custom_arginfo_object (const struct printf_info *info, size_t n,
			   int *argtypes)
{
  /* We always take exactly one argument and this is a pointer to the
     RpsObject_st structure.. */
  if (n > 0)
    argtypes[0] = PA_POINTER;
  return 1;
}				/* end rps_custom_arginfo_object */

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
rps_set_debug (const char *dbglev)
{
  if (!dbglev)
    return;
  const char *comma = NULL;
  char dbglevstr[32];
  for (const char *pc = dbglev; pc; pc = comma ? (comma + 1) : NULL)
    {
      comma = strchr (pc, ',');
      memset (dbglevstr, 0, sizeof (dbglevstr));
      if (comma)
	strncpy (dbglevstr, pc,
		 ((comma - pc) <
		  sizeof (dbglevstr)) ? (comma - pc) : (sizeof (dbglevstr) -
							1));
      else
	strncpy (dbglevstr, pc, sizeof (dbglevstr) - 1);
      dbglevstr[sizeof (dbglevstr) - 1] = (char) 0;
#define RPS_SETDEBUGMAC(Opt) if (!strcmp(#Opt,dbglevstr)) {	\
      rps_debug_flags |= 1 << RPS_DEBUG_##Opt;			\
      printf("debug flag " #Opt "\n");				\
    }
      RPS_DEBUG_OPTIONS (RPS_SETDEBUGMAC);
#undef RPS_SETDEBUGMAC
    }
}				/* end rps_set_debug */


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
  printf ("\t short git id: %s\n", _rps_git_short_id);
  printf ("\t full git id: %s\n", _rps_git_id);
  printf ("\t last git tag: %s\n", _rps_git_last_tag);
  printf ("\t last git commit: %s\n", _rps_git_last_commit);
  printf ("\t git remote origin URL: %s\n", _rps_git_remote_origin_url);
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
  printf
    ("\t Jansson library runtime version: %s (see digip.org/jansson/)\n",
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
  struct utsname uts;
  memset (&uts, 0, sizeof (uts));
  uname (&uts);
  printf ("\n *** types information %s:%d gitid %s *** \n",
	  __FILE__, __LINE__, _rps_git_short_id);
  printf ("uts: sysname=%s nodename=%s release=%s version='%s' machine=%s\n",
	  uts.sysname, uts.nodename, uts.release, uts.version, uts.machine);
  printf
    (" RPS_OID_BUFLEN=%d, RPS_OID_HI_NBDIGITS=%d, RPS_NBDIGITS_OID_LO=%d\n",
     RPS_OID_BUFLEN, RPS_OID_HI_NBDIGITS, RPS_NBDIGITS_OID_LO);
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
  EXPLAIN_TYPE (jmp_buf);	/* for longjmp/setjmp */
  EXPLAIN_TYPE (struct RpsZonedMemory_st);
  EXPLAIN_TYPE (struct RpsZonedValue_st);
  EXPLAIN_TYPE (struct rps_owned_payload_st);
  EXPLAIN_TYPE (struct RpsPayl_AttrTable_st);
  EXPLAIN_TYPE (struct rps_callframedescr_st);
  EXPLAIN_TYPE (struct rps_protocallframe_st);
  EXPLAIN_TYPE (pthread_mutex_t);
  EXPLAIN_TYPE (pthread_cond_t);
  EXPLAIN_TYPE (RpsObject_t);
  EXPLAIN_TYPE (RpsOid);
  EXPLAIN_TYPE (RpsAttrTable_t);
  EXPLAIN_TYPE (RpsSetOb_t);
  EXPLAIN_TYPE (RpsString_t);
  EXPLAIN_TYPE (RpsDouble_t);
  EXPLAIN_TYPE (RpsJson_t);
  EXPLAIN_TYPE (RpsTupleOb_t);
  EXPLAIN_TYPE (RpsGtkWidget_t);
  EXPLAIN_TYPE (RpsClosure_t);
  EXPLAIN_TYPE (RpsSymbol_t);
  EXPLAIN_TYPE (RpsMutableSetOb_t);
  EXPLAIN_TYPE (struct internal_symbol_node_rps_st);
  EXPLAIN_TYPE (struct internal_mutable_set_ob_node_rps_st);
#undef EXPLAIN_TYPE4
#undef EXPLAIN_TYPE3
#undef EXPLAIN_TYPE
#undef TYPEFMT_rps
  putchar ('\n');
  fflush (NULL);
  /// seven random oid-s for testing....
  for (int cnt = 0; cnt < 7; cnt++)
    {
      RpsOid oidr = rps_oid_random ();
      char idrbuf[32];
      memset (idrbuf, 0, sizeof (idrbuf));
      rps_oid_to_cbuf (oidr, idrbuf);
      printf
	("random id#%d {id_hi=%018ld,id_lo=%018ld} %s h%#08x (%s:%d)\n",
	 cnt, oidr.id_hi, oidr.id_lo, idrbuf, rps_oid_hash (oidr), __FILE__,
	 __LINE__);
      const char *end = NULL;
      RpsOid oidrbis = rps_cstr_to_oid (idrbuf, &end);
      char idbisbuf[32];
      memset (idbisbuf, 0, sizeof (idbisbuf));
      rps_oid_to_cbuf (oidrbis, idbisbuf);
      printf ("oidrbis#%d   {id_hi=%018ld,id_lo=%018ld} %s (%s:%d)\n",
	      cnt, oidr.id_hi, oidr.id_lo, idbisbuf, __FILE__, __LINE__);
      fflush (NULL);
    }
  /// oidstrs for testing
  const char *rootarridstr[] = {
#define RPS_INSTALL_ROOT_OB(Oid) #Oid,
#include "generated/rps-roots.h"
    NULL
  };
  for (int rix = 0; rix < RPS_NB_ROOT_OB; rix++)
    {
      const char *curidstr = rootarridstr[rix];
      RPS_ASSERT (curidstr != NULL);
      const char *end = NULL;
      printf ("testing rix#%d curidstr %s (%s:%d)\n", rix, curidstr,
	      __FILE__, __LINE__);
      RpsOid curidroot = rps_cstr_to_oid (curidstr, &end);
      RPS_ASSERTPRINTF (rps_oid_is_valid (curidroot), "rix#%d rootstr %s",
			rix, curidstr);
      RPS_ASSERTPRINTF (end && *end == 0, "rix#%d rootstr %s bad end", rix,
			curidstr);
      if (rix % 7 == 0)
	{
	  char curbuf[32];
	  memset (curbuf, 0, sizeof (curbuf));
	  rps_oid_to_cbuf (curidroot, curbuf);
	  printf
	    ("rix#%d %s hash%#08x {id_hi=%018ld,id_lo=%018ld} %s (%s:%d)\n",
	     rix, curidstr, rps_oid_hash (curidroot),
	     curidroot.id_hi, curidroot.id_lo, curbuf, __FILE__, __LINE__);
	  RPS_ASSERT (!strcmp (curidstr, curbuf));
	}
    };
  fflush (NULL);
  {
    const char idstr1[] = "_0J1C39JoZiv03qA2H9";
    const char *end = NULL;
    printf
      ("\"%s\" : strlen=%ld, size=%zd, RPS_OID_BUFLEN=%d, RPS_NBDIGITS_OID_HI=%d, RPS_NBDIGITS_OID_LO=%d\n",
       idstr1, strlen (idstr1), sizeof (idstr1), RPS_OID_BUFLEN,
       RPS_OID_HI_NBDIGITS, RPS_NBDIGITS_OID_LO);
    RpsOid id1 = rps_cstr_to_oid (idstr1, &end);
    assert (end && *end == 0);
    char idbuf1[32];
    memset (idbuf1, 0, sizeof (idbuf1));
    rps_oid_to_cbuf (id1, idbuf1);
    printf ("idstr1=%s id1:{id_hi=%ld,id_lo=%ld} hash %u idbuf1=%s\n",
	    idstr1, id1.id_hi, id1.id_lo, rps_oid_hash (id1), idbuf1);
  }
  printf ("prime above thirteen = %ld\n", rps_prime_above (13));
  printf ("prime below fiveteen = %ld\n", rps_prime_below (15));
  fflush (NULL);
  exit (EXIT_SUCCESS);
}				/* end rps_show_types_info */


/// Very probably, in the future, rps_value_type will disappear and be
/// replaced by similar C statements in generated C code. Before that
/// happens, do not make it inline without discussion on
/// <team@refpersys.org>
enum RpsType
rps_value_type (RpsValue_t val)
{
  if (val == RPS_NULL_VALUE)
    return RPS_TYPE__NONE;
  else if (val & 1)
    return RPS_TYPE_INT;
  return (enum RpsType) RPS_ZONED_MEMORY_TYPE (val);
}				/* end of rps_value_type */

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
  backtrace_full (state, skip + 1, rps_printbt_callback,
		  rps_errorbt_callback, (void *) &data);
}				/* end backtrace_print_BM */

void
rps_fatal_stop_at (const char *fil, int lineno)
{
  char thnambuf[16];
  memset (thnambuf, 0, sizeof (thnambuf));
  pthread_getname_np (pthread_self (), thnambuf, sizeof (thnambuf));
  fprintf (stderr, "** FATAL STOP %s:%d (tid#%d/%s) - shortgitid %s\n",
	   fil ? fil : "???", lineno, (int) rps_gettid (), thnambuf,
	   _rps_git_short_id);
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


static long rps_dbgcnt;

void
rps_debug_printf_at (const char *filname, int fline, enum Rps_Debug dbgopt,
		     const char *fmt, ...)
{
  if (((1 << dbgopt) & rps_debug_flags) == 0)
    return;
  if (!fmt)
    return;
  FILE *dbgf = stderr;
  bool wantbacktrace = false;
  if (fmt[0] == '|')
    {
      wantbacktrace = true;
      fmt++;
    };
  flockfile (dbgf);
  bool ismainth = rps_main_thread_handle == pthread_self ();
  char threadbfr[24];
  memset (threadbfr, 0, sizeof (threadbfr));
  if (!filname)
    filname = "?";
  if (fline < 0)
    {
      fputc ('\n', dbgf);
      fline = -fline;
    }
  if (ismainth)
    strcpy (threadbfr, "▬!$");	//U+25AC BLACK RECTANGLE
  else
    {
      char thrbuf[16];
      memset (thrbuf, 0, sizeof (thrbuf));
      pthread_getname_np (pthread_self (), thrbuf, sizeof (thrbuf) - 1);
      snprintf (threadbfr, sizeof (threadbfr),
		// U+2045 LEFT SQUARE BRACKET WITH QUILL
		"⁅%s:%d⁆"
		// U+2046 RIGHT SQUARE BRACKET WITH QUILL
		, thrbuf, (int) rps_gettid ());
    }
  if (rps_dbgcnt % 16 == 0)
    fputc ('\n', dbgf);
  fprintf (dbgf, "°%s°%s:%d#%ld;", threadbfr, filname, fline, rps_dbgcnt);
  switch (dbgopt)
    {
#define RPS_DEBUG_OUTOPTMAC(Dopt) case RPS_DEBUG_##Dopt: fputs(#Dopt,dbgf); break;
      RPS_DEBUG_OPTIONS (RPS_DEBUG_OUTOPTMAC)
#undef RPS_DEBUG_OUTOPTMAC
    }
  fprintf (dbgf, "@%.2f:", rps_clocktime (CLOCK_REALTIME));
  va_list arglst;
  va_start (arglst, fmt);
  vfprintf (dbgf, fmt, arglst);
  va_end (arglst);
  fputc ('\n', dbgf);
  if (wantbacktrace)
    {
      fputs ("|||\n", dbgf);
      rps_backtrace_print (rps_backtrace_common_state, 1, dbgf);
      fputc ('\n', dbgf);
    }
  fflush (dbgf);
  rps_dbgcnt++;
  funlockfile (dbgf);
}				/* end rps_debug_printf_at */



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



void
rps_emit_gplv3plus_notice (FILE * fil, const char *name,
			   const char *lineprefix, const char *linesuffix)
{
  time_t now = 0;
  time (&now);
  char yearbuf[8];
  struct tm nowtm = { };
  localtime_r (&now, &nowtm);
  memset (yearbuf, 0, sizeof (yearbuf));
  strftime (yearbuf, sizeof (yearbuf), "%Y", &nowtm);
  fprintf (fil,
	   "%s ---------------------------------------------------- %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s emitted file %.64s %s\n",
	   lineprefix ? lineprefix : "", name, linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s SPDX-License-Identifier: GPL-3.0-or-later %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s © Copyright 2019 - %s The Reflective Persistent System Team %s\n",
	   lineprefix ? lineprefix : "",
	   yearbuf, linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s team@refpersys.org & http://refpersys.org/ %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil, "%s %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s License: %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  This program is free software: you can redistribute it and/or modify %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  it under the terms of the GNU General Public License as published by %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  the Free Software Foundation, either version 3 of the License, or %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  (at your option) any later version. %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  This program is distributed in the hope that it will be useful, %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  but WITHOUT ANY WARRANTY; without even the implied warranty of %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  GNU General Public License for more details. %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  You should have received a copy of the GNU General Public License %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s  along with this program.  If not, see <http://www.gnu.org/licenses/>. %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
  fprintf (fil,
	   "%s ---------------------------------------------------- %s\n",
	   lineprefix ? lineprefix : "", linesuffix ? linesuffix : "");
}				/* end rps_emit_gplv3plus_notice */


static double rps_start_real_clock;
static double rps_start_cpu_clock;

void
rps_exit_handler (void)
{
  char threadname[32];
  memset (threadname, 0, sizeof (threadname));
  pthread_getname_np (pthread_self (), threadname, sizeof (threadname));
  printf ("\n"
	  "REFPERSYS git %s exiting process %d/%s on %s - %.2f real %.2f cpu\n",
	  _rps_git_short_id, (int) getpid (), threadname, rps_hostname (),
	  rps_clocktime (CLOCK_REALTIME) - rps_start_real_clock,
	  rps_clocktime (CLOCK_PROCESS_CPUTIME_ID) - rps_start_cpu_clock);
  fflush (NULL);
  if (rps_backtrace_common_state)
    {
      rps_backtrace_print (rps_backtrace_common_state, 1, stdout);
      fflush (NULL);
    }
}				/* end rps_exit_handler */

int
main (int argc, char **argv)
{
  rps_progname = argv[0];
  rps_start_real_clock = rps_clocktime (CLOCK_REALTIME);
  rps_start_cpu_clock = rps_clocktime (CLOCK_PROCESS_CPUTIME_ID);
  atexit (rps_exit_handler);
#warning temporary call to mallopt. Should be removed once loading and dumping completes.
  mallopt (M_CHECK_ACTION, 03);
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
  rps_allocation_initialize ();
  curl_global_init (CURL_GLOBAL_ALL);
  GError *argperr = NULL;
  /* CAVEAT: we need a valid $DISPLAY, even when running in --batch
     mode.  This should be improved by generating C code later, or
     parsing twice our program arguments [argc, argv]. */
  bool started = gtk_init_with_args (&argc, &argv,
				     "\n** RefPerSys - a symbolic artificial intelligence system. See refpersys.org **\n"
				     "[C variant on github.com/RefPerSys/refpersys-in-c ...]\n",
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
  if (rps_showing_debug_help)
    {
      printf ("%s: possible debug levels are", rps_progname);
#define RPS_DEBUG_OUTSHOWMAC(Dopt) fputs(" " #Dopt, stdout);
      RPS_DEBUG_OPTIONS (RPS_DEBUG_OUTSHOWMAC);
#undef RPS_DEBUG_OUTSHOWMAC
      putchar ('\n');
      fflush (NULL);
    };
  if (rps_showing_types)
    {
      rps_show_types_info ();
    };
  if (rps_nb_threads > 0)
    {
      if (rps_nb_threads < RPS_MIN_NB_THREADS)
	rps_nb_threads = RPS_MIN_NB_THREADS;
      else if (rps_nb_threads > RPS_MAX_NB_THREADS)
	rps_nb_threads = RPS_MAX_NB_THREADS;
    }
  rps_initialize_objects_machinery ();
#warning other payload routines should be registered here
  rps_register_payload_removal (RpsPyt_ClassInfo,
				rps_classinfo_payload_remover, NULL);
  rps_check_all_objects_buckets_are_valid ();
  if (!rps_load_directory)
    rps_load_directory = rps_topdirectory;
  if (rps_terminal_is_escaped)
    {
      rps_terminal_has_stderr = isatty (STDERR_FILENO);
      rps_terminal_has_stdout = isatty (STDOUT_FILENO);
    }
  if (rps_debug_str_load)
    {
      printf ("setting debug before load to %s\n", rps_debug_str_load);
      rps_set_debug (rps_debug_str_load);
    }
  rps_load_initial_heap ();
  if (rps_debug_str_after)
    {
      printf ("setting debug after load to %s\n", rps_debug_str_after);
      rps_set_debug (rps_debug_str_after);
    }
  if (rps_nb_threads > 0)
    rps_run_agenda (rps_nb_threads);
  if (rps_with_gui)
    rps_run_gui (&argc, argv);
  if (rps_dump_directory)
    rps_dump_heap (NULL, rps_dump_directory);
}				/* end of main function */


//// This function computes the closure to send a given method (by its
//// selector SELOB) to some non-nil value VAL.  It is inefficient, and
//// should be replaced by better generated C code.
RpsClosure_t *
rps_value_compute_method_closure (RpsValue_t val, const RpsObject_t * selob)
{
  RpsClosure_t *closres = NULL;
  char smallbuf[16];
  RpsOid clasid = { 0, 0 };
  RpsObject_t *clasob = NULL;
  const char *clidstr = NULL;
  if (val == RPS_NULL_VALUE)
    return NULL;
  if (!selob || !rps_is_valid_object ((RpsObject_t *) selob))
    return NULL;
  memset (smallbuf, 0, sizeof (smallbuf));
  switch (rps_value_type (val))
    {
    case RPS_TYPE_INT:
      clasob = RPS_ROOT_OB (_2A2mrPpR3Qf03p6o5b);	//int∈class
      clidstr = "_2A2mrPpR3Qf03p6o5b";
      break;
    case RPS_TYPE_DOUBLE:
      clasob = RPS_ROOT_OB (_98sc8kSOXV003i86w5);	//double∈class
      clidstr = "_98sc8kSOXV003i86w5";
      break;
    case RPS_TYPE_STRING:
      clasob = RPS_ROOT_OB (_62LTwxwKpQ802SsmjE);	//string∈class
      clidstr = "_62LTwxwKpQ802SsmjE";
      break;
    case RPS_TYPE_JSON:
      clasob = RPS_ROOT_OB (_3GHJQW0IIqS01QY8qD);	//json∈class
      clidstr = "_3GHJQW0IIqS01QY8qD";
      break;
    case RPS_TYPE_TUPLE:
      clasob = RPS_ROOT_OB (_6NVM7sMcITg01ug5TC);	//tuple∈class
      clidstr = "_6NVM7sMcITg01ug5TC";
      break;
    case RPS_TYPE_SET:
      clasob = RPS_ROOT_OB (_6JYterg6iAu00cV9Ye);	//set∈class
      clidstr = "_6JYterg6iAu00cV9Ye";
      break;
    case RPS_TYPE_CLOSURE:
      clasob = RPS_ROOT_OB (_4jISxMJ4PYU0050nUl);	//closure∈class
      clidstr = "_4jISxMJ4PYU0050nUl";
      break;
    case RPS_TYPE_OBJECT:
      {
	RpsObject_t *curob = (RpsObject_t *) val;
	pthread_mutex_lock (&curob->ob_mtx);
	clasob = curob->ob_class;
	pthread_mutex_unlock (&curob->ob_mtx);
      }
      break;
    case RPS_TYPE_FILE:
      clidstr = "?*file*?";
      goto unimplemented;
    case RPS_TYPE_GTKWIDGET:
      clidstr = "?*gtkwidget?*";
      goto unimplemented;
#warning rps_value_compute_method_closure unimplemented for GTKWIDGET and FILE
    default:
      snprintf (smallbuf, sizeof (smallbuf), "?*#%u#*?",
		(unsigned) rps_value_type (val));
      clidstr = smallbuf;
    unimplemented:
      RPS_FATAL
	("rps_value_compute_method_closure unimplemented for val@%p %s",
	 (void *) val, clidstr);
    }
  if (!clasob && clidstr && clidstr[0] == '_')
    {
      const char *end = NULL;
      clasid = rps_cstr_to_oid (clidstr, &end);
      if (end && *end == (char) 0)
	clasob = rps_find_object_by_oid (clasid);
    }
  int cnt = 0;
  // Even with buggy heap, we don't want to loop indefinitely.... It
  // is likely that we will just loop less than a dozen of times.
  while (clasob != NULL && cnt < 100 && !closres)
    {
      RpsClassInfo_t *clinf = NULL;
      RpsObject_t *superob = NULL;
      pthread_mutex_lock (&clasob->ob_mtx);
      clinf = rps_get_object_payload_of_type (clasob, -RpsPyt_ClassInfo);
      if (clinf && clinf->pclass_magic == RPS_CLASSINFO_MAGIC)
	{
	  closres = rps_classinfo_get_method (clinf, (RpsObject_t *) selob);
	}
      if (!closres)
	{
	  superob = clinf->pclass_super;
	}
      pthread_mutex_unlock (&clasob->ob_mtx);
      clasob = superob;
      cnt++;
    }
  return closres;
}				/* end rps_value_compute_method_closure */

/************** end of file main_rps.c ****************/
