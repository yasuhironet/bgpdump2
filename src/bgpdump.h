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

#ifndef _BGPDUMP_H_
#define _BGPDUMP_H_

#ifndef MIN
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif /*MIN*/

extern int debug;
extern int detail;
extern int verbose;
extern int show;
extern int brief;
extern int compat_mode;
extern int udiff;
extern int route_count;
extern int stat;
extern int benchmark;
extern int lookup;
extern char *lookup_addr;
extern char *lookup_file;
extern int peer_table_only;
extern unsigned long autnums[];
extern int autsiz;

extern struct ptree *ptree[];
#define ROUTE_ORIG_SIZE (1000 * 1000 * 1000)
extern int route_orig_size;

extern int safi;
extern int qaf;
#define MAX_ADDR_LENGTH 16

#endif /*_BGPDUMP_H_*/

