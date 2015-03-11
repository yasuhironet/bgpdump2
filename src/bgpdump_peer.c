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

#include "bgpdump_peer.h"
#include "bgpdump_data.h"

struct peer peer_null;
struct peer peer_table[PEER_MAX];
int peer_size = 0;

int peer_spec_index[PEER_INDEX_MAX];
int peer_spec_size = 0;

void
peer_table_init ()
{
  memset (&peer_null, 0, sizeof (struct peer));
  memset (peer_table, 0, sizeof (peer_table));
}

void
peer_print (struct peer *peer)
{
  char buf[64], buf2[64], buf3[64];
  inet_ntop (AF_INET, &peer->bgp_id, buf, sizeof (buf));
  inet_ntop (AF_INET, &peer->ipv4_addr, buf2, sizeof (buf2));
  inet_ntop (AF_INET6, &peer->ipv6_addr, buf3, sizeof (buf3));
  printf ("%s asn:%d [%s|%s]", buf, peer->asnumber, buf2, buf3);
}

void
peer_route_count_show ()
{
  int i;

  printf ("#timestamp,peer1,peer2,...\n");
  printf ("%lu,", (unsigned long) timestamp);
  for (i = 0; i < peer_size; i++)
    {
      printf ("%llu", (unsigned long long) peer_table[i].route_count);
      if (i < peer_size - 1)
        printf (",");
    }
  printf ("\n");
  fflush (stdout);
}

void
peer_route_count_clear ()
{
  int i;
  for (i = 0; i < peer_size; i++)
    peer_table[i].route_count = 0;
}

void
peer_route_count_by_plen_show ()
{
  int i, j;

  for (i = 0; i < peer_size; i++)
    {
      if (peer_spec_size)
        {
          int match = 0;
          for (j = 0; j < peer_spec_size; j++)
            if (i == peer_spec_index[j])
              match++;

          if (! match)
            continue;
        }

      printf ("%lu,", (unsigned long) timestamp);
      for (j = 0; j < 33; j++)
        {
          printf ("%llu", peer_table[i].route_count_by_plen[j]);
          if (j < 32)
            printf (",");
        }
      printf ("\n");
    }
}

void
peer_route_count_by_plen_clear ()
{
  int i, j;
  for (i = 0; i < peer_size; i++)
    for (j = 0; j < 33; j++)
      peer_table[i].route_count_by_plen[j] = 0;
}

