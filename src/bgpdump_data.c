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
#include <time.h>
#include <assert.h>

#include "bgpdump.h"
#include "bgpdump_option.h"
#include "bgpdump_data.h"
#include "bgpdump_peer.h"
#include "bgpdump_route.h"
#include "bgpdump_peerstat.h"
#include "bgpdump_udiff.h"

#include "queue.h"
#include "ptree.h"

extern struct bgp_route *diff_table[];
extern struct ptree *diff_ptree[];

#define BUFFER_OVERRUN_CHECK(P,SIZE,END) \
  if ((P) + (SIZE) > (END))              \
    {                                    \
      printf ("buffer overrun.\n");      \
      return;                            \
    }

uint32_t timestamp;
uint16_t mrt_type;
uint16_t mrt_subtype;
uint32_t mrt_length;

uint32_t sequence_number;
uint16_t peer_index;
char prefix[16];
uint8_t prefix_length;

void
bgpdump_process_mrt_header (struct mrt_header *h, struct mrt_info *info)
{
  unsigned long newtime;

  newtime = ntohl (h->timestamp);

  if (verbose && timestamp != newtime)
    {
      struct tm *tm;
      char timebuf[64];
      time_t clock;

      clock = (time_t) newtime;
      tm = localtime (&clock);
      strftime (timebuf, sizeof (timebuf), "%Y/%m/%d %H:%M:%S", tm);
      printf ("new timestamp: %lu (\"%s\")\n",
              (unsigned long) ntohl (h->timestamp), timebuf);
    }

  timestamp = newtime;
  mrt_type = ntohs (h->type);
  mrt_subtype = ntohs (h->subtype);
  mrt_length = ntohl (h->length);

  info->timestamp = newtime;
  info->type = ntohs (h->type);
  info->subtype = ntohs (h->subtype);
  info->length = ntohl (h->length);

  if (show && (debug || detail))
    printf ("MRT Header: ts: %lu type: %hu sub: %hu len: %lu\n",
            (unsigned long) timestamp,
            (unsigned short) mrt_type,
            (unsigned short) mrt_subtype,
            (unsigned long) mrt_length);
}

void
bgpdump_table_v2_peer_entry (int index, char *p, char *data_end, int *retsize)
{
  int size, total = 0;
  uint8_t peer_type;
  int asn4byte, afi_ipv6;
  struct in_addr peer_bgp_id;
  struct in_addr ipv4_addr;
  struct in6_addr ipv6_addr;
  uint16_t peer_as_2byte = 0;
  uint32_t peer_as_4byte = 0;

  char buf[64], buf2[64];

  memset (&ipv4_addr, 0, sizeof (ipv4_addr));
  memset (&ipv6_addr, 0, sizeof (ipv6_addr));

  size = sizeof (peer_type);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  peer_type = *(uint8_t *)p;
  p += size;
  total += size;

#define FLAG_PEER_ADDRESS_IPV6 0x01
#define FLAG_AS_NUMBER_SIZE    0x02
  afi_ipv6 = 0;
  if (peer_type & FLAG_PEER_ADDRESS_IPV6)
    afi_ipv6 = 1;
  asn4byte = 0;
  if (peer_type & FLAG_AS_NUMBER_SIZE)
    asn4byte = 1;

  size = sizeof (peer_bgp_id);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  memcpy (&peer_bgp_id, p, sizeof (peer_bgp_id));
  p += size;
  total += size;

  if (afi_ipv6)
    {
      size = sizeof (ipv6_addr);
      BUFFER_OVERRUN_CHECK(p, size, data_end)
      memcpy (&ipv6_addr, p, size);
      p += size;
      total += size;
    }
  else
    {
      size = sizeof (ipv4_addr);
      BUFFER_OVERRUN_CHECK(p, size, data_end)
      memcpy (&ipv4_addr, p, size);
      p += size;
      total += size;
    }

  if (asn4byte)
    {
      size = sizeof (peer_as_4byte);
      BUFFER_OVERRUN_CHECK(p, size, data_end)
      peer_as_4byte = ntohl (*(uint32_t *)p);
      p += size;
      total += size;
    }
  else
    {
      size = sizeof (peer_as_2byte);
      BUFFER_OVERRUN_CHECK(p, size, data_end)
      peer_as_2byte = ntohs (*(uint16_t *)p);
      p += size;
      total += size;
    }

  inet_ntop (AF_INET, &peer_bgp_id, buf, sizeof (buf));
  if (afi_ipv6)
    inet_ntop (AF_INET6, &ipv6_addr, buf2, sizeof (buf2));
  else
    inet_ntop (AF_INET, &ipv4_addr, buf2, sizeof (buf2));

  if (show && (debug || detail))
    {
      printf ("Peer[%d]: Type: %s%s%s(%#02x) "
              "BGP ID: %-15s AS: %-5d Address: %-15s\n",
              index, (peer_type & FLAG_PEER_ADDRESS_IPV6 ? "ipv6" : ""),
              ((peer_type & FLAG_PEER_ADDRESS_IPV6) &&
               (peer_type & FLAG_AS_NUMBER_SIZE) ?
               "|" : ""),
              (peer_type & FLAG_AS_NUMBER_SIZE ? "4byte-as" : ""),
              peer_type, buf,
              (int) (asn4byte ? peer_as_4byte : peer_as_2byte), buf2);
    }

  if (index < PEER_MAX)
    {
      struct peer new;
      memset (&new, 0, sizeof (new));
      new.bgp_id = peer_bgp_id;
      new.ipv6_addr = ipv6_addr;
      new.ipv4_addr = ipv4_addr;
      new.asnumber = (asn4byte ? peer_as_4byte : peer_as_2byte);

      if (verbose || peer_table_only ||
          (memcmp (&peer_table[index], &peer_null, sizeof (struct peer)) &&
           memcmp (&peer_table[index], &new, sizeof (struct peer))))
         {
           printf ("# peer_table[%d] changed: ", index);
           peer_print (&new);
           printf ("\n");
           fflush (stdout);
         }

      peer_table[index] = new;
      peer_size = index + 1;
    }
  else
    printf ("peer_table overflow.\n");

  if (autsiz)
    {
      int i;
      for (i = 0; i < autsiz; i++)
        {
          if (peer_table[index].asnumber == autnums[i] &&
              peer_spec_size < PEER_INDEX_MAX)
            {
              peer_spec_index[peer_spec_size] = index;
              peer_route_table[peer_spec_size] = route_table_create ();
              peer_route_size[peer_spec_size] = 0;
              peer_ptree[peer_spec_size] = ptree_create ();
              peer_spec_size++;
            }
        }
    }

  *retsize = total;
}

void
bgpdump_process_table_v2_peer_index_table (struct mrt_header *h,
                                           struct mrt_info *info,
                                           char *data_end)
{
  char *p;
  int size;
  int i;
  struct in_addr collector_bgp_id;
  uint16_t view_name_length;
  char *view_name;
  uint16_t peer_count;
  char buf[64];

  p = (char *)h + sizeof (struct mrt_header);

  /* Collector BGP ID */
  size = sizeof (collector_bgp_id);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  collector_bgp_id.s_addr = *(uint32_t *)p;
  p += size;

  /* View Name Length */
  size = sizeof (view_name_length);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  view_name_length = ntohs (*(uint16_t *)p);
  p += size;

  /* View Name */
  size = view_name_length;
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  view_name = p;
  p += size;

  /* Peer Count */
  size = sizeof (peer_count);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  peer_count = ntohs (*(uint16_t *)p);
  p += size;

  if (peer_table_only || (show && (debug || detail)))
    {
      inet_ntop (AF_INET, &collector_bgp_id, buf, sizeof (buf));
      printf ("Collector BGP ID: %s\n", buf);
      printf ("View Name Length: %d\n", (int) view_name_length);
      printf ("View Name: %s\n", view_name);
      printf ("Peer Count: %d\n", (int) peer_count);
    }

  for (i = 0; i < peer_count; i++)
    {
      bgpdump_table_v2_peer_entry (i, p, data_end, &size);
      p += size;
    }
}

void
bgpdump_process_bgp_attributes (struct bgp_route *route, char *start, char *end)
{
  char *p = start;
  int size;

  char attr_flags[64];
  char *attr_name;
  char unknown_buf[16];

  uint16_t attribute_type;
  uint16_t attribute_length;

  char *r;
  int i;

#define OPTIONAL         0x8000
#define TRANSITIVE       0x4000
#define PARTIAL          0x2000
#define EXTENDED_LENGTH  0x1000

#define TYPE_CODE        0x00ff
#define ORIGIN           1
#define AS_PATH          2
#define NEXT_HOP         3
#define MULTI_EXIT_DISC  4
#define LOCAL_PREF       5
#define ATOMIC_AGGREGATE 6
#define AGGREGATOR       7
#define COMMUNITY        8
#define MP_REACH_NLRI    14
#define MP_UNREACH_NLRI  15
#define EXTENDED_COMMUNITY 16

  while (p < end)
    {
      size = sizeof (attribute_type);
      BUFFER_OVERRUN_CHECK(p, size, end)
      attribute_type = ntohs (*(uint16_t *)p);
      p += size;

      snprintf (attr_flags, sizeof(attr_flags), "%s,%s,%s,%s",
        (attribute_type & OPTIONAL ?  "optional" : "well-known"),
        (attribute_type & TRANSITIVE ?  "transitive" : "non-transitive"),
        (attribute_type & PARTIAL ?  "partial" : "complete"),
        (attribute_type & EXTENDED_LENGTH ?  "len-1byte" : "len-2bytes"));

      switch (attribute_type & TYPE_CODE)
        {
        case ORIGIN:
          attr_name = "origin";
          break;
        case AS_PATH:
          attr_name = "as-path";
          break;
        case NEXT_HOP:
          attr_name = "next-hop";
          break;
        case MULTI_EXIT_DISC:
          attr_name = "multi-exit-disc";
          break;
        case LOCAL_PREF:
          attr_name = "local-pref";
          break;
        case ATOMIC_AGGREGATE:
          attr_name = "atomic-aggregate";
          break;
        case AGGREGATOR:
          attr_name = "aggregator";
          break;
        case COMMUNITY:
          attr_name = "community";
          break;
        case MP_REACH_NLRI:
          attr_name = "mp-reach-nlri";
          break;
        case MP_UNREACH_NLRI:
          attr_name = "mp-unreach-nlri";
          break;
        case EXTENDED_COMMUNITY:
          attr_name = "extended-community";
          break;
        default:
          snprintf (unknown_buf, sizeof (unknown_buf),
                    "unknown (%d)", attribute_type & TYPE_CODE);
          attr_name = unknown_buf;
          break;
        }

      if (attribute_type & EXTENDED_LENGTH)
        {
          size = 2;
          BUFFER_OVERRUN_CHECK(p, size, end)
          attribute_length = ntohs (*(uint16_t *)p);
          p += size;
        }
      else
        {
          size = 1;
          BUFFER_OVERRUN_CHECK(p, size, end)
          attribute_length = *(uint8_t *)p;
          p += size;
        }

      if (show && detail)
        printf ("  attr: %s <%s> (%#04x) len: %d\n",
                attr_name, attr_flags, attribute_type, attribute_length);

      BUFFER_OVERRUN_CHECK(p, attribute_length, end)
      switch (attribute_type & TYPE_CODE)
        {
        case AS_PATH:
          r = p;
          while (r < p + attribute_length)
            {
              uint8_t type = *(uint8_t *) r;
              r++;
              uint8_t path_size = *(uint8_t *) r;
              r++;

              if (show && detail)
                printf ("  as_path[%s:%d]:",
                        (type == 1 ? "set" : "seq"), path_size);

              route->path_size = path_size;
              for (i = 0; i < path_size; i++)
                {
                  uint32_t as_path = ntohl (*(uint32_t *) r);

                  if (show && detail)
                    printf (" %lu", (unsigned long) as_path);

                  if (i < ROUTE_PATH_LIMIT)
                    route->path_list[i] = as_path;
#if 1
                  else
                    {
                      if (show && detail)
                        printf ("\n");
                      printf ("path_list buffer overflow.\n");
                      route_print (route);
                    }
#endif

                  if (i == path_size - 1)
                    {
                      route->origin_as = as_path;
                    }

#if 0
                  if (autsiz)
                    {
                      int j;
                      unsigned long prev, curr;
                      prev = 0;
                      if (i > 0)
                        prev = route->path_list[i - 1];
                      curr = route->path_list[i];

                      if (prev > 0 && prev != curr)
                        {
                          for (j = 0; j < autsiz; j++)
                            {
                              if (prev == autnums[j])
                                printf ("%lu -- %lu\n", prev, curr);
                              else if (curr == autnums[j])
                                printf ("%lu -- %lu\n", curr, prev);
                            }
                        }
                    }
#endif

                  r += sizeof (as_path);
                }

              if (show && detail)
                printf ("\n");
            }
          break;

        case NEXT_HOP:
          memset (route->nexthop, 0, sizeof (route->nexthop));
          memcpy (route->nexthop, p, attribute_length);
          if (show && detail)
            {
              char buf[32];
              inet_ntop (safi, route->nexthop, buf, sizeof (buf));
              printf ("  nexthop: %s\n", buf);
            }
          break;

        case ORIGIN:
          if (show && detail)
            printf ("  origin: %d\n", (int) *p);
          route->origin = (uint8_t) *p;
          break;

        case ATOMIC_AGGREGATE:
          if (show && detail)
            printf ("  atomic_aggregate: len: %d\n", attribute_length);
          route->atomic_aggregate++;
          break;

        case LOCAL_PREF:
          route->localpref = ntohl (*(uint32_t *)p);
          if (show && detail)
            printf ("  local-pref: %u\n", (uint32_t) route->localpref);
          break;

        case MULTI_EXIT_DISC:
          route->med = ntohl (*(uint32_t *)p);
          if (show && detail)
            printf ("  med: %u\n", (uint32_t) route->med);
          break;

        case COMMUNITY:
          break;

        case EXTENDED_COMMUNITY:
          break;

        case MP_REACH_NLRI:
          {
            unsigned short afi;
            unsigned char safi;
            unsigned char len;
            unsigned char reserved;
            unsigned char nlri_plen;
            char nlri_prefix[16];

            r = p;
            afi = ntohs (*(unsigned short *)r);
            r += 2;
            safi = (unsigned char) *r;
            r++;
            len = (unsigned char) *r;
            r++;
            if ((debug || detail) && len != sizeof (struct in6_addr))
              printf ("warning: MP_NLRI: nexthop len: %d\n", len);
            memset (route->nexthop, 0, sizeof (route->nexthop));
            memcpy (route->nexthop, r, MIN(MAX_ADDR_LENGTH, len));
            if ((debug || verbose) && len == 2 * sizeof (struct in6_addr))
              {
                char nexthop1[16], nexthop2[16];
                char bufn1[64], bufn2[64];
                memset (nexthop1, 0, sizeof (nexthop1));
                memset (nexthop2, 0, sizeof (nexthop2));
                memcpy (nexthop1, &r[0], 16);
                memcpy (nexthop2, &r[16], 16);
                inet_ntop (AF_INET6, nexthop1, bufn1, sizeof (bufn1));
                inet_ntop (AF_INET6, nexthop2, bufn2, sizeof (bufn2));
                printf ("nexthop1: %s\n", bufn1);
                printf ("nexthop2: %s\n", bufn2);
              }
            r += len;
            reserved = (unsigned char) *r;
            r++;

            nlri_plen = (unsigned char) *r;
            r++;
            //printf ("nlri_plen: %d\n", nlri_plen);
            memset (nlri_prefix, 0, sizeof (nlri_prefix));
            memcpy (nlri_prefix, r, (nlri_plen + 7) / 8);

            if (show && detail)
              {
                char buf[64], buf2[64];
                inet_ntop (AF_INET6, route->nexthop, buf2, sizeof (buf2));
                inet_ntop (AF_INET6, nlri_prefix, buf, sizeof (buf));
                printf ("  mp_reach_nlri: (afi/safi: %d/%d) %s(size:%d) %s/%d\n",
                        afi, safi, buf2, len, buf, nlri_plen);
              }
          }
          break;

        default:
          break;
        }

      p += attribute_length;
    }

}

void
bgpdump_process_table_v2_rib_entry (int index, char **q,
                                    struct mrt_info *info,
                                    char *data_end)
{
  char *p = *q;
  int size;

  uint32_t originated_time;
  uint16_t attribute_length;

  int peer_match, i;

  size = sizeof (peer_index);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  peer_index = ntohs (*(uint16_t *)p);
  p += size;

  size = sizeof (originated_time);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  originated_time = ntohl (*(uint32_t *)p);
  p += size;

  size = sizeof (attribute_length);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  attribute_length = ntohs (*(uint16_t *)p);
  p += size;

  int peer_spec_i = 0;
  peer_match = 0;
  for (i = 0; i < MIN(peer_spec_size, PEER_INDEX_MAX); i++)
    {
      if (peer_index == peer_spec_index[i])
        {
          peer_spec_i = i;
          peer_match++;
        }
    }

  if (peer_spec_size && debug)
    printf ("peer_index: %d, peer_match: %d\n", peer_index, peer_match);

  if (! peer_spec_size || peer_match)
    {
      if (show && (debug || detail))
        printf ("rib[%d]: peer[%d] originated_time: %lu attribute_length: %d\n",
                index, peer_index, (unsigned long) originated_time,
                attribute_length);

      if (peer_index < PEER_MAX)
        {
          if (route_count)
            peer_table[peer_index].route_count++;

          if (plen_dist)
            peer_table[peer_index].route_count_by_plen[prefix_length]++;
        }

      struct bgp_route route;

      memset (&route, 0, sizeof (route));
      memcpy (route.prefix, prefix, (prefix_length + 7) / 8);
      route.prefix_length = prefix_length;

      if (brief || show || lookup || udiff || stat ||
          compat_mode || autsiz || heatmap)
        bgpdump_process_bgp_attributes (&route, p, p + attribute_length);

      /* Now all the BGP attributes for this rib_entry are processed. */

      if (stat)
        {
          int i, peer_match = 0;

          if (peer_spec_size == 0)
            peer_match++;
          else
            {
              for (i = 0; i < peer_spec_size; i++)
                if (peer_index == peer_spec_index[i])
                  peer_match++;
            }

          if (peer_match)
            {
              //printf ("peer_stat_save: peer: %d\n", peer_index);
              peer_stat_save (peer_index, &route);
            }
        }

#if 0
      if (lookup || heatmap)
        {
          struct bgp_route *rp;
          if (route_size < route_limit)
            {
              rp = &routes[route_size++];
            }
          else
            {
              rp = &routes[route_size - 1];
              printf ("route table overflow.\n");
            }

          memcpy (rp, &route, sizeof (struct bgp_route));

          ptree_add ((char *)&rp->prefix, rp->prefix_length,
                     (void *)rp, ptree[safi]);
        }
#else

      if (peer_spec_size)
        {
          struct bgp_route *rp;
          int *route_size = &peer_route_size[peer_spec_i];
          if (*route_size >= nroutes)
            {
              printf ("route table overflow.\n");
              *route_size = nroutes - 1;
            }

          struct bgp_route *rpp;
          rpp = peer_route_table[peer_spec_i];
          rp = &rpp[sequence_number];
          //rp = &peer_route_table[peer_index][sequence_number];
          *route_size = *route_size + 1;
          //(*route_size)++;

          //route_print (&route);
          memcpy (rp, &route, sizeof (struct bgp_route));

          if (safi == AF_INET)
            ptree_add ((char *)&rp->prefix, rp->prefix_length,
                       (void *)rp, peer_ptree[peer_spec_i]);
        }
#endif

      if (udiff)
        {
          for (i = 0; i < MIN (peer_spec_size, 2); i++)
            {
              if (peer_spec_index[i] == peer_index)
                {
                  diff_table[i][sequence_number] = route;
                  if (udiff_lookup)
                    ptree_add ((char *)&route.prefix, route.prefix_length,
                               (void *)&diff_table[i][sequence_number],
                               diff_ptree[i]);
                }
            }
        }

      if (brief)
        route_print_brief (&route);
      else if (show)
        route_print (&route);
      else if (compat_mode)
        route_print_compat (&route);
    }

  BUFFER_OVERRUN_CHECK(p, attribute_length, data_end)
  p += attribute_length;

  *q = p;
}

void
bgpdump_process_table_v2_rib_unicast (struct mrt_header *h,
                                      struct mrt_info *info,
                                      char *data_end)
{
  char *p;
  int size;
  int i;

  uint32_t prefix_size;
  uint16_t entry_count;

  p = (char *)h + sizeof (struct mrt_header);

  size = sizeof (sequence_number);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  sequence_number = ntohl (*(uint32_t *)p);
  p += size;

  size = sizeof (prefix_length);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  prefix_length = *(uint8_t *)p;
  p += size;

  prefix_size = ((prefix_length + 7) / 8);
  size = prefix_size;
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  memset (prefix, 0, sizeof (prefix));
  memcpy (prefix, p, prefix_size);
  p += size;

  size = sizeof (entry_count);
  BUFFER_OVERRUN_CHECK(p, size, data_end)
  entry_count = ntohs (*(uint16_t *)p);
  p += size;

  if (show && debug)
    {
      char pbuf[64];
      inet_ntop (safi, prefix, pbuf, sizeof (pbuf));
      printf ("Sequence Number: %lu Prefix %s/%d Entry Count: %d\n",
              (unsigned long) sequence_number,
              pbuf, prefix_length, entry_count);
    }

  for (i = 0; i < entry_count && p < data_end; i++)
    {
      bgpdump_process_table_v2_rib_entry (i, &p, info, data_end);
    }

  if (udiff)
    {
      bgpdump_udiff_compare (sequence_number);

#if 0
      struct bgp_route *route;

      if (udiff_verbose)
        {
          printf ("seq: %lu\n", (unsigned long) sequence_number);
          if (! IS_ROUTE_NULL (&diff_table[0][sequence_number]))
            {
              route = &diff_table[0][sequence_number];
              printf ("{");
              route_print (route);
            }
          if (! IS_ROUTE_NULL (&diff_table[1][sequence_number]))
            {
              route = &diff_table[1][sequence_number];
              printf ("}");
              route_print (route);
            }
        }

      /* only in left */
      if (! IS_ROUTE_NULL (&diff_table[0][sequence_number]) &&
          IS_ROUTE_NULL (&diff_table[1][sequence_number]))
        {
          route = &diff_table[0][sequence_number];
          if (! udiff_lookup)
            {
              printf ("-");
              route_print (route);
            }
          else
            {
              struct ptree_node *x;
              int plen = (qaf == AF_INET ? 32 : 128);
              x = ptree_search ((char *)&route->prefix, plen, diff_ptree[1]);
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
      if (IS_ROUTE_NULL (&diff_table[0][sequence_number]) &&
          ! IS_ROUTE_NULL (&diff_table[1][sequence_number]))
        {
          route = &diff_table[1][sequence_number];
          if (! udiff_lookup)
            {
              printf ("+");
              route_print (route);
            }
          else
            {
              struct ptree_node *x;
              int plen = (qaf == AF_INET ? 32 : 128);
              x = ptree_search ((char *)&route->prefix, plen, diff_ptree[0]);
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
      if (! IS_ROUTE_NULL (&diff_table[0][sequence_number]) &&
          ! IS_ROUTE_NULL (&diff_table[1][sequence_number]) &&
          diff_table[0][sequence_number].prefix_length > 0)
        {
          int plen = diff_table[0][sequence_number].prefix_length - 1;

          if (udiff_lookup)
            {
              struct ptree_node *x;

              route = &diff_table[0][sequence_number];
              x = ptree_search ((char *)&route->prefix, plen, diff_ptree[1]);
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

              route = &diff_table[1][sequence_number];
              x = ptree_search ((char *)&route->prefix, plen, diff_ptree[0]);
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
#endif /*0*/

    }
}

void
bgpdump_process_table_dump_v2 (struct mrt_header *h, struct mrt_info *info,
                               char *data_end)
{
  switch (info->subtype)
    {
    case BGPDUMP_TABLE_V2_PEER_INDEX_TABLE:
      bgpdump_process_table_v2_peer_index_table (h, info, data_end);
      break;
    case BGPDUMP_TABLE_V2_RIB_IPV4_UNICAST:
      if (! peer_table_only)
        {
          safi = AF_INET;
          bgpdump_process_table_v2_rib_unicast (h, info, data_end);
        }
      break;
    case BGPDUMP_TABLE_V2_RIB_IPV6_UNICAST:
      if (! peer_table_only)
        {
          safi = AF_INET6;
          bgpdump_process_table_v2_rib_unicast (h, info, data_end);
        }
      break;
    default:
      printf ("unsupported subtype: %d\n", info->subtype);
      break;
    }
}

