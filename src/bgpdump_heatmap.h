/*
 * Bgpdump2: A Tool to Read and Compare the BGP RIB Dump Files.
 * Copyright (C) 2021.  Yasuhiro Ohara <yasu@nttv6.jp>
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

#ifndef _BGPDUMP_HEATMAP_H_
#define _BGPDUMP_HEATMAP_H_

void rot (u_int64_t n, u_int32_t *x, u_int32_t *y, u_int32_t rx, u_int32_t ry);
void d2xy (u_int64_t n, u_int64_t d, u_int32_t *x, u_int32_t *y);

void heatmap_image_hilbert_gplot (int peer_spec_i);
void heatmap_image_hilbert_data (int peer_spec_i);
void heatmap_image_hilbert_data_aspath_max_distance (int peer_spec_i);

#endif /*_BGPDUMP_HEATMAP_H_*/

