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
#include <stdint.h>
#include <stdatomic.h>
#include <math.h>
#include <limits.h>

// man7.org/linux/man-pages/man3/gnu_get_libc_version.3.html
#include <gnu/libc-version.h>


// GNU libunistring www.gnu.org/software/libunistring/
// we use UTF-8 strings
#include "unistr.h"


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

extern struct backtrace_state* rps_backtrace_common_state;
extern const char* rps_progname; /* argv[0] of main */
extern void* rps_dlhandle;	/* global dlopen handle */

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
  RpsTy_TupleOb,
  RpsTy_SetOb,
  RpsTy_Object,
  RpsTy_GtkWiget,		/* some GtkWidget* pointer; of course GTK widgets are not persisted */
  RpsTy_File,		/* some opened FILE* handle; of course they are not persisted */
  RpsTy__LAST
};

/// a value is a word
typedef uintptr_t RpsValue_t;

typedef struct RpsZoneObject_st  RpsObject_t; ///// forward declaration

/// object ids, also known as oid-s
struct RpsOid_st {
  uint64_t id_hi, id_lo;
};
typedef struct RpsOid_st RpsOid_t;

/// an hash has 32 bits and conventionally is never 0
typedef uint32_t RpsHash_t;

/// a payload is not a proper value, but garbaged collected as if it was one....
/// payload types - prefix is RpsPyt
enum {
  RpsPyt__NONE,
  RpsPyt_AttrTable,		/* associate objects to values */
  RpsPyt_StringBuf,		/* mutable string buffer */
  RpsPyt__LAST
};

/// the fields in every zoned memory -value or payload-; we use macros to mimic C field inheritance
#define RPSFIELDS_ZONED_MEMORY \
  unsigned char zm_type; \
  atomic_uchar zm_gcmark; \
  uint32_t zm_size


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


////////////// string values
#define RPSFIELDS_STRING \
  RPSFIELDS_ZONED_VALUE; \
  char cstr[];			/* flexible array zone, zv_size is length in UTF8 characters, not in bytes */

struct RpsZoneString_st { RPSFIELDS_STRING; };
typedef struct RpsZoneString_st RpsString_t; /* for RpsTy_String */

/////////////// boxed JSON values
#define RPSFIELDS_JSON \
  RPSFIELDS_ZONED_VALUE; \
  json_t *json

struct RpsZoneJson_st { RPSFIELDS_JSON; };
typedef struct RpsZoneJson_st RpsJson_t; /* for RpsTy_Json */

/////////////// tuple of objects value
#define RPSFIELDS_TUPLEOB \
  RPSFIELDS_ZONED_VALUE; \
  RpsObject_t* tuple_comp[]	/* zv_size is the number of components */

struct RpsZoneTupleOb_st { RPSFIELDS_TUPLEOB; };
typedef struct RpsZoneTupleOb_st RpsTupleOb_t; /* for RpsTy_TupleOb */

/////////////// set of objects value
#define RPSFIELDS_SETOB \
  RPSFIELDS_ZONED_VALUE; \
  RpsObject_t* set_elem[]	/* zv_size is the number of elements, and they are ordered by oid */

struct RpsZoneSetOb_st { RPSFIELDS_SETOB; };
typedef struct RpsZoneTupleOb_st RpsTupleOb_t; /* for RpsTy_TupleOb */


extern void rps_load_initial_heap(void);
extern void rps_abort(void) __attribute__((noreturn));

extern pid_t rps_gettid(void);
extern double rps_clocktime(clockid_t);

#endif /*REFPERSYS_INCLUDED*/
//// end of file Refpersys.h
