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

#ifndef _BGPDUMP_PEER_H_
#define _BGPDUMP_PEER_H_

struct peer
{
  struct in_addr bgp_id;
  struct in_addr ipv4_addr;
  struct in6_addr ipv6_addr;
  uint32_t asnumber;
  uint64_t route_count;
  uint64_t route_count_by_plen[33];
  uint64_t route_count_ipv4;
  uint64_t route_count_ipv6;
};

#define PEER_MAX 256

#define PEER_INDEX_MAX 8

extern struct peer peer_null;
extern struct peer peer_table[];
extern int peer_size;

extern int peer_spec_index[];
extern int peer_spec_size;

extern struct bgp_route *peer_route_table[];
extern int peer_route_size[];
extern struct ptree *peer_ptree[];

void peer_table_init ();
void peer_print (struct peer *peer);
void peer_route_count_show ();
void peer_route_count_clear ();
void peer_route_count_list ();
void peer_route_count_by_plen_show ();
void peer_route_count_by_plen_clear ();

#endif /*_BGPDUMP_PEER_H_*/

