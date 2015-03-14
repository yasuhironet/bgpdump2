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
#include <string.h>
#include <strings.h>
#include <stdlib.h>

unsigned long long
resolv_number (char *notation, char **endptr)
{
  unsigned long long number, unit;
  char digits[64];
  unsigned long digit;
  int len;
  char *p;

  snprintf (digits, sizeof (digits), "%s", notation);
  len = strlen (digits);

  unit = 1;
  if (len > 3)
    {
      p = strstr (digits, "KiB");
      if (! p)
        p = strstr (digits, "MiB");
      if (! p)
        p = strstr (digits, "GiB");
      if (! p)
        p = strstr (digits, "TiB");
      if (p)
        {
          switch (*p)
            {
            case 'K':
              unit = 1024ULL;
              break;
            case 'M':
              unit = 1024ULL * 1024ULL;
              break;
            case 'G':
              unit = 1024ULL * 1024ULL * 1024ULL;
              break;
            case 'T':
              unit = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
              break;
            default:
              unit = 1ULL;
              break;
            }
          *p = '\0';
          len = strlen (digits);
        }
    }
  if (len > 1)
    {
      switch (digits[len - 1])
        {
        case 'k':
        case 'K':
          unit = 1000ULL;
          digits[len - 1] = '\0';
          break;
        case 'm':
        case 'M':
          unit = 1000ULL * 1000ULL;
          digits[len - 1] = '\0';
          break;
        case 'g':
        case 'G':
          unit = 1000ULL * 1000ULL * 1000ULL;
          digits[len - 1] = '\0';
          break;
        case 'T':
          unit = 1000ULL * 1000ULL * 1000ULL * 1000ULL;
          digits[len - 1] = '\0';
          break;
        case 'P':
          unit = 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL;
          digits[len - 1] = '\0';
          break;
        default:
          break;
        }
    }

  digit = strtoul (digits, endptr, 0);
  number = digit * unit;

  return number;
}

