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

extern int debug;
extern int detail;
extern int verbose;
extern int show;
extern int lookup;
extern char *lookup_addr;
extern char *lookup_file;
extern int udiff;
extern int multi_origin;
extern int tschange;
extern int route_count;
extern int plen_dist;
extern int benchmark;
extern unsigned long long ntimes;
extern unsigned long long nroutes;
extern char *input;
extern char *output;
extern unsigned long seed;
extern int peer_table_only;
extern int route_table;
extern int mbt_table;
extern int integrity;
extern int exhaustive;

