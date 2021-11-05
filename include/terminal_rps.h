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

#ifndef REFPERSYS_INCLUDE_TERMINAL_RPS_H_INCLUDED
#define REFPERSYS_INCLUDE_TERMINAL_RPS_H_INCLUDED


extern bool rps_terminal_is_escaped;
extern bool rps_terminal_has_stderr;
extern bool rps_terminal_has_stdout;


// adapted from https://github.com/bstarynk
#define RPS_TERMINAL_NORMAL_ESCAPE \
  (rps_terminal_is_escaped ? "\033[0m" : "")

#define RPS_TERMINAL_BOLD_ESCAPE \
  (rps_terminal_is_escaped ? "\033[1m" : "")

#define RPS_TERMINAL_FAINT_ESCAPE \
  (rps_terminal_is_escaped ? "\033[2m" : "")

#define RPS_TERMINAL_ITALICS_ESCAPE \
  (rps_terminal_is_escaped ? "\033[3m" : "")

#define RPS_TERMINAL_UNDERLINE_ESCAPE \
  (rps_terminal_is_escaped ? "\033[4m" : "")

#define RPS_TERMINAL_BLINK_ESCAPE \
  (rps_terminal_is_escaped ? "\033[5m" : "")


#endif /* !REFPERSYS_INCLUDE_TERMINAL_RPS_H_INCLUDED */

