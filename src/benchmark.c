/*
 * Bgpdump2: A Tool to Read and Compare the BGP RIB Dump Files.
 * Copyright (C) 2015.  Yasuhiro Ohara <yasu@nttv6.jp>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>

struct timeval start, end, diff;

void
benchmark_start ()
{
  gettimeofday (&start, NULL);
}

void
benchmark_stop ()
{
  gettimeofday (&end, NULL);
}

void
benchmark_print (uint64_t query_size)
{
  diff.tv_sec = end.tv_sec;
  diff.tv_usec = end.tv_usec;
  if (end.tv_usec < start.tv_usec)
    {
      diff.tv_sec--;
      diff.tv_usec += 1000000;
    }
  diff.tv_usec -= start.tv_usec;
  diff.tv_sec -= start.tv_sec;

  diff.tv_usec += 1000000 * diff.tv_sec;
  printf ("%llu query: %lu usec (%f Mlps)\n",
          (unsigned long long) query_size,
          (unsigned long) diff.tv_usec,
          (double) query_size / diff.tv_usec * 1000000 / 1000000);
}

