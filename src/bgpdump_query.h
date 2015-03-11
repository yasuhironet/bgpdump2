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

#include "bgpdump.h"

struct query
{
  char destination[MAX_ADDR_LENGTH];
  int plen;
  char nexthop[MAX_ADDR_LENGTH];
};

extern struct query *query_table;
extern uint64_t query_limit;
extern uint64_t query_size;

void query_init ();
void query_addr (char *lookup_addr);
unsigned long query_file_count (char *lookup_file);
void query_file (char *lookup_file);
void query_random (int ntimes);
void query_list ();

