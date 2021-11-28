/****************************************************************
 * file alloc_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It has the memory allocator and possibly the garbage collector.
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

/* Before the bootstrap - generation of most of the C code of the
   system, all memory zones for values and payloads are malloc-ed.
   Once most the garbage collection code is generated, we would use a
   more fancy and faster allocation scheme - generational copying GC
   techniques.... */

#define RPS_MAX_ALLOCSIZE (1L<<24)

/// allocate a zeroed memory zone; these should be manually freed, and
/// are almost always allocated thru the RPS_ALLOC_ZEROED macro defined
/// in header file Refpersys.h
void *
alloc0_at_rps (size_t sz, const char *file, int lineno)
{
  /* It could happen that our GC will forbid allocation during
     GC-marking time.... We may need a condition variable later here. */
#warning we probably want some synchronization in alloc0_at_rps
  if (sz < sizeof (void *))
    sz = sizeof (void *);
  if (sz > RPS_MAX_ALLOCSIZE)
    RPS_FATAL_AT (file, lineno, "too big allocation %zd", sz);
  void *z = malloc (sz);
  if (!z)
    RPS_FATAL_AT (file, lineno, "failed to allocate %zd bytes (%m).", sz);
  memset (z, 0, sz);
  return z;
}				/* end alloc0_at_rps */


#define RPS_NB_ZONED_CHAINS 61
struct RpsZonedMemory_st *rps_zoned_chainarr[RPS_NB_ZONED_CHAINS];
pthread_mutex_t rps_zoned_mtxarr[RPS_NB_ZONED_CHAINS];
pthread_cond_t rps_zoned_condarr[RPS_NB_ZONED_CHAINS];
volatile atomic_bool rps_zoned_alloc_blocked;

#define RPS_ALLOC_WAIT_MILLISEC 25	/*amount of time we wait for zoned alloc to be unblocked */

// we need to use condvar, etc...
#warning some pthread code is missing in routine below

/* We sometimes need to block every zone allocation, e.g. to let the
   garbage collector run....  See comment about pthreads in
   agenda_rps.c */
void				/* called by RPS_BLOCK_ZONE_ALLOCATION */
block_zone_allocation_at_rps (const char *file, int lineno)
{
  RPS_ASSERT (file != NULL && lineno > 0);
  atomic_store (&rps_zoned_alloc_blocked, TRUE);
}				/* end block_zone_allocation_at_rps */


void
permit_zone_allocation_at_rps (const char *file, int lineno)
{
  RPS_ASSERT (file != NULL && lineno > 0);
  atomic_store (&rps_zoned_alloc_blocked, FALSE);
}				/* end permit_zone_allocation_at_rps */

/// allocate a garbage collected and dynamically typed memory zone;
/// these should never be manually freed outside of our GC, and are
/// almost always allocated thru the RPS_ALLOC_ZONE macro defined in
/// header file Refpersys.h
void *
alloczone_at_rps (size_t bytsz, int8_t type, const char *file, int lineno)
{
  RPS_ASSERT (file != NULL && lineno > 0);
  if (bytsz > RPS_MAX_ZONE_SIZE)
    RPS_FATAL_AT (file, lineno, "too big memory zone %zd requested", bytsz);
  if (type == 0)
    RPS_FATAL_AT (file, lineno,
		  "invalid zero type for memory zone of %zd bytes", bytsz);
  RPS_ASSERT (bytsz >= sizeof (struct RpsZonedMemory_st));
  unsigned h =
    (rps_hash_cstr (file) + bytsz + type + lineno) % RPS_NB_ZONED_CHAINS;
  pthread_mutex_lock (&rps_zoned_mtxarr[h]);
  while (atomic_load (&rps_zoned_alloc_blocked))
    {
      struct timespec ts = { 0, 0 };
      clock_gettime (CLOCK_REALTIME, &ts);
      ts.tv_nsec += RPS_ALLOC_WAIT_MILLISEC * (1000 * 1000);
      while (ts.tv_nsec > (1000 * 1000 * 1000))
	{
	  ts.tv_sec++;
	  ts.tv_nsec -= (1000 * 1000 * 1000);
	};
      pthread_cond_timedwait (&rps_zoned_condarr[h], &rps_zoned_mtxarr[h],
			      &ts);
    }
  struct RpsZonedMemory_st *zm =
    (struct RpsZonedMemory_st *) alloc0_at_rps (bytsz, file, lineno);
  atomic_init (&zm->zm_gcmark, 0);
  atomic_init (&zm->zm_atype, type);
  atomic_init (&zm->zm_gclink, rps_zoned_chainarr[h]);
  rps_zoned_chainarr[h] = zm;
  pthread_mutex_unlock (&rps_zoned_mtxarr[h]);
  return (void *) zm;
}				/* end alloczone_at_rps */


/// initialization routine, to be called once and early in main.
void
rps_allocation_initialize (void)
{
  pthread_mutexattr_t mtxat;
  memset (&mtxat, 0, sizeof (mtxat));
  pthread_mutexattr_init (&mtxat);
  pthread_mutexattr_settype (&mtxat, PTHREAD_MUTEX_ERRORCHECK_NP);
  for (int i = 0; i < RPS_NB_ZONED_CHAINS; i++)
    {
      pthread_mutex_init (&rps_zoned_mtxarr[i], &mtxat);
      pthread_cond_init (&rps_zoned_condarr[i], NULL);
    }
}				/* end rps_allocation_initialize */

/* end of file alloc_rps.c */
