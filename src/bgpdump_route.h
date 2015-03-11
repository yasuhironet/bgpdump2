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

#ifndef _BGPDUMP_ROUTE_H_
#define _BGPDUMP_ROUTE_H_

#define ROUTE_LIMIT_DEFAULT "1000K"
#define ROUTE_PATH_LIMIT 64

#include "bgpdump.h"

struct bgp_route
{
  char prefix[MAX_ADDR_LENGTH];
  uint8_t prefix_length;
  char nexthop[MAX_ADDR_LENGTH];
  uint8_t path_size;
  uint32_t origin_as;
  uint32_t path_list[ROUTE_PATH_LIMIT];
  uint8_t origin;
  uint8_t atomic_aggregate;
  uint32_t localpref;
  uint32_t med;
  uint32_t community;
};

extern struct bgp_route *routes;
extern int route_limit;
extern int route_size;

extern char addr_none[];

#define IS_ROUTE_NULL(route) \
  ((route)->prefix_length == 0 && \
   ! memcmp ((route)->prefix, addr_none, MAX_ADDR_LENGTH) && \
   ! memcmp ((route)->nexthop, addr_none, MAX_ADDR_LENGTH))

void route_init ();
void route_finish ();

void route_print_brief (struct bgp_route *route);
void route_print (struct bgp_route *route);
void route_print_compat (struct bgp_route *route);

#endif /*_BGPDUMP_ROUTE_H_*/

