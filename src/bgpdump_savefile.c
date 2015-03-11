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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <assert.h>

#include "queue.h"
#include "ptree.h"

#include "bgpdump_route.h"

#if 0
void
bgpdump_input (char *input, struct ptree *ptree)
{
  FILE *fp;
  char *p, buf[256];
  struct in_addr prefix, nexthop;
  int plen;
  char *string;
  char buf1[32], buf2[32];
  uint64_t route_size = 0;

  fp = fopen (input, "r");
  if (! fp)
    {
      printf ("cannot open input file: %s\n", input);
      exit (-1);
    }

  assert (ptree);
  route_size = 0;

  while (1)
    {
      p = fgets (buf, sizeof (buf), fp);
      if (! p)
        break;

      memset (&prefix, 0, sizeof (struct in_addr));
      memset (&nexthop, 0, sizeof (struct in_addr));
      string = buf;

      p = strsep (&string, " \t/");
      inet_pton (AF_INET, p, &prefix);

      p = strsep (&string, " \t/");
      plen = (int) strtol (p, NULL, 0);

      p = strsep (&string, " \t/");
      inet_pton (AF_INET, p, &nexthop);

      inet_ntop (AF_INET, &prefix, buf1, sizeof (buf1));
      inet_ntop (AF_INET, &nexthop, buf2, sizeof (buf2));
      printf ("prefix: %s/%d nexthop: %s\n", buf1, plen, buf2);

      routes[route_size].prefix = prefix;
      routes[route_size].prefix_length = plen;
      routes[route_size].nexthop = nexthop;
      ptree_add ((char *)&prefix, plen, &routes[route_size], ptree);
      route_size++;
    }

  fclose (fp);
}

void
bgpdump_output (char *output, struct ptree *ptree)
{
  FILE *fp;
  char buf[256];
  char buf1[32], buf2[32];
  struct ptree_node *x;

  assert (ptree);

  fp = fopen (output, "w");
  if (! fp)
    {
      printf ("cannot open output file: %s\n", output);
      exit (-1);
    }

  for (x = ptree_head (ptree); x; x = ptree_next (x))
    {
      struct bgp_route *r = x->data;
      inet_ntop (AF_INET, &r->prefix, buf1, sizeof (buf1));
      inet_ntop (AF_INET, &r->nexthop, buf2, sizeof (buf2));
      snprintf (buf, sizeof (buf), "%s/%d %s\n",
                buf1, r->prefix_length, buf2);
      fputs (buf, fp);
    }

  fclose (fp);
}


#endif
