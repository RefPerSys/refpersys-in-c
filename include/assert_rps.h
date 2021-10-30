/****************************************************************
 * file include/assert_rps.h
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *      Defines macros related to assertion macros.
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


#ifndef REFPERSYS_INCLUDE_ASSERT_RPS_H_INCLUDED
#define REFPSERSY_INCLUDE_ASSERT_RPS_H_INCCLUDED


#ifndef NDEBUG
///
#define RPS_ASSERT_AT_BIS(Fil,Lin,Func,Cond) do {		\
  if (!(Cond)) {						\
  fprintf(stderr, "\n\n"					\
	  "%s*** RefPerSys ASSERT failed: %s%s\n"		\
	  "%s:%d: {%s}\n\n",					\
	  (rps_stderr_istty?RPS_TERMINAL_BOLD_ESCAPE:""),	\
          #Cond,						\
	  (rps_stderr_istty?RPS_TERMINAL_NORMAL_ESCAPE:""),	\
	  Fil,Lin,Func);					\
  rps_fatal_stop_at(Fil,Lin); }} while(0)

#define RPS_ASSERT_AT(Fil,Lin,Func,Cond) RPS_ASSERT_AT_BIS(Fil,Lin,Func,Cond)
#define RPS_ASSERT(Cond) RPS_ASSERT_AT(__FILE__,__LINE__,__PRETTY_FUNCTION__,(Cond))

#define RPS_ASSERTPRINTF_AT_BIS(Fil,Lin,Func,Cond,Fmt,...) do {	\
    if (!(Cond)) {						\
      fprintf(stderr, "\n\n"					\
	      "%s*** RefPerSys ASSERTPRINTF failed:%s %s\n"	\
	      "%s:%d: {%s}\n",					\
	  (rps_stderr_istty?RPS_TERMINAL_BOLD_ESCAPE:""),	\
		#Cond,						\
	  (rps_stderr_istty?RPS_TERMINAL_NORMAL_ESCAPE:""),	\
	      Fil, Lin, Func);					\
      fprintf(stderr, "!*!*! " Fmt "\n\n", ##__VA_ARGS__);	\
      rps_fatal_stop_at(Fil, Lin); }} while(0)

#define RPS_ASSERTPRINTF_AT(Fil,Lin,Func,Cond,Fmt,...) RPS_ASSERTPRINTF_AT_BIS(Fil,Lin,Func,Cond,Fmt,##__VA_ARGS__)
#define RPS_ASSERTPRINTF(Cond,Fmt,...) RPS_ASSERTPRINTF_AT(__FILE__,__LINE__,__PRETTY_FUNCTION__,(Cond),Fmt,##__VA_ARGS__)
#else
#define RPS_ASSERT(Cond) do { if (false && (Cond)) rps_fatal_stop_at(__FILE_,__LINE__); } while(0)
#define RPS_ASSERTPRINTF(Cond,Fmt,...)  do { if (false && (Cond)) \
      fprintf(stderr, Fmt "\n", ##__VA_ARGS__); } while(0)

#endif /*NDEBUG*/
#endif /* !defined REFPERSYS_INCLUDE_ASSERT_RPS_H_INCLUDED */

