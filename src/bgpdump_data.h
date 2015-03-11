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

#ifndef _BGPDUMP_DATA_H_
#define _BGPDUMP_DATA_H_

#define BGPDUMP_TYPE_DEPRECATED_MRT_NULL       0
#define BGPDUMP_TYPE_DEPRECATED_MRT_START      1
#define BGPDUMP_TYPE_DEPRECATED_MRT_DIE        2
#define BGPDUMP_TYPE_DEPRECATED_MRT_I_AM_DEAD  3
#define BGPDUMP_TYPE_DEPRECATED_MRT_PEER_DOWN  4
#define BGPDUMP_TYPE_DEPRECATED_BGP            5
#define BGPDUMP_TYPE_DEPRECATED_RIP            6
#define BGPDUMP_TYPE_DEPRECATED_IDRP           7
#define BGPDUMP_TYPE_DEPRECATED_RIPNG          8
#define BGPDUMP_TYPE_DEPRECATED_BGP4PLUS       9
#define BGPDUMP_TYPE_DEPRECATED_BGP4PLUS_01    10

#define BGPDUMP_TYPE_OSPFV2                    11
#define BGPDUMP_TYPE_TABLE_DUMP                12
#define BGPDUMP_TYPE_TABLE_DUMP_V2             13
#define BGPDUMP_TYPE_BGP4MP                    16
#define BGPDUMP_TYPE_BGP4MP_ET                 17
#define BGPDUMP_TYPE_ISIS                      32
#define BGPDUMP_TYPE_ISIS_ET                   33
#define BGPDUMP_TYPE_OSPFV3                    48
#define BGPDUMP_TYPE_OSPFV3_ET                 49

#define BGPDUMP_TABLE_V2_PEER_INDEX_TABLE      1
#define BGPDUMP_TABLE_V2_RIB_IPV4_UNICAST      2
#define BGPDUMP_TABLE_V2_RIB_IPV4_MULTICAST    3
#define BGPDUMP_TABLE_V2_RIB_IPV6_UNICAST      4
#define BGPDUMP_TABLE_V2_RIB_IPV6_MULTICAST    5
#define BGPDUMP_TABLE_V2_RIB_GENERIC           6

struct mrt_header
{
  uint32_t timestamp;
  uint16_t type;
  uint16_t subtype;
  uint32_t length;
};

struct mrt_info
{
  uint32_t timestamp;
  uint16_t type;
  uint16_t subtype;
  uint32_t length;
};

extern uint32_t timestamp;
extern uint16_t mrt_type;
extern uint16_t mrt_subtype;
extern uint32_t mrt_length;

void
bgpdump_process_mrt_header (struct mrt_header *h, struct mrt_info *info);

void
bgpdump_process_table_dump_v2 (struct mrt_header *h, struct mrt_info *info,
                               char *data_end);

#endif /*_BGPDUMP_DATA_H_*/

