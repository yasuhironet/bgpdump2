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

#ifndef _BGPDUMP_PEER_STAT_H_
#define _BGPDUMP_PEER_STAT_H_

struct peer_stat
{
  uint64_t route_count;
  //uint64_t route_count_by_plen[33];
  uint64_t route_count_by_plen[129];
  struct ptree *nexthop_count;
  struct ptree *origin_as_count;
  struct ptree *as_path_count;
  struct ptree *as_path_len_count;
};

extern struct peer_stat peer_stat[];

void peer_stat_init ();
void peer_stat_finish ();
void peer_stat_save (int peer_index, struct bgp_route *route);
void peer_stat_show ();

#endif /*_BGPDUMP_PEER_STAT_H_*/

