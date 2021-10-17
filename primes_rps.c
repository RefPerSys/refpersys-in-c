// file primes_rps.c
// SPDX-License-Identifier: GPL-3.0-or-later
/***
 * Description:
 *      This file is part of the Reflective Persistent System. See refpersys.org
 *
 *
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      Abhishek Chakravarti <abhishek@taranjali.org>
 *      Nimesh Neema <nimeshneema@gmail.com>
 *
 *      © Copyright 2019 - 2021 The Reflective Persistent System Team
 *      team@refpersys.org & http://refpersys.org/
 *

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***/

#include <stdint.h>


// A zero-terminated array of primes, gotten with something similar to :
//  primesieve  2 32000000000 -p | awk '($1>p+p/10){print $1, ","; p=$1}' | indent | fmt
static const int64_t rps_primes_tab[] = { 2, 3,
  5, 7, 11, 13, 17, 19, 23, 29, 37, 41, 47, 53, 59, 67, 79, 89, 101, 113,
  127, 149, 167, 191, 211, 233, 257, 283, 313, 347, 383, 431, 479, 541,
  599, 659, 727, 809, 907, 1009, 1117, 1229, 1361, 1499, 1657, 1823, 2011,
  2213, 2437, 2683, 2953, 3251, 3581, 3943, 4339, 4783, 5273, 5801, 6389,
  7039, 7753, 8537, 9391, 10331, 11369, 12511, 13763, 15149, 16673, 18341,
  20177, 22229, 24469, 26921, 29629, 32603, 35869, 39461, 43411, 47777,
  52561, 57829, 63617, 69991, 76991, 84691, 93169, 102497, 112757, 124067,
  136481, 150131, 165161, 181693, 199873, 219871, 241861, 266051, 292661,
  321947, 354143, 389561, 428531, 471389, 518533, 570389, 627433, 690187,
  759223, 835207, 918733, 1010617, 1111687, 1222889, 1345207, 1479733,
  1627723, 1790501, 1969567, 2166529, 2383219, 2621551, 2883733, 3172123,
  3489347, 3838283, 4222117, 4644329, 5108767, 5619667, 6181639, 6799811,
  7479803, 8227787, 9050599, 9955697, 10951273, 12046403, 13251047,
  14576161, 16033799, 17637203, 19400929, 21341053, 23475161, 25822679,
  28404989, 31245491, 34370053, 37807061, 41587807, 45746593, 50321261,
  55353391, 60888739, 66977621, 73675391, 81042947, 89147249, 98061979,
  107868203, 118655027, 130520531, 143572609, 157929907, 173722907,
  191095213, 210204763, 231225257, 254347801, 279782593, 307760897,
  338536987, 372390691, 409629809, 450592801, 495652109, 545217341,
  599739083, 659713007, 725684317, 798252779, 878078057, 965885863,
  1062474559, 1168722059, 1285594279, 1414153729, 1555569107, 1711126033,
  1882238639, 2070462533, 2277508787, 2505259681, 2755785653, 3031364227,
  3334500667, 3667950739, 4034745863, 4438220467, 4882042547, 5370246803,
  5907271567, 6497998733, 7147798607, 7862578483, 8648836363, 9513720011,
  10465092017, 11511601237, 12662761381, 13929037523, 15321941293,
  16854135499, 18539549051, 20393503969, 22432854391, 24676139909,
  27143753929, 29858129341,
  0
};

const unsigned rps_nb_primes_in_tab =
  sizeof (rps_primes_tab) / sizeof (rps_primes_tab[0]) - 1;

int64_t
rps_prime_of_index (int ix)
{
  if (ix >= 0 && ix <= rps_nb_primes_in_tab)
    return rps_primes_tab[ix];
  else
    return 0;
}				/* end rps_prime_of_index */

int
rps_index_of_prime (int64_t n)
{
  if (n <= 1)
    return -1;
  if (n == 2)
    return 0;
  int lo = 0, hi = rps_nb_primes_in_tab;
  while (lo + 6 < hi)
    {
      int md = (lo + hi) / 2;
      if (rps_primes_tab[md] > n)
	hi = md;
      else if (rps_primes_tab[md] == n)
	return md;
      else
	lo = md;
    };
  for (int ix = lo; ix < hi; ix++)
    if (rps_primes_tab[ix] == n)
      return ix;
  return -1;
}				/* end rps_index_of_prime */

int64_t
rps_prime_above (int64_t n)
{
  int lo = 0, hi = rps_nb_primes_in_tab;
  while (lo + 6 < hi)
    {
      int md = (lo + hi) / 2;
      if (rps_primes_tab[md] > n)
	hi = md;
      else
	lo = md;
    };
  for (int ix = lo; ix < hi; ix++)
    if (rps_primes_tab[ix] > n)
      return rps_primes_tab[ix];
  return 0;
}				/* end rps_prime_above */

int64_t
rps_prime_below (int64_t n)
{
  int lo = 0, hi = rps_nb_primes_in_tab;
  if (n <= 2)
    return 0;
  while (lo + 6 < hi)
    {
      int md = (lo + hi) / 2;
      if (rps_primes_tab[md] > n)
	hi = md;
      else
	lo = md;
    };
  for (int ix = lo; ix < hi; ix++)
    if (rps_primes_tab[ix] < n)
      return rps_primes_tab[ix];
  return 0;
}				/* end rps_prime_below */



/********************* end of file primes_rps.c ************/
