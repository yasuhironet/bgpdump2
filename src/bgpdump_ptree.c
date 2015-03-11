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

#include "bgpdump_opt.h"
#include "bgpdump_query.h"
#include "bgpdump_route.h"

void
ptree_list (struct ptree *ptree)
{
  uint64_t count = 0;
  struct ptree_node *x;
  struct bgp_route *br;
  char buf[64], buf2[64];

  printf ("listing ptree.\n");
  for (x = ptree_head (ptree); x; x = ptree_next (x))
    {
      if (x->data)
        {
          br = x->data;
          inet_ntop (qaf, br->prefix, buf, sizeof (buf));
          inet_ntop (qaf, br->nexthop, buf2, sizeof (buf2));
          printf ("%s/%d: %s\n", buf, br->prefix_length, buf2);
          count++;
        }
      if (debug)
        ptree_node_print (x);
    }
  printf ("number of routes: %llu\n", (unsigned long long) count);
}

void
ptree_query (struct ptree *ptree,
             struct query *query_table, uint64_t query_size)
{
  int i;
  struct ptree_node *x;

  for (i = 0; i < query_size; i++)
    {
      char *query = query_table[i].destination;
      char *answer = query_table[i].nexthop;
      int plen = (qaf == AF_INET ? 32 : 128);
      x = ptree_search (query, plen, ptree);
      if (x)
        {
          struct bgp_route *route = x->data;
          memcpy (answer, route->nexthop, MAX_ADDR_LENGTH);
          if (! benchmark)
            route_print (route);
        }
      else if (! benchmark)
        {
          char buf[64];
          inet_ntop (qaf, query, buf, sizeof (buf));
          printf ("%s: no route found.\n", buf);
        }
    }
}

