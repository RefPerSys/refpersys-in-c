/****************************************************************
 * file Refpersys.h
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *      It is almost its only public C99 header file.
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



#ifndef REFPERSYS_INCLUDED
#define REFPERSYS_INCLUDED

// A name containing `unsafe` refers to something which should be used
// with great caution.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif /*_GNU_SOURCE*/

#include <argp.h>
#include <ctype.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/syslog.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <dlfcn.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/personality.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>

// man7.org/linux/man-pages/man3/gnu_get_libc_version.3.html
#include <gnu/libc-version.h>


// GNU libunistring www.gnu.org/software/libunistring/
// we use UTF-8 strings
#include <unicode/unistr.h>


/// Ian Taylor's libbacktrace github.com/ianlancetaylor/libbacktrace
#include "backtrace.h"

/// GTK3 graphical toolkit gtk.org/gtk3/
#include "gtk/gtk.h"
 
/// CURL library for HTTP web client curl.se/libcurl/
#include "curl/curl.h"

/// libjansson JSON library https://digip.org/jansson/
#include "jansson.h"

/// global variables
extern bool rps_running_in_batch; /* no user interface */
extern bool rps_showing_version;
extern bool rps_with_gui;
extern struct backtrace_state* rps_backtrace_common_state;
extern const char* rps_progname; /* argv[0] of main */
extern void* rps_dlhandle;	/* global dlopen handle */
extern const char*rps_load_directory;

/// global variables declared in generated __timestamp.c file:
extern const char rps_timestamp[]; /* some date string */
extern const unsigned long rps_timelong; /* some (time_t) */
extern const char rps_topdirectory[]; /* the source topdirectory */
extern const char rps_gitid[];
extern const char rps_lastgittag[];
extern const char rps_lastgitcommit[];
extern const char rps_gitremoteoriginurl[];
extern const char rps_md5sum[];
extern const char*const rps_files[];
extern const char rps_makefile[];
extern const char* rps_subdirectories[];
extern const char rps_c_compiler_version[];
extern const char rps_shortgitid[];

extern const char*rps_hostname(void);


//// prime numbers and a table of primes:
extern const unsigned rps_nb_primes_in_tab;
extern int64_t rps_prime_of_index (int ix);
extern int rps_index_of_prime (int64_t n);
extern int64_t rps_prime_above (int64_t n);
extern int64_t rps_prime_below (int64_t n);

/// both backtrace_full and backtrace_simple callbacks are continuing with a 0 return code:
enum { RPS_CONTINUE_BACKTRACE=0, RPS_STOP_BACKTRACE=1 };


/// value types - prefix is RpsTy
enum {
  RpsTy__NONE,
  RpsTy_Int,			/* tagged int, 63 bits, without memory zone */
  // the following are in garbage collected memory, our zoned values
  RpsTy_Double,
  RpsTy_String,
  RpsTy_Json,
  RpsTy_GtkWiget,		/* some GtkWidget* pointer; of course GTK widgets are not persisted */
  RpsTy_TupleOb,
  RpsTy_SetOb,
  RpsTy_Object,
  RpsTy_File,		/* some opened FILE* handle; of course they are not persisted */
  RpsTy__LAST
};

/// an hash has 32 bits and conventionally is never 0
typedef uint32_t RpsHash_t;

/// a value is a word, sometimes a pointer, sometimes a tagged integer (odd word)
typedef uintptr_t RpsValue_t;

#define RPS_NULL_VALUE ((RpsValue_t)0)

/// the loader internals are in file load_rps.c
typedef struct RpsZoneObject_st  RpsObject_t; ///// forward declaration
typedef struct RpsPayl_Loader_st RpsLoader_t; ///// forward declaration
typedef struct RpsPayl_AttrTable_st RpsAttrTable_t; //// forward declaration
extern bool rps_is_valid_loader(RpsLoader_t*ld);
extern bool rps_is_valid_filling_loader (RpsLoader_t *);

extern RpsHash_t rps_hash_cstr (const char *s);

/*****************************************************************/
/// object ids, also known as oid-s
struct RpsOid_st {
  uint64_t id_hi, id_lo;
};
typedef struct RpsOid_st RpsOid_t;

#define RPS_B62DIGITS \
  "0123456789"				  \
  "abcdefghijklmnopqrstuvwxyz"		  \
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

#define RPS_OIDBUFLEN 24
#define RPS_OIDBASE (sizeof(RPS_B62DIGITS)-1)
#define RPS_MIN_OID_HI (62*62*62)
#define RPS_MAX_OID_HI /* 8392993658683402240, about 8.392994e+18 */ \
  ((uint64_t)10 * 62 * (62 * 62 * 62) * (62 * 62 * 62) * (62 * 62 * 62))
#define RPS_NBDIGITS_OID_HI 11
#define RPS_DELTA_OID_HI (RPS_MAX_OID_HI - RPS_MIN_OID_HI)
#define RPS_MIN_OID_LO (62*62*62)
#define RPS_MAX_OID_LO /* about 3.52161e+12 */ \
  ((uint64_t)62 * (62L * 62 * 62) * (62 * 62 * 62))
#define RPS_DELTA_OID_LO (RPS_MAX_OID_LO - RPS_MIN_OID_LO)
#define RPS_NBDIGITS_OID_LO 7
#define RPS_OID_NBCHARS (RPS_NBDIGITS_OID_HI+RPS_NBDIGITS_OID_LO+1)
#define RPS_OID_MAXBUCKETS (10*62)

extern bool rps_oid_is_null(const RpsOid_t oid);
extern bool rps_oid_is_valid(const RpsOid_t oid);
extern bool rps_oid_equal(const RpsOid_t oid1, const RpsOid_t oid2);
extern bool rps_oid_less_than(const RpsOid_t oid1, const RpsOid_t oid2);
extern bool rps_oid_less_equal(const RpsOid_t oid1, const RpsOid_t oid2);

/*****************************************************************/
/// a payload is not a proper value, but garbaged collected as if it was one....
/// payload types - prefix is RpsPyt
enum {
  RpsPyt__NONE,
  RpsPyt_Loader,		/* the initial loader */
  RpsPyt_AttrTable,		/* associate objects to values */
  RpsPyt_StringBuf,		/* mutable string buffer */
  RpsPyt__LAST
};

/// the fields in every zoned memory -value or payload-; we use macros to mimic C field inheritance
#define RPSFIELDS_ZONED_MEMORY						\
  int8_t zm_type; /* the type of that zone - value (>0) or payload (<0) */ \
  atomic_uchar zm_gcmark; /* the garbage collector mark */ 		\
  uint16_t zm_xtra;	  /* some short extra data */			\
  uint32_t zm_length	  /* the size of variable-sized data */


struct RpsZonedMemory_st { RPSFIELDS_ZONED_MEMORY; };

/// zoned values all have some non-zero hash
#define RPSFIELDS_ZONED_VALUE \
  RPSFIELDS_ZONED_MEMORY; \
  RpsHash_t zv_hash

struct RpsZonedValue_st { RPSFIELDS_ZONED_VALUE; };


////////////// boxed double
#define RPSFIELDS_DOUBLE \
  RPSFIELDS_ZONED_VALUE; \
  double dbl_val

struct RpsZoneDouble_st { RPSFIELDS_DOUBLE; };
typedef struct RpsZoneDouble_st  RpsDouble_t; /*for RpsTy_Double, zv_size is unused */
RpsHash_t rps_hash_double (double x);
// allocate a boxed double which is not NAN, fatal if NAN
const RpsDouble_t*rps_alloc_boxed_double(double x);
// load a boxed double
const RpsDouble_t*rps_load_boxed_double(json_t*js, RpsLoader_t*ld);

////////////// string values; their zm_xtra is an index of a prime allocated size
#define RPSFIELDS_STRING \
  RPSFIELDS_ZONED_VALUE; \
  char cstr[];			/* flexible array zone, zm_size is length in UTF8 characters, not in bytes */

struct RpsZoneString_st { RPSFIELDS_STRING; };
typedef struct RpsZoneString_st RpsString_t; /* for RpsTy_String */
// allocate a string
const RpsString_t*rps_alloc_string(const char*str);
// sprintf a string value
const RpsString_t*rps_sprintf_string(const char*fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
// load a boxed double
const RpsString_t*rps_load_string(json_t*js, RpsLoader_t*ld);

/////////////// boxed JSON values
#define RPSFIELDS_JSON \
  RPSFIELDS_ZONED_VALUE; \
  json_t *json

struct RpsZoneJson_st { RPSFIELDS_JSON; };
typedef struct RpsZoneJson_st RpsJson_t; /* for RpsTy_Json */
const RpsJson_t* rps_alloc_json(const json_t*js);
const RpsJson_t* rps_load_json(const json_t*js, RpsLoader_t*ld);

/////////////// boxed GtkWidget*
#define RPSFIELDS_GTKWIDGET \
  RPSFIELDS_ZONED_VALUE; \
  GtkWidget*gtk_widget

struct RpsZoneGtkWidget_st { RPSFIELDS_GTKWIDGET; };
typedef struct RpsZoneGtkWidget_st RpsGtkWidget_t; /* for RpsTy_GtkWiget */
const RpsGtkWidget_t* rps_alloc_gtk_widget(GtkWidget*);
// no load routine for GtkWidget

/////////////// tuple of objects value
#define RPSFIELDS_TUPLEOB \
  RPSFIELDS_ZONED_VALUE; \
  RpsObject_t* tuple_comp[]	/* zv_size is the number of components */

struct RpsZoneTupleOb_st { RPSFIELDS_TUPLEOB; };
typedef struct RpsZoneTupleOb_st RpsTupleOb_t; /* for RpsTy_TupleOb */
const RpsTupleOb_t* rps_alloc_vtuple(unsigned arity, ...);
const RpsTupleOb_t* rps_alloc_tuple_sized(unsigned arity, const RpsObject_t**arr);
/////////////// set of objects value
#define RPSFIELDS_SETOB \
  RPSFIELDS_ZONED_VALUE; \
  RpsObject_t* set_elem[]	/* zv_size is the number of elements, and they are ordered by oid */

struct RpsZoneSetOb_st { RPSFIELDS_SETOB; };
typedef struct RpsZoneSetOb_st RpsSetOb_t; /* for RpsTy_SetOb */


////////////////////////// objects
#define RPSFIELDS_OBJECT                        \
  RPSFIELDS_ZONED_VALUE;                        \
  RpsOid_t ob_id;                               \
  double ob_mtime;                              \
  pthread_mutex_t ob_mtx;                       \
  RpsObject_t* ob_class;                        \
  RpsObject_t* ob_zone;                         \
  RpsAttrTable_t* ob_attrtable;                 \
  /* other fields missing */


struct RpsZoneObject_st { RPSFIELDS_OBJECT; };
extern bool rps_is_valid_object(const RpsObject_t* obj);
extern bool rps_object_less(const RpsObject_t* ob1, const RpsObject_t*ob2);

/////////////// table of attributes (objects) with their values
/////////////// entries are either empty or sorted by ascending attributes
struct rps_attrentry_st {
  RpsObject_t* ent_attr;
  RpsValue_t ent_val;
};

#define RPS_MAX_NB_ATTRS (1U<<28)
//// the zm_xtra is a prime index for the allocated size
//// the zm_length is the actual number of non-empty entries
#define RPSFIELDS_ATTRTABLE			\
  RPSFIELDS_ZONED_MEMORY;			\
  struct rps_attrentry_st attr_entries[]

///// for RpsAttrTable_t
struct RpsPayl_AttrTable_st { RPSFIELDS_ATTRTABLE; };
extern RpsAttrTable_t*rps_alloc_empty_attr_table(unsigned size);
extern RpsValue_t rps_attr_table_find(const RpsAttrTable_t*tbl, RpsObject_t*obattr);
extern RpsAttrTable_t*rps_attr_table_put(RpsAttrTable_t*tbl, RpsObject_t*obattr, RpsValue_t val);
extern RpsAttrTable_t*rps_attr_table_remove(RpsAttrTable_t*tbl, RpsObject_t*obattr, RpsValue_t val);

////////////////////////////////////////////////////////////////
extern void rps_load_initial_heap(void);
extern void rps_abort(void) __attribute__((noreturn));

extern void rps_fatal_stop_at (const char *fil, int lineno) __attribute__((noreturn));

#define RPS_FATAL_AT_BIS(Fil,Lin,Fmt,...) do {			\
    fprintf (stderr,						\
	    "RefPerSys FATAL:%s:%d: <%s>\n " Fmt "\n\n",	\
            Fil, Lin, __func__, ##__VA_ARGS__);			\
    fflush (stderr);						\
    rps_fatal_stop_at(Fil,Lin); } while(0)

#define RPS_FATAL_AT(Fil,Lin,Fmt,...) RPS_FATAL_AT_BIS(Fil,Lin,Fmt,##__VA_ARGS__)
#define RPS_FATAL(Fmt,...) RPS_FATAL_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

extern void*alloc0_at_rps(size_t sz, const char*fil, int lineno);
#define RPS_ALLOC_ZEROED(Sz) alloc0_at_rps((Sz),__FILE__,__LINE__)

extern void*alloczone_at_rps(size_t bytsz, int8_t type, const char*fil, int lineno);
#define RPS_ALLOC_ZONE(Bsz,Ty) alloczone_at_rps((Bsz),(Ty),__FILE__,__LINE__)
#define RPS_MAX_ZONE_SIZE (size_t)(1L<<28)
extern pid_t rps_gettid(void);
extern double rps_clocktime(clockid_t);

//////////////// tagged integer values
extern RpsValue_t rps_tagged_integer_value(intptr_t i);
extern bool rps_is_tagged_integer(const RpsValue_t v);
extern intptr_t rps_value_to_integer(const RpsValue_t v); /* gives 0 for a non-tagged integer */

#endif /*REFPERSYS_INCLUDED*/
//// end of file Refpersys.h
