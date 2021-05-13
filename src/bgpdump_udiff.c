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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "queue.h"
#include "ptree.h"

#include "bgpdump_option.h"
#include "bgpdump_route.h"
#include "bgpdump_peer.h"

void
bgpdump_udiff_compare (uint32_t sequence_number)
{
  struct bgp_route *route;
  struct bgp_route *rt0 = peer_route_table[0];
  struct bgp_route *rt1 = peer_route_table[1];
  struct ptree *pt0 = peer_ptree[0];
  struct ptree *pt1 = peer_ptree[1];

  if (udiff_verbose)
    {
      printf ("seq: %lu\n", (unsigned long) sequence_number);
      if (! IS_ROUTE_NULL (&rt0[sequence_number]))
        {
          route = &rt0[sequence_number];
          printf ("{");
          route_print (route);
        }
      if (! IS_ROUTE_NULL (&rt1[sequence_number]))
        {
          route = &rt1[sequence_number];
          printf ("}");
          route_print (route);
        }
    }

  /* only in left */
  if (! IS_ROUTE_NULL (&rt0[sequence_number]) &&
      IS_ROUTE_NULL (&rt1[sequence_number]))
    {
      route = &rt0[sequence_number];
      if (! udiff_lookup)
        {
          printf ("-");
          route_print (route);
        }
      else
        {
          struct ptree_node *x;
          int plen = (qafi == AF_INET ? 32 : 128);
          x = ptree_search ((char *)&route->prefix, plen, pt1);
          if (x)
            {
              /* only in left but also entirely reachable in right */
              struct bgp_route *other = x->data;
              if (other->flag == '>')
                {
                  route->flag = '(';
                  printf ("(");
                }
              else
                {
                  route->flag = '-';
                  printf ("-");
                }
              route_print (route);
            }
          else
            {
              /* only in left and unreachable in right (maybe partially) */
              route->flag = '<';
              printf ("<");
              route_print (route);
            }
        }
    }

  /* only in right */
  if (IS_ROUTE_NULL (&rt0[sequence_number]) &&
      ! IS_ROUTE_NULL (&rt1[sequence_number]))
    {
      route = &rt1[sequence_number];
      if (! udiff_lookup)
        {
          printf ("+");
          route_print (route);
        }
      else
        {
          struct ptree_node *x;
          int plen = (qafi == AF_INET ? 32 : 128);
          x = ptree_search ((char *)&route->prefix, plen, pt0);
          if (x)
            {
              /* only in right but also entirely reachable in left */
              struct bgp_route *other = x->data;
              if (other->flag == '<')
                {
                  route->flag = ')';
                  printf (")");
                }
              else
                {
                  route->flag = '+';
                  printf ("+");
                }
              route_print (route);
            }
          else
            {
              /* only in right and unreachable in left (maybe partially) */
              route->flag = '>';
              printf (">");
              route_print (route);
            }
        }
    }

  /* exist in both */
  if (! IS_ROUTE_NULL (&rt0[sequence_number]) &&
      ! IS_ROUTE_NULL (&rt1[sequence_number]) &&
      rt0[sequence_number].prefix_length > 0)
    {
      int plen = rt0[sequence_number].prefix_length - 1;

      if (udiff_lookup)
        {
          struct ptree_node *x;

          route = &rt0[sequence_number];
          x = ptree_search ((char *)&route->prefix, plen, pt1);
          if (x)
            {
              /* the shorter in right was '>' */
              struct bgp_route *other = x->data;
              if (other->flag == '>')
                {
                  route->flag = '(';
                  printf ("(");
                  route_print (route);
                }
            }

          route = &rt1[sequence_number];
          x = ptree_search ((char *)&route->prefix, plen, pt0);
          if (x)
            {
              /* the shorter in left was '<' */
              struct bgp_route *other = x->data;
              if (other->flag == '<')
                {
                  route->flag = ')';
                  printf (")");
                  route_print (route);
                }
            }
        }
    }
}

