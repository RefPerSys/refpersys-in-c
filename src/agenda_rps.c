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
 * is called the number of pthreads NPT, and should be settable by some
 * program option, e.g. --nb-threads=4; that number of jobs should be more
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
 * + The NPT agenda threads.  These threads can use RPS_ALLOC_ZONE but
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
  json_t *jpriolow = json_object_get (jv, "priority_low");
  json_t *jprionormal = json_object_get (jv, "priority_normal");
  json_t *jpriohigh = json_object_get (jv, "priority_high");
  RpsAgenda_t *agenpayl		//
    = RPS_ALLOC_ZONE (sizeof (RpsAgenda_t), -RpsPyt_Agenda);
  RpsObject_t *obpriolow =	//
    jpriolow ? (rps_loader_json_to_object (ld, jpriolow)) : NULL;
  if (obpriolow)
    agenpayl->agenda_que[AgPrio_Low] = obpriolow;
  RpsObject_t *obprionormal =	//
    jprionormal ? (rps_loader_json_to_object (ld, jprionormal)) : NULL;
  if (obprionormal)
    agenpayl->agenda_que[AgPrio_Normal] = obprionormal;
  RpsObject_t *obpriohigh =	//
    jpriohigh ? (rps_loader_json_to_object (ld, jpriohigh)) : NULL;
  if (obpriohigh)
    agenpayl->agenda_que[AgPrio_Normal] = obpriohigh;
  rps_object_put_payload (obj, agenpayl);
  RPS_ASSERT (obj == RPS_THE_AGENDA_OBJECT);
}				/* end rpsldpy_agenda */


enum rps_agenda_thread_state_en
{
  RpsAgTh__None,
  RpsAgTh_Idle,
  RpsAgTh_Running,
  RpsAgTh_WantGC,
  RpsAgTh_WantDump,
};

volatile atomic_bool rps_agenda_running;
struct rps_agenda_thread_descr_st
{
  int16_t agth_index;
  volatile enum rps_agenda_thread_state_en agth_state;
  pthread_t agth_pthread;
  char agth_thname[16];
  RpsObject_t *agth_curtasklet;
  void *agth_bottomstack;
  volatile atomic_ulong agth_loop_counter;
} rps_agenda_threadarr[RPS_MAX_NB_THREADS + 2];

pthread_attr_t rps_agenda_attrthread;
pthread_cond_t rps_agenda_changed_cond = PTHREAD_COND_INITIALIZER;

void
rps_stop_agenda (void)
{
  atomic_store (&rps_agenda_running, false);
  pthread_cond_broadcast (&rps_agenda_changed_cond);
#warning rps_stop_agenda should be coded
}				/* end rps_stop_agenda */

void *
rps_thread_routine (void *ptr)
{
  volatile intptr_t botstack[4] = { 0, 0, 0, 0 };
  RPS_ASSERT (ptr > (void *) rps_agenda_threadarr
	      && ptr <= (void *) (rps_agenda_threadarr + RPS_MAX_NB_THREADS));
  struct rps_agenda_thread_descr_st *d = ptr;
  d->agth_index = d - rps_agenda_threadarr;
  d->agth_pthread = pthread_self ();
  snprintf (d->agth_thname, sizeof (d->agth_thname), "rpsagth#%d",
	    d->agth_index);
  pthread_setname_np (pthread_self (), d->agth_thname);
  d->agth_bottomstack = &botstack;
  usleep (5000 + d->agth_index * 3333);
  printf ("thread#%d:%ld..(%s:%d)\n", d->agth_index, (long) pthread_self (),
	  __FILE__, __LINE__);
  usleep (100 + d->agth_index * 333);
  /******
   * TODO: each thread routine should loop while the agenda is active:
   *
   * + if a GC is needed (and this happens rarely), wait till every
   * other thread is non-active and capable of garbage collection.
   * The garbage collection should probably be running only in the
   * main thread, but this has to be designed....
   *
   * + choose one tasklet to run in the_agenda and remove it
   *
   * + execute that tasklet, hopefully in a dozen of milliseconds
   *****/
  RpsObject_t *obtasklet = NULL;
  while (atomic_load (&rps_agenda_running))
    {
      obtasklet = NULL;
      uint64_t count = atomic_fetch_add (&d->agth_loop_counter, 1) + 1;
      /// We sometimes sleep to give other threads the opportunity to
      /// run.  When debugged and code reviewed, the constants below
      /// (100µs and the 333µs factor) should be lowered.
      if ((count - 1) % 64 == 0)
	usleep (100 + d->agth_index * 333);
      pthread_mutex_lock (&RPS_THE_AGENDA_OBJECT->ob_mtx);
      RpsAgenda_t *paylagenda = RPS_THE_AGENDA_OBJECT->ob_payload;
      RPS_ASSERT (paylagenda != NULL
		  && paylagenda->zm_type == -RpsPyt_Agenda);
      for (enum RpsAgendaPrio_en prio = AgPrio_High; prio >= AgPrio_Low;
	   prio--)
	{
	  RpsObject_t *obque = paylagenda->agenda_que[prio];
	  if (!obque)
	    continue;
	  RPS_ASSERT (rps_is_valid_object (obque));
	  obtasklet = rps_object_deque_pop_first (obque);
	  if (obtasklet != NULL)
	    break;
	};			/* end for enum RpsAgendaPrio_en prio... */
      if (obtasklet == NULL)
	{
	  struct timespec ts = { 0, 0 };
	  clock_gettime (CLOCK_REALTIME, &ts);
	  ts.tv_sec += 1;
	  pthread_cond_wait (&rps_agenda_changed_cond, &ts);
	}
      pthread_mutex_unlock (&RPS_THE_AGENDA_OBJECT->ob_mtx);
      if (obtasklet != NULL)
	{
	  /* should check the obtasklet and run it */
	  RPS_ASSERT (rps_is_valid_object (obtasklet));
	  pthread_mutex_lock (&obtasklet->ob_mtx);
#warning should check and run the obtasklet in incomplete rps_thread_routine
	  RPS_FATAL ("unimplemented rps_thread_routine when found obtasklet");
	  RpsTasklet_t* payltasklet = obtasklet->ob_payload;
	  if (payltasklet && payltasklet->zm_type == -RpsPyt_Tasklet) {
	    RpsClosure_t* clos = payltasklet->tasklet_closure;
	    double obsoltime = payltasklet->tasklet_obsoltime;
	    if (obsoltime <= rps_clocktime(CLOCK_REALTIME)) {
#warning rps_thread_routine should apply clos...
	    }
	  }
	  pthread_mutex_unlock (&obtasklet->ob_mtx);
	}
#warning incomplete rps_thread_routine
      usleep (10000);
      RPS_FATAL ("incomplete rps_thread_routine %d", d->agth_index);
    };				/* end while rps_agenda_running */
}				/* end rps_thread_routine */

void
rps_run_agenda (int nbthreads)
{
  static size_t agthread_stacksize = (6 * 1024 * 1024);
#warning rps_run_agenda unimplemented
  if (nbthreads < RPS_MIN_NB_THREADS || nbthreads > RPS_MAX_NB_THREADS)
    RPS_FATAL ("rps_run_agenda with invalid nbthreads %d", nbthreads);
  pthread_attr_init (&rps_agenda_attrthread);
  pthread_attr_setstacksize (&rps_agenda_attrthread, agthread_stacksize);
  pthread_attr_setguardsize (&rps_agenda_attrthread, 1024 * 1024);
  pthread_attr_setdetachstate (&rps_agenda_attrthread,
			       PTHREAD_CREATE_JOINABLE);
  printf ("rps_run_agenda nbthreads=%d\n", nbthreads);
  fflush (NULL);
  atomic_store (&rps_agenda_running, true);
  for (int ix = 1; ix <= nbthreads; ix++)
    {
      pthread_t curth = (pthread_t) (0);
      int err =
	pthread_create (&curth, &rps_agenda_attrthread, rps_thread_routine,
			(void *) (rps_agenda_threadarr + ix));
      if (err)
	RPS_FATAL ("failed to create agenda thread#%d / %d : err#%d (%s)", ix,
		   nbthreads, err, strerror (err));
      usleep (1000);
    }
  usleep (500000);
  RPS_FATAL ("unimplemented rps_run_agenda %d threads", nbthreads);
}				/* end rps_run_agenda */
