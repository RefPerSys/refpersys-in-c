/****************************************************************
 * file Refpersys.h
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *      It is almost its only public C11 header file.
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
#include <stdalign.h>
#include <math.h>
#include <limits.h>
#include <setjmp.h>


// See also the important comment about pthreads in file agenda_rps.c

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

/*  Adelson-Velsky and Landis generic balanced trees from
    http://attractivechaos.github.io/klib/ see also
    https://en.wikipedia.org/wiki/AVL_tree */
#include "kavl.h"

/// global variables
extern bool rps_running_in_batch;	/* no user interface */
extern bool rps_showing_version;
extern bool rps_with_gui;

/// number of agenda threads
#define RPS_MIN_NB_THREADS 2
#define RPS_MAX_NB_THREADS 16
extern int rps_nb_threads;

extern struct backtrace_state *rps_backtrace_common_state;
extern const char *rps_progname;	/* argv[0] of main */
extern void *rps_dlhandle;	/* global dlopen handle */
extern const char *rps_load_directory;
extern const char *rps_dump_directory;

/// global variables declared in generated __timestamp.c file:
extern const char _rps_git_id[];
extern const char _rps_git_last_tag[];
extern const char _rps_git_last_commit[];
extern const char _rps_git_short_id[];
extern const char _rps_git_remote_origin_url[];
extern const char rps_timestamp[];	/* some date string */
extern const unsigned long rps_timelong;	/* some (time_t) */
extern const char rps_topdirectory[];	/* the source topdirectory */
extern const char rps_md5sum[];
extern const char *const rps_files[];
extern const char rps_makefile[];
extern const char *rps_subdirectories[];
extern const char rps_c_compiler_version[];

extern const char *rps_hostname (void);


//// prime numbers and a table of primes:
extern const unsigned rps_nb_primes_in_tab;
extern int64_t rps_prime_of_index (int ix);
extern int rps_index_of_prime (int64_t n);
extern int64_t rps_prime_above (int64_t n);
extern int64_t rps_prime_below (int64_t n);





/////////////////////////////////////////////////////////
///// DEBUGGING SUPPORT

extern void rps_set_debug (const char *dbglev);

/// keep the debug options in alphabetical order
#define RPS_DEBUG_OPTIONS(dbgmacro)		\
  dbgmacro(CMD)					\
  dbgmacro(CODEGEN)				\
  dbgmacro(DUMP)				\
  dbgmacro(GARBCOLL)				\
  dbgmacro(GUI)                                 \
  dbgmacro(LOAD)				\
  dbgmacro(LOWREP)				\
  dbgmacro(MISC)				\
  dbgmacro(MSGSEND)				\
  dbgmacro(PARSE)				\
  dbgmacro(REPL)				\
  dbgmacro(WEB)
				/*end RPS_DEBUG_OPTIONS */

#define RPS_DEBUG_OPTION_DEFINE(dbgopt) RPS_DEBUG_##dbgopt,

extern unsigned rps_debug_flags;


enum Rps_Debug
{
  RPS_DEBUG__NONE,
  // expands to RPS_DEBUG_CMD, RPS_DEBUG_DUMP, RPS_DEBUG_GARBAGE_COLLECTOR...
  RPS_DEBUG_OPTIONS (RPS_DEBUG_OPTION_DEFINE) RPS_DEBUG__LAST
};

// so we could code  RPS_DEBUG_PRINTF(NEVER, ....)
#define RPS_DEBUG_NEVER RPS_DEBUG__NONE

#define RPS_DEBUG_ENABLED(dbgopt) (rps_debug_flags & (1 << RPS_DEBUG_##dbgopt))


/// debug print to stderr; if FLINE is negative, print a
/// newline before.... If FMT starts with a |, print the backtrace...
void
rps_debug_printf_at (const char *fname, int fline, enum Rps_Debug dbgopt,
		     const char *fmt, ...)
__attribute__((format (printf, 4, 5)));


#define RPS_DEBUG_PRINTF_AT(fname, fline, dbgopt, fmt, ...)      \
do                                                               \
  {                                                              \
    if (RPS_DEBUG_ENABLED(dbgopt))                               \
      rps_debug_printf_at(fname, fline, RPS_DEBUG_##dbgopt, fmt, \
                          ##__VA_ARGS__);                        \
  }                                                              \
while (0)


#define RPS_DEBUG_PRINTF_AT_BIS(fname, fline, dbgopt, fmt, ...)  \
   RPS_DEBUG_PRINTF_AT(fname, fline, dbgopt, fmt, ##__VA_ARGS__)

#define RPS_DEBUG_PRINTF(dbgopt, fmt, ...) \
  RPS_DEBUG_PRINTF_AT_BIS(__FILE__, __LINE__, dbgopt, fmt, ##__VA_ARGS__)

#define RPS_DEBUG_NLPRINTF(dbgopt, fmt, ...) \
  RPS_DEBUG_PRINTF_AT_BIS(__FILE__, -__LINE__, dbgopt, fmt, ##__VA_ARGS__)

/// both backtrace_full and backtrace_simple callbacks are continuing with a 0 return code:
enum RpsBacktrace
{
  RPS_BACKTRACE_CONTINUE = 0,
  RPS_BACKTRACE_STOP = 1
};


/// value types - prefix is RPS_TYPE
enum RpsType
{
  RPS_TYPE__NONE,
  RPS_TYPE_INT,			/* tagged int, 63 bits, without memory zone */
  // the following are in garbage collected memory, our zoned values
  RPS_TYPE_DOUBLE,
  RPS_TYPE_STRING,
  RPS_TYPE_JSON,
  RPS_TYPE_GTKWIDGET,		/* some GtkWidget* pointer; of course GTK widgets are not persisted */
  RPS_TYPE_TUPLE,
  RPS_TYPE_SET,
  RPS_TYPE_CLOSURE,
  RPS_TYPE_OBJECT,
  RPS_TYPE_FILE,		/* some opened FILE* handle; of course they are not persisted */
  RPS_TYPE__LAST
};


const char *rps_type_str (int ty);

/* maximal value depth before encountering objects:*/
#define RPS_MAX_VALUE_DEPTH 64

/// an hash has 32 bits and conventionally is never 0
typedef uint32_t RpsHash_t;


typedef struct RpsZoneObject_st RpsObject_t;	///// forward declaration
typedef struct RpsPayl_AttrTable_st RpsAttrTable_t;	//// forward declaration

/// callback function, by convention returning false to "stop" or "fail" e.g. some iteration
typedef bool rps_object_callback_sig_t (RpsObject_t * ob, void *data);

/// a value is a word, sometimes a pointer, sometimes a tagged integer (odd word)
typedef uintptr_t RpsValue_t;

#define RPS_NULL_VALUE ((RpsValue_t)0)
extern enum RpsType rps_value_type (RpsValue_t val);
/// the loader internals are in file load_rps.c
typedef struct RpsPayl_Loader_st RpsLoader_t;	///// forward declaration
extern bool rps_is_valid_loader (RpsLoader_t * ld);
extern bool rps_is_valid_filling_loader (RpsLoader_t * ld);
extern bool rps_is_valid_creating_loader (RpsLoader_t * ld);
extern unsigned rps_loader_nb_globals (RpsLoader_t * ld);
extern unsigned rps_loader_nb_constants (RpsLoader_t * ld);
extern RpsValue_t rps_loader_json_to_value (RpsLoader_t * ld, json_t * jv);

extern RpsObject_t *rps_loader_json_to_object (RpsLoader_t * ld, json_t * jv);

//// Signature of extern "C" functions dlsymed for payload loading;
//// their name starts with rpsldpy_ and the object has been locked..
typedef void rpsldpysig_t (RpsObject_t * obz, RpsLoader_t * ld,
			   const json_t * jv, int spaceindex);
#define RPS_PAYLOADING_PREFIX "rpsldpy_"

/// the dumper internals are in file dump_rps.c
typedef struct RpsPayl_Dumper_st RpsDumper_t;	///// forward declaration
extern bool rps_is_valid_dumper (RpsDumper_t * du);
extern void rps_dumper_scan_value (RpsDumper_t * du, RpsValue_t val,
				   unsigned depth);
extern void rps_dumper_scan_object (RpsDumper_t * du, RpsObject_t * ob);
extern void rps_dumper_scan_internal_object (RpsDumper_t * du,
					     RpsObject_t * ob);
extern json_t *rps_dump_json_for_value (RpsDumper_t * du, RpsValue_t val,
					unsigned depth);
extern json_t *rps_dump_json_for_object (RpsDumper_t * du,
					 const RpsObject_t * ob);
///// hash of strings
extern RpsHash_t rps_hash_cstr (const char *s);

#include "include/oid_rps.h"

struct rps_owned_payload_st;
/// A payload is not a proper value, but garbaged collected as if it was one....
/// payload types - prefix is RpsPyt.  In memory we used negative payload indexes...
enum
{
  //// this enumeration needs to be in generated C code of course....
  RpsPyt__NONE,
  RpsPyt_CallFrame,		/* #1 call frames */
  RpsPyt_Loader,		/* #2 the initial loader */
  RpsPyt_AttrTable,		/* #3 associate objects to values */
  RpsPyt_StringBuf,		/* #4 mutable string buffer */
  RpsPyt_Symbol,		/* #5 symbol */
  RpsPyt_ClassInfo,		/* #6 class information */
  RpsPyt_MutableSetOb,		/* #7 mutable set of objects */
  RpsPyt_DequeOb,		/* #8 double ended queue of objects */
  RpsPyt_Tasklet,		/* #9 tasklet in agenda */
  RpsPyt_Agenda,		/* #10 the agenda */
  RpsPyt_StringDict,		/* #11 string dictionary associating names
				   to values */
  RpsPyt_HashTblObj,		/* #12 hashtable of objects */
  RpsPyt_Space,			/* #13 space payload */
  RpsPyt_Dumper,		/* #14 dumper payload */
  RpsPyt__LAST
};

// the maximal index is for internal arrays, and allow for more than
// ten more payload types to be added during a run.
#define RPS_MAX_PAYLOAD_TYPE_INDEX  (((RpsPyt__LAST+15)|0xf)+1)


/****************************************************************
 * Garbage collected memory zones.  Most boxed values or payloads are
 * starting with these...
 ****************************************************************/
/// the fields in every zoned memory -value or payload-; we use macros to mimic C field inheritance
#define RPSFIELDS_ZONED_MEMORY						\
  atomic_schar zm_atype; /* the type of that zone - value (>0) or payload (<0) */ \
  atomic_uchar zm_gcmark; /* the garbage collector mark */ 		\
  uint16_t zm_xtra;	  /* some short extra data */			\
  uint32_t zm_length;		/* the size of variable-sized data */   \
  volatile void* zm_gclink	/* a pointer for our naive GC */


struct RpsZonedMemory_st
{
  RPSFIELDS_ZONED_MEMORY;
};

static inline int8_t
rps_zoned_memory_type (const void *ad)
{
  if (!ad)
    return 0;
  return atomic_load (&((struct RpsZonedMemory_st *) ad)->zm_atype);
}				/* end rps_zoned_memory_type */

#define RPS_ZONED_MEMORY_TYPE(Ad) rps_zoned_memory_type((const void*)(Ad))

static inline unsigned char
rps_zoned_memory_gcmark (const void *ad)
{
  if (!ad)
    return 0;
  return atomic_load (&((struct RpsZonedMemory_st *) ad)->zm_gcmark);
}				/* end rps_zoned_memory_gcmark */


/// zoned values all have some non-zero hash
#define RPSFIELDS_ZONED_VALUE \
  RPSFIELDS_ZONED_MEMORY; \
  RpsHash_t zv_hash

struct RpsZonedValue_st
{
  RPSFIELDS_ZONED_VALUE;
};


/****************************************************************
 * Boxed double values
 ****************************************************************/
#define RPSFIELDS_DOUBLE \
  RPSFIELDS_ZONED_VALUE; \
  double dbl_val

struct RpsZoneDouble_st
{
  RPSFIELDS_DOUBLE;
};
typedef struct RpsZoneDouble_st RpsDouble_t;	/*for RPS_TYPE_DOUBLE, zm_length is unused */
RpsHash_t rps_hash_double (double x);
// allocate a boxed double which is not NAN, fatal if NAN
const RpsDouble_t *rps_alloc_boxed_double (double x);
// load a boxed double
const RpsDouble_t *rps_load_boxed_double (json_t * js, RpsLoader_t * ld);
// return NAN if not double
double rps_double_value (RpsValue_t val);


/****************************************************************
 * Boxed string values; their zm_xtra is an index of a prime allocated
 * size.
 ****************************************************************/
#define RPSFIELDS_STRING \
  RPSFIELDS_ZONED_VALUE; \
  char cstr[];			/* flexible array zone, zm_length is length in UTF8 characters, not in bytes */

struct RpsZoneString_st
{
  RPSFIELDS_STRING;
};
typedef struct RpsZoneString_st RpsString_t;	/* for RPS_TYPE_STRING */
// allocate a string
const RpsString_t *rps_alloc_string (const char *str);
// sprintf a string value
const RpsString_t *rps_sprintf_string (const char *fmt, ...)
  __attribute__((__format__ (__printf__, 1, 2)));
// load a string
const RpsString_t *rps_load_string (json_t * js, RpsLoader_t * ld);
// get the UTF8 bytes of a string value, or else NULL
const char *rps_stringv_utf8bytes (RpsValue_t v);
// get the length, in Unicode glyphs, of a string value
unsigned rps_stringv_utf8length (RpsValue_t v);
// get the hashcode of a string value
RpsHash_t rps_stringv_hash (RpsValue_t v);

/****************************************************************
 * Boxed JSON values.
 ****************************************************************/
#define RPSFIELDS_JSON \
  RPSFIELDS_ZONED_VALUE; \
  const json_t *json

struct RpsZoneJson_st
{
  RPSFIELDS_JSON;
};
typedef struct RpsZoneJson_st RpsJson_t;	/* for RPS_TYPE_JSON */
const RpsJson_t *rps_alloc_json (const json_t * js);
const RpsJson_t *rps_load_json (const json_t * js, RpsLoader_t * ld);
const json_t *rps_json_value (RpsValue_t val);


/****************************************************************
 * Boxed GtkWidget-s values, they cannot be persisted in the heap.
 ****************************************************************/
/////////////// boxed GtkWidget*
#define RPSFIELDS_GTKWIDGET \
  RPSFIELDS_ZONED_VALUE; \
  GtkWidget*gtk_widget

struct RpsZoneGtkWidget_st
{
  RPSFIELDS_GTKWIDGET;
};
typedef struct RpsZoneGtkWidget_st RpsGtkWidget_t;	/* for RPS_GTKWIDGET */
RpsValue_t rps_alloc_gtk_widget (GtkWidget *);
GtkWidget *rps_gtk_widget_value (RpsValue_t val);

// no load routine for GtkWidget




/****************************************************************
 * Tuple of objects value.
 ****************************************************************/
#define RPSFIELDS_TUPLEOB \
  RPSFIELDS_ZONED_VALUE; \
  RpsObject_t* tuple_comp[]	/* zm_length is the number of components */

struct RpsZoneTupleOb_st
{
  RPSFIELDS_TUPLEOB;
};
typedef struct RpsZoneTupleOb_st RpsTupleOb_t;	/* for RPS_TYPE_TUPLE */
unsigned rps_vtuple_size (const RpsTupleOb_t * tup);
RpsObject_t *rps_vtuple_nth (const RpsTupleOb_t * tup, int rk);
const RpsTupleOb_t *rps_alloc_vtuple (unsigned arity, /*objects */ ...);
const RpsTupleOb_t *rps_alloc_tuple_sized (unsigned arity,
					   RpsObject_t ** arr);
const RpsTupleOb_t *rps_load_tuple (const json_t * js, RpsLoader_t * ld);



/****************************************************************
 * Ordered set of objects value.
 ****************************************************************/
#define RPSFIELDS_SETOB \
  RPSFIELDS_ZONED_VALUE; \
  const RpsObject_t* set_elem[]	/* zm_length is the number of elements, and they are ordered by oid */

struct RpsZoneSetOb_st
{
  RPSFIELDS_SETOB;
};
typedef struct RpsZoneSetOb_st RpsSetOb_t;	/* for RPS_TYPE_SET */
const RpsSetOb_t *rps_alloc_vset (unsigned card, /*objects */ ...);
const RpsSetOb_t *rps_alloc_set_sized (unsigned nbcomp,
				       const RpsObject_t ** arr);
const RpsSetOb_t *rps_load_set (const json_t * js, RpsLoader_t * ld);

// return the index of an element or -1 if non member
int rps_set_index (const RpsSetOb_t * setv, const RpsObject_t * ob);

// test membership
bool rps_set_contains (const RpsSetOb_t * setv, const RpsObject_t * ob);

// cardinal of a set
unsigned rps_set_cardinal (const RpsSetOb_t * setv);

/// get the N-th member of a set; if N<0 count from last.
const RpsObject_t *rps_set_nth_member (const RpsSetOb_t * setv, int n);

/****************************************************************
 * Closures.  The connective of a closure is an object whose
 * ob_routsig and ob_routaddr are set.  When the signature is
 * appropriate, the function pointed by ob_routaddr is called when
 * applying the closure.
 ****************************************************************/
#define RPSFIELDS_CLOSURE \
  RPSFIELDS_ZONED_VALUE; \
  RpsObject_t* clos_conn; \
  RpsValue_t clos_meta; \
  RpsValue_t clos_val[]		/*of zm_length */

#define RPS_CLOSURE_MAX_NB_VALUE 1024
struct RpsClosure_st
{
  RPSFIELDS_CLOSURE;
};
typedef struct RpsClosure_st RpsClosure_t;

const RpsClosure_t *rps_closure_make (RpsObject_t * conn, unsigned arity,
				      ...);
const RpsClosure_t *rps_closure_meta_make (RpsObject_t * conn,
					   RpsValue_t meta, unsigned arity,
					   ...);
const RpsClosure_t *rps_closure_array_make (RpsObject_t * conn,
					    RpsValue_t meta, unsigned arity,
					    RpsValue_t * cvalarr);
const RpsObject_t *rps_closure_connective (RpsValue_t val);
RpsValue_t rps_closure_get_closed_value (RpsValue_t val, int ix);
RpsValue_t rps_closure_meta (RpsValue_t);
unsigned rps_closure_size (RpsValue_t);

/* TODO: closure application code should be declared. The ob_routaddr
   of clos_conn is holding the C function pointer to be called. The
   ob_routsig should be checked. Closure application code should
   carefully be tail-call friendly. */
typedef struct rps_value_and_int_st
{
  RpsValue_t val;
  intptr_t num;
} RpsValueAndInt;

typedef struct rps_two_values_st
{
  RpsValue_t main_val;
  RpsValue_t xtra_val;
} RpsTwoValues;

typedef struct rps_protocallframe_st rps_callframe_t;

/*****************************************************************
 *                       APPLYING FUNCTIONS
 *
 * Notice that on x86/64 calling conventions only six arguments are
 * passed by registers.
 *
 ****************************************************************/

typedef RpsValue_t rps_apply_v_sigt (rps_callframe_t * callerframe,
				     const RpsClosure_t * clos,
				     RpsValue_t arg0, RpsValue_t arg1,
				     RpsValue_t arg2, RpsValue_t arg3);
RpsValue_t rps_closure_apply_v (rps_callframe_t * callerframe,
				const RpsClosure_t * clos, RpsValue_t arg0,
				RpsValue_t arg1, RpsValue_t arg2,
				RpsValue_t arg3);

typedef RpsValueAndInt rps_apply_vi_sigt (rps_callframe_t * callerframe,
					  const RpsClosure_t * clos,
					  RpsValue_t arg0, RpsValue_t arg1,
					  RpsValue_t arg2, RpsValue_t arg3);
RpsValueAndInt rps_closure_apply_vi (rps_callframe_t * callerframe,
				     const RpsClosure_t * clos,
				     RpsValue_t arg0, RpsValue_t arg1,
				     RpsValue_t arg2, RpsValue_t arg3);

typedef RpsTwoValues rps_apply_twov_sigt (rps_callframe_t * callerframe,
					  const RpsClosure_t * clos,
					  RpsValue_t arg0, RpsValue_t arg1,
					  RpsValue_t arg2, RpsValue_t arg3);
RpsTwoValues rps_closure_apply_twov (rps_callframe_t * callerframe,
				     const RpsClosure_t * clos,
				     RpsValue_t arg0, RpsValue_t arg1,
				     RpsValue_t arg2, RpsValue_t arg3);

typedef intptr_t rps_apply_i_sigt (rps_callframe_t * callerframe,
				   RpsClosure_t * clos, RpsValue_t arg0,
				   RpsValue_t arg1, RpsValue_t arg2,
				   RpsValue_t arg3);
intptr_t rps_closure_apply_i (rps_callframe_t * callerframe,
			      RpsClosure_t * clos, RpsValue_t arg0,
			      RpsValue_t arg1, RpsValue_t arg2,
			      RpsValue_t arg3);

/****************************************************************
 * Boxed FILE handle.
 ****************************************************************/
#define RPSFIELDS_FILE \
  RPSFIELDS_ZONED_VALUE; \
  /* we probably need more fields here... */ \
  FILE*fileh;

struct RpsZoneFile_st
{
  RPSFIELDS_FILE;
};
typedef struct RpsZoneFile_st RpsFile_t;	/* for RPS_TYPE_FILE */
// allocate a file handle
const RpsFile_t *rps_alloc_plain_file (FILE * f);
FILE *rps_file_of_value (RpsValue_t val);

/****************************************************************
 * Mutable and mutexed heavy objects.
 * ==================================
 *
 *
 * The ob_class is an object representing the RefPerSys class of our
 * object.  The ob_space is its space. Different spaces are persisted
 * in different files.  The ob_attrtable is for the attributes of that
 * object.  The ob_routsig and ob_routaddr is for the routine inside
 * that object, e.g. when that object is a closure connective.  The
 * ob_nbcomp is the used length of the component array ob_comparr,
 * whose allocated size is ob_compsize.  The ob_payload is the
 * optional object payload.
 ****************************************************************/
#define RPSFIELDS_OBJECT                                        \
  RPSFIELDS_ZONED_VALUE;                                        \
  RpsOid ob_id;                                                 \
  double ob_mtime;                                              \
  pthread_mutex_t ob_mtx;                                       \
  RpsObject_t* ob_class;                                        \
  RpsObject_t* ob_space;                                        \
  RpsAttrTable_t* ob_attrtable /*unowned!*/;                    \
  RpsObject_t* ob_routsig      /* signature of routine */;      \
  void* ob_routaddr;           /* dlsymed address of routine */ \
  unsigned ob_nbcomp;                                           \
  unsigned ob_compsize;                                         \
  RpsValue_t*ob_comparr;                                        \
  void* ob_payload


#define RPS_MAX_NB_OBJECT_COMPONENTS (1U<<20)

struct RpsZoneObject_st
{
  RPSFIELDS_OBJECT;
};

struct internal_rootob_node_rps_st
{
  RpsObject_t *rootobrps_obj;
    KAVL_HEAD (struct internal__rootob_node_rps_st) rootobrps_head;
};
extern void rps_initialize_objects_machinery (void);
extern void rps_initialize_objects_for_loading (RpsLoader_t * ld,
						unsigned nbglobroot);
extern bool rps_is_valid_object (RpsObject_t * obj);
extern bool rps_object_less (RpsObject_t * ob1, RpsObject_t * ob2);
extern int rps_object_cmp (const RpsObject_t * ob1, const RpsObject_t * ob2);
extern void rps_object_array_qsort (const RpsObject_t ** arr, int size);
extern RpsObject_t *rps_find_object_by_oid (const RpsOid oid);
extern RpsObject_t *rps_get_loaded_object_by_oid (RpsLoader_t * ld,
						  const RpsOid oid);
extern RpsValue_t rps_get_object_attribute (RpsObject_t * ob,
					    RpsObject_t * obattr);
// In a given object, get its payload if it has type paylty; accepts
// any payload if paylty is 0.  For example:
// rps_get_object_payload_of_type(obclass, RpsPyt_ClassInfo);
extern void *rps_get_object_payload_of_type (RpsObject_t * ob, int paylty);
extern void rps_check_all_objects_buckets_are_valid (void);
extern void rps_object_reserve_components (RpsObject_t * obj,
					   unsigned nbcomp);
extern void rps_add_global_root_object (RpsObject_t * obj);
extern void rps_remove_global_root_object (RpsObject_t * obj);
extern unsigned rps_nb_global_root_objects (void);
extern const RpsSetOb_t *rps_set_of_global_root_objects (void);




/****************************************************************
 * Common superfields of owned payloads
 ****************************************************************/
#define RPSFIELDS_OWNED_PAYLOAD			\
  RPSFIELDS_ZONED_MEMORY;			\
  RpsObject_t* payl_owner

/// By convention, the zm_atype of payload is a small negative index, e.g. some RpsPyt_* 
struct rps_owned_payload_st
{
  RPSFIELDS_OWNED_PAYLOAD;
};

void rps_object_put_payload (RpsObject_t * ob, void *payl);
typedef void rps_payload_remover_t (RpsObject_t *,
				    struct rps_owned_payload_st *,
				    void *data);
extern void rps_register_payload_removal (int paylty,
					  rps_payload_remover_t * rout,
					  void *data);
typedef void rps_payload_dump_scanner_t (RpsDumper_t * du,
					 struct rps_owned_payload_st *payl,
					 void *data);
extern void rps_register_payload_dump_scanner (int paylty,
					       rps_payload_dump_scanner_t *
					       rout, void *data);
typedef void rps_payload_dump_serializer_t (RpsDumper_t * du,
					    struct rps_owned_payload_st *payl,
					    json_t * json, void *data);
extern void rps_register_payload_dump_serializer (int paylty, rps_payload_dump_serializer_t * rout,	///
						  void *data);

extern void rps_dump_scan_object_payload (RpsDumper_t * du, RpsObject_t * ob);
extern void rps_dump_serialize_object_payload (RpsDumper_t * du,
					       RpsObject_t * ob,
					       json_t * jsob);


/****************************************************************
 * Payload for table of ordered attributes (objects) associated to
 * values.
 ****************************************************************/
/////////////// table of attributes (objects) with their values
/////////////// entries are either empty or sorted by ascending attributes
struct rps_attrentry_st
{
  RpsObject_t *ent_attr;
  RpsValue_t ent_val;
};

#define RPS_MAX_NB_ATTRS (1U<<28)
//// the zm_xtra is a prime index for the allocated size
//// the zm_length is the actual number of non-empty entries
#define RPSFIELDS_ATTRTABLE			\
  RPSFIELDS_OWNED_PAYLOAD;			\
  struct rps_attrentry_st attr_entries[]

///// for RpsAttrTable_t
struct RpsPayl_AttrTable_st
{
  RPSFIELDS_ATTRTABLE;
};
extern RpsAttrTable_t *rps_alloc_empty_attr_table (unsigned size);
extern RpsValue_t rps_attr_table_find (const RpsAttrTable_t * tbl,
				       RpsObject_t * obattr);
/// These functions could sometimes re-allocate a new table and free the old one ...
extern RpsAttrTable_t *rps_attr_table_put (RpsAttrTable_t * tbl,
					   RpsObject_t * obattr,
					   RpsValue_t val);
extern RpsAttrTable_t *rps_attr_table_remove (RpsAttrTable_t * tbl,
					      RpsObject_t * obattr);


/****************************************************************
 * Owned symbol payload
 ****************************************************************/
typedef struct RpsPayl_Symbol_st RpsSymbol_t;
struct internal_symbol_node_rps_st
{
  RpsSymbol_t *synodrps_symbol;
    KAVL_HEAD (struct internal_symbol_node_rps_st) synodrps_head;
};

#define RPSFIELDS_PAYLOAD_SYMBOL		\
  RPSFIELDS_OWNED_PAYLOAD;			\
  const RpsString_t* symb_name;			\
  RpsValue_t symb_value


///// for RpsPyt_Symbol
struct RpsPayl_Symbol_st
{
  RPSFIELDS_PAYLOAD_SYMBOL;
};
typedef struct RpsPayl_Symbol_st RpsSymbol_t;
extern RpsSymbol_t *rps_find_symbol (const char *name);
extern RpsSymbol_t *rps_register_symbol (const char *name);



/****************************************************************
 * Class information payload for RpsPyt_ClassInfo
 ****************************************************************/
#define RPSFIELDS_PAYLOAD_CLASSINFO				\
  RPSFIELDS_OWNED_PAYLOAD;					\
  uint64_t pclass_magic /*always RPS_CLASSINFO_MAGIC*/;         \
  RpsObject_t* pclass_super /*:superclass*/;			\
  RpsAttrTable_t *pclass_methdict/*:method dictionary*/;	\
  RpsObject_t* pclass_symbol	/*:optional symbol */

#define RPS_CLASSINFO_MAGIC 0x3d3c6b284031d237UL

struct RpsPayl_ClassInfo_st
{
  RPSFIELDS_PAYLOAD_CLASSINFO;
};
typedef struct RpsPayl_ClassInfo_st RpsClassInfo_t;

extern bool rps_is_valid_classinfo (const RpsClassInfo_t * clinf);
extern RpsObject_t *rps_classinfo_super (const RpsClassInfo_t * clinf);
extern RpsObject_t *rps_classinfo_symbol (const RpsClassInfo_t * clinf);
extern RpsAttrTable_t *rps_classinfo_methdict (const RpsClassInfo_t * clinf);
extern RpsClosure_t *rps_classinfo_get_method (const RpsClassInfo_t * clinf,
					       RpsObject_t * selob);

extern RpsObject_t *rps_obclass_super (RpsObject_t * obcla);
extern RpsObject_t *rps_obclass_symbol (RpsObject_t * obcla);
extern RpsAttrTable_t *rps_obclass_methdict (RpsObject_t * obcla);
extern RpsClosure_t *rps_obclass_get_method (RpsObject_t * obcla,
					     RpsObject_t * selob);

//// given some non-nil value, return the closure to send a method of given selector
extern RpsClosure_t *rps_value_compute_method_closure (RpsValue_t val,
						       const RpsObject_t *
						       selob);

extern rps_payload_remover_t rps_classinfo_payload_remover;
extern rps_payload_dump_scanner_t rps_classinfo_payload_dump_scanner;
extern rps_payload_dump_serializer_t rps_classinfo_payload_dump_serializer;


/****************************************************************
 * Mutable ordered set of objects payload for -RpsPyt_MutableSetOb
 ****************************************************************/
/* Internally we use "kavl.h" */
struct internal_mutable_set_ob_node_rps_st
{
  const RpsObject_t *setobnodrps_obelem;
    KAVL_HEAD (struct internal_mutable_set_ob_node_rps_st) setobnodrps_head;
};

#define RPSFIELDS_PAYLOAD_MUTABLESETOB		\
  RPSFIELDS_OWNED_PAYLOAD;			\
  unsigned muset_card; \
  struct internal_mutable_set_ob_node_rps_st* muset_root


///// for RpsPyt_MutableSetOb
struct RpsPayl_MutableSetOb_st
{
  RPSFIELDS_PAYLOAD_MUTABLESETOB;
};
typedef struct RpsPayl_MutableSetOb_st RpsMutableSetOb_t;

/// initialize the payload to an empty mutable set
extern void rps_object_mutable_set_initialize (RpsObject_t *);
/// add an object, all objects of a tuple, a set.... into a mutable set
extern void rps_object_mutable_set_add (RpsObject_t * obset, RpsValue_t val);
/// remove an object, all objects of a tuple, a set.... into a mutable set
extern void rps_object_mutable_set_remove (RpsObject_t * obset,
					   RpsValue_t val);
/// build the set inside a mutable set
extern const RpsSetOb_t *rps_object_mutable_set_reify (RpsObject_t * obset);

extern bool rps_paylsetob_add_element (RpsMutableSetOb_t * paylmset,
				       const RpsObject_t * ob);
extern bool rps_paylsetob_remove_element (RpsMutableSetOb_t * paylmset,
					  const RpsObject_t * ob);

/****************************************************************
 * Double-ended queue/linked-list payload for -RpsPyt_DequeOb
 ****************************************************************/
struct rps_dequeob_link_st;
#define RPSFIELDS_PAYLOAD_DEQUE			\
  RPSFIELDS_OWNED_PAYLOAD;			\
  struct rps_dequeob_link_st *deqob_first;	\
  struct rps_dequeob_link_st *deqob_last

#define RPS_DEQUE_CHUNKSIZE 6
struct rps_dequeob_link_st
{
  RpsObject_t *dequeob_chunk[RPS_DEQUE_CHUNKSIZE];
  struct rps_dequeob_link_st *dequeob_prev;
  struct rps_dequeob_link_st *dequeob_next;
};
struct RpsPayl_DequeOb_st
{
  RPSFIELDS_PAYLOAD_DEQUE;
};
typedef struct RpsPayl_DequeOb_st RpsDequeOb_t;
RpsDequeOb_t *rps_deque_for_dumper (RpsDumper_t * du);
/// initialize the payload to an empty double ended queue
extern void rps_object_deque_ob_initialize (RpsObject_t *);
/// The get functions are accessors; the pop functions are shinking
/// the queue.  The push functions are growing it and return false on
/// failure.
extern RpsObject_t *rps_object_deque_get_first (RpsObject_t * obq);
extern RpsObject_t *rps_object_deque_pop_first (RpsObject_t * obq);
extern bool rps_object_deque_push_first (RpsObject_t * obq,
					 RpsObject_t * obelem);
extern RpsObject_t *rps_object_deque_get_last (RpsObject_t * obq);
extern RpsObject_t *rps_object_deque_pop_last (RpsObject_t * obq);
extern bool rps_object_deque_push_last (RpsObject_t * obq,
					RpsObject_t * obelem);
extern int rps_object_deque_length (RpsObject_t * obq);
extern RpsObject_t *rps_payldeque_get_first (RpsDequeOb_t * deq);
extern RpsObject_t *rps_payldeque_pop_first (RpsDequeOb_t * deq);
extern bool rps_payldeque_push_first (RpsDequeOb_t * deq,
				      RpsObject_t * obelem);
extern RpsObject_t *rps_payldeque_get_last (RpsDequeOb_t * deq);
extern RpsObject_t *rps_payldeque_pop_last (RpsDequeOb_t * deq);
extern bool rps_payldeque_push_last (RpsDequeOb_t * deq,
				     RpsObject_t * obelem);
extern int rps_payldeque_length (RpsDequeOb_t * deq);


/****************************************************************
 * Hashtable of objects payload for -RpsPyt_HashTblObj
 ****************************************************************/

#define RPS_HTB_EMPTY_SLOT ((RpsObject_t*)(-1L))
#define RPS_HTBOB_MAGIC 0x3210d03f	/*839962687 */
  /* zm_atype should be -RpsPyt_HashTblObj */
  /* zm_length is the total number of objects */
  /* zm_extra is the prime index of buckets */
#define RPSFIELDS_PAYLOAD_HASHTBLOB			\
  RPSFIELDS_OWNED_PAYLOAD;				\
  unsigned htbob_magic /*should be RPS_HTBOB_MAGIC */;  \
struct rps_dequeob_link_st **htbob_bucketarr

struct RpsPayl_HashTblOb_st
{
  RPSFIELDS_PAYLOAD_HASHTBLOB;
};
typedef struct RpsPayl_HashTblOb_st RpsHashTblOb_t;
extern bool rps_hash_tbl_is_valid (const RpsHashTblOb_t * htb);
// create some unowned hash table of objects of a given initial capacity
extern RpsHashTblOb_t *rps_hash_tbl_ob_create (unsigned capacity);
// reorganize and somehow optimize an hash table to its current content
extern void rps_hash_tbl_reorganize (RpsHashTblOb_t * htb);
// reserve space for NBEXTRA more objects, return true on success
// when NBEXTRA is 0, reorganize the hash table to its current size
extern bool rps_hash_tbl_ob_reserve_more (RpsHashTblOb_t * htb,
					  unsigned nbextra);
// add a new element, return true if it was absent
extern bool rps_hash_tbl_ob_add (RpsHashTblOb_t * htb, RpsObject_t * obelem);
// remove an element, return true if it was there
extern bool rps_hash_tbl_ob_remove (RpsHashTblOb_t * htb,
				    RpsObject_t * obelem);
// cardinal of an hash table of objects
extern unsigned rps_hash_tbl_ob_cardinal (RpsHashTblOb_t * htb);
// iterate on objects of an hashtable, return number of visited object
// till rout returns false. The routine should not update the
// hashtable.
extern unsigned rps_hash_tbl_iterate (RpsHashTblOb_t * htb,
				      rps_object_callback_sig_t * rout,
				      void *data);

// make a set from the elements of an hash table
extern const RpsSetOb_t *rps_hash_tbl_set_elements (RpsHashTblOb_t * htb);

/****************************************************************
 * String dictionary payload for -RpsPyt_StringDict
 ****************************************************************/
/* Internally we use "kavl.h" */
struct internal_string_dict_node_rps_st
{
  const RpsString_t *strdicnodrps_name;
  RpsValue_t strdicnodrps_val;
    KAVL_HEAD (struct internal_string_dict_node_rps_st) strdicnodrps_head;
};
#define RPSFIELDS_PAYLOAD_STRINGDICTOB			\
  RPSFIELDS_OWNED_PAYLOAD;				\
  unsigned strdict_size;				\
  struct internal_string_dict_node_rps_st*strdict_root


///// for RpsPyt_StringDict
struct RpsPayl_StringDictOb_st
{
  RPSFIELDS_PAYLOAD_STRINGDICTOB;
};
typedef struct RpsPayl_StringDictOb_st RpsStringDictOb_t;

/// initialize the payload to an empty string dictionary
extern void rps_object_string_dictionary_initialize (RpsObject_t *);
extern RpsValue_t rps_object_string_dictionary_cstr_find (RpsObject_t *
							  obstrdict,
							  const char *cstr);
extern RpsValue_t rps_object_string_dictionary_val_find (RpsObject_t *
							 obstrdict,
							 const RpsString_t *
							 strv);
void rps_object_string_dictionary_put (RpsObject_t * obstrdict,
				       const RpsString_t * strv,
				       const RpsValue_t val);



/****************************************************************
 * Callframe payloads are in call stacks only for -RpsPyt_CallFrame
 ****************************************************************/

// call frame descriptors are in constant read-only memory
#define RPS_CALLFRD_MAGIC 20919	/*0x51b7 */

struct rps_callframedescr_st
{
  const uint16_t calfrd_magic;	/* always RPS_CALLFRD_MAGIC */
  const uint16_t calfrd_nbvalue;
  const uint16_t calfrd_nbobject;
  const uint16_t calfrd_xtrasiz;
  const char calfrd_str[];
};


#define RPSFIELDS_PAYLOAD_PROTOCALLFRAME	       	\
  RPSFIELDS_OWNED_PAYLOAD;				\
  const struct rps_callframedescr_st* calfr_descr;      \
  intptr_t calfr_base[0] __attribute__((aligned(2*sizeof(void*))))

struct rps_protocallframe_st
{
  RPSFIELDS_PAYLOAD_PROTOCALLFRAME;
};


/****************************************************************
 * Space payload for -RpsPyt_Space
 ****************************************************************/
#define RPSFIELDS_PAYLOAD_SPACE			\
  RPSFIELDS_OWNED_PAYLOAD;			\
  RpsValue_t space_data


///// for RpsPyt_Space
struct RpsPayl_Space_st
{
  RPSFIELDS_PAYLOAD_SPACE;
};
typedef struct RpsPayl_Space_st RpsSpace_t;


/****************************************************************
 * Tasklet payload for -RpsPyt_Tasklet
 ****************************************************************/
#define RPSFIELDS_PAYLOAD_TASKLET		\
  RPSFIELDS_OWNED_PAYLOAD;			\
  double tasklet_obsoltime;			\
  RpsClosure_t* tasklet_closure;		\
  bool tasklet_transient

struct RpsPayl_Tasklet_st
{
  RPSFIELDS_PAYLOAD_TASKLET;
};
typedef struct RpsPayl_Tasklet_st RpsTasklet_t;

/****************************************************************
 * Agenda payload for -RpsPyt_Agenda
 ****************************************************************/
enum RpsAgendaPrio_en
{
  AgPrio_Idle = -1,
  AgPrio__None = 0,
  AgPrio_Low,
  AgPrio_Normal,
  AgPrio_High,
  AgPrio__LAST
};
#define RPSFIELDS_PAYLOAD_AGENDA		\
  RPSFIELDS_OWNED_PAYLOAD;			\
  RpsObject_t *agenda_que[AgPrio__LAST]

//// there is an important comment about pthreads in src/agenda_rps.c file.
struct RpsPayl_Agenda_st
{
  RPSFIELDS_PAYLOAD_AGENDA;
};
typedef struct RpsPayl_Agenda_st RpsAgenda_t;

#define RPS_THE_AGENDA_OBJECT RPS_ROOT_OB(_1aGtWm38Vw701jDhZn)	//"the_agenda"∈agenda

void rps_run_agenda (int nbthreads);
// the below function is approximate.... It could return true a µs
// before the agenda stops, etc...  Could be used in some run-time
// assert tests...
extern volatile bool rps_agenda_is_running (void);


////////////////////////////////////////////////////////////////
extern void rps_load_initial_heap (void);
extern void rps_dump_heap (rps_callframe_t * frame, const char *dir);	/// dump into DIR or
						/// else into
						/// rps_dump_directory
extern void rps_abort (void) __attribute__((noreturn));
extern void rps_backtrace_print (struct backtrace_state *state, int skip,
				 FILE * f);

extern void rps_emit_gplv3plus_notice (FILE * fil, const char *name,
				       const char *lineprefix,
				       const char *linesuffix);

extern void rps_fatal_stop_at (const char *fil, int lineno)
  __attribute__((noreturn));

#define RPS_FATAL_AT_BIS(Fil,Lin,Fmt,...) do {			\
    fprintf (stderr,						\
	    "RefPerSys FATAL:%s:%d: <%s>\n " Fmt "\n\n",	\
            Fil, Lin, __func__, ##__VA_ARGS__);			\
    fflush (stderr);						\
    rps_fatal_stop_at(Fil,Lin); } while(0)

#define RPS_FATAL_AT(Fil,Lin,Fmt,...) RPS_FATAL_AT_BIS(Fil,Lin,Fmt,##__VA_ARGS__)
#define RPS_FATAL(Fmt,...) RPS_FATAL_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)


#include "include/terminal_rps.h"

#define RPS_MANIFEST_FORMAT "RefPerSysFormat2021A"

#include "include/assert_rps.h"

extern void rps_allocation_initialize (void);	/* early initialization */

extern void *alloc0_at_rps (size_t sz, const char *fil, int lineno);
#define RPS_ALLOC_ZEROED(Sz) alloc0_at_rps((Sz),__FILE__,__LINE__)

extern void *alloczone_at_rps (size_t bytsz, int8_t type,
			       const char *fil, int lineno);
#define RPS_ALLOC_ZONE(Bsz,Ty) alloczone_at_rps((Bsz),(Ty),__FILE__,__LINE__)
#define RPS_MAX_ZONE_SIZE (size_t)(1L<<24)

// block every zone allocation, to be able to start the garbage collector
extern void block_zone_allocation_at_rps (const char *file, int lineno);
#define RPS_BLOCK_ZONE_ALLOCATION() block_zone_allocation_at_rps(__FILE__,__LINE__)
extern void permit_zone_allocation_at_rps (const char *file, int lineno);
#define RPS_PERMIT_ZONE_ALLOCATION() permit_zone_allocation_at_rps(__FILE__,__LINE__)

extern pid_t rps_gettid (void);
extern double rps_clocktime (clockid_t);

//////////////// tagged integer values
extern RpsValue_t rps_tagged_integer_value (intptr_t i);
extern bool rps_is_tagged_integer (const RpsValue_t v);
extern intptr_t rps_value_to_integer (const RpsValue_t v);	/* gives 0 for a non-tagged integer */


/*******************************************************************
 * GTK user interface
 *******************************************************************/
extern void rps_run_gui (int *pargc, char **argv);

//////////////////////////////////////////////////////////////////
/// C code can refer to root objects
#define RPS_ROOT_OB(Oid) rps_rootob##Oid
#define RPS_INSTALL_ROOT_OB(Oid) extern RpsObject_t* RPS_ROOT_OB(Oid);
#include "generated/rps-roots.h"

#endif /*REFPERSYS_INCLUDED */
//// end of file Refpersys.h
