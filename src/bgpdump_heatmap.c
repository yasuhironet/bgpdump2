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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "ptree.h"

#include "bgpdump_option.h"
#include "bgpdump_peer.h"
#include "bgpdump_route.h"

void
rot (u_int64_t n, u_int32_t *x, u_int32_t *y, u_int32_t rx, u_int32_t ry)
{
  int t;
  if (ry == 0)
    {
      if (rx == 1)
        {
          *x = n-1 - *x;
          *y = n-1 - *y;
        }

      t = *x;
      *x = *y;
      *y = t;
    }
}

void
d2xy (u_int64_t n, u_int64_t d, u_int32_t *x, u_int32_t *y)
{
  u_int64_t s, t = d;
  u_int32_t rx, ry;
  *x = *y = 0;
  for (s = 1; s < n; s *= 2)
    {
      rx = 1 & (t / 2);
      ry = 1 & (t ^ rx);

      rot (s, x, y, rx, ry);

      *x += s * rx;
      *y += s * ry;
      t /= 4;
    }
}

void
heatmap_image_hilbert_gplot (int peer_index)
{
  //int peer_index = peer_spec_index[peer_spec_i];

  unsigned int a0;
  unsigned long val = 0;

  u_int32_t x1, y1, x2, y2;
  u_int32_t xs, ys, xe, ye;

  unsigned int index;

  int textxmargin = 1;
  int textymargin = 5;

  FILE *fp;
  char filename[256];
  snprintf (filename, sizeof (filename), "%s-p%d.gp",
            heatmap_prefix, peer_index);
  fp = fopen (filename, "w+");
  if (! fp)
    {
      fprintf (stderr, "can't open file %s: %s\n",
               filename, strerror (errno));
      return;
    }

  fprintf (fp, "set xlabel \"\"\n");
  fprintf (fp, "set ylabel \"\"\n");
  fprintf (fp, "\n");
  fprintf (fp, "unset tics\n");
  fprintf (fp, "\n");
  fprintf (fp, "set cbrange [0:256]\n");
  fprintf (fp, "set cbtics 32\n");
  fprintf (fp, "\n");
  fprintf (fp, "set xrange [-1:256]\n");
  fprintf (fp, "set yrange [256:-1]\n");
  fprintf (fp, "\n");
  fprintf (fp, "set pm3d map\n");
  fprintf (fp, "set palette defined (0 \"black\", 1 \"red\", "
               "128 \"yellow\", 192 \"green\", 255 \"blue\")\n");
  fprintf (fp, "\n");
  fprintf (fp, "set style rect back fs empty border lc rgb \"white\"\n");
  fprintf (fp, "\n");

  for (a0 = 0; a0 < 256; a0++)
    {
      val = (a0 << 8);
      d2xy (1ULL << 16, val, &x1, &y1);
      val = (a0 << 8) + 255;
      d2xy (1ULL << 16, val, &x2, &y2);

      index = a0 + 1;
      //printf ("%u: start (%u,%u) end (%u,%u).\n",
      //        a0, x1, y1, x2, y2);

#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif
#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif
      xs = (MIN (x1, x2)) / 16 * 16;
      ys = (MIN (y1, y2)) / 16 * 16;
      xe = xs + 16;
      ye = ys + 16;

      fprintf (fp, "set label  %u \"%u\" at first %u,%u left "
              "font \",8\" front textcolor rgb \"white\"\n",
              index, a0, xs + textxmargin, ys + textymargin);
      fprintf (fp, "set object %u rect from %u,%u to %u,%u front\n",
              index, xs, ys, xe, ye);
    }

  int asnum;
  char bgpid[32], bgpaddr[32];
  asnum = peer_table[peer_index].asnumber;
  inet_ntop (AF_INET, &peer_table[peer_index].bgp_id,
             bgpid, sizeof (bgpid));
  inet_ntop (AF_INET, &peer_table[peer_index].ipv4_addr,
             bgpaddr, sizeof (bgpaddr));

  char *p, titlename[64];
  p = rindex (heatmap_prefix, '/');
  if (p)
    snprintf (titlename, sizeof (titlename), "%s", ++p);
  else
    snprintf (titlename, sizeof (titlename), "%s", heatmap_prefix);

  fprintf (fp, "\n");
  fprintf (fp, "set title \"%s p%d bgpid:%s addr:%s AS%d\"\n",
           titlename, peer_index, bgpid, bgpaddr, asnum);
  fprintf (fp, "set term png\n");
  fprintf (fp, "set output '%s-p%d.png'\n", heatmap_prefix, peer_index);
  fprintf (fp, "splot '%s-p%d.dat' u 1:2:3 with image notitle\n",
           heatmap_prefix, peer_index);
  fprintf (fp, "\n");
#if 0
  fprintf (fp, "set term postscript eps enhanced color\n");
  fprintf (fp, "set output '%s-p%d.eps'\n", heatmap_prefix, peer_index);
  fprintf (fp, "replot\n");
  fprintf (fp, "\n");
#endif

  fclose (fp);

  printf ("%s is written.\n", filename);
}

void
heatmap_image_hilbert_data (int peer_index, struct ptree *ptree)
{
  //int peer_index = peer_spec_index[peer_spec_i];
  //struct ptree *ptree = peer_ptree[peer_spec_i];

  unsigned int a0, a1, a2;
  struct in_addr addr = { 0 };
  unsigned long val = 0;
  unsigned char *p = (unsigned char *) &addr;
  struct ptree_node *node;
  unsigned long count = 0;

  unsigned int array[256][256];

  u_int32_t x, y;
  x = y = 0;

  for (a0 = 0; a0 < 256; a0++)
    {
      p[0] = (unsigned char) a0;
      for (a1 = 0; a1 < 256; a1++)
        {
          p[1] = (unsigned char) a1;

          count = 0;
          for (a2 = 0; a2 < 256; a2++)
            {
              p[2] = (unsigned char) a2;
              //printf ("heat: addr: %s\n", inet_ntoa (addr));

              node = ptree_search ((char *)&addr, 24, ptree);
              if (node)
                {
                  //struct bgp_route *route = node->data;
                  //route_print (route);
                  count++;
                }
              else
                {
                  //printf ("no route.\n");
                }
            }

          p[2] = 0;
          val = (a0 << 8) + a1;
          d2xy (1ULL << 16, val, &x, &y);
#if 0
          printf ("val: %lu, x: %lu, y: %lu, count: %lu\n",
                  val, (unsigned long) x, (unsigned long) y,
                  (unsigned long) count);
#endif

          array[x][y] = count;
        }
    }

  //printf ("\n");

  FILE *fp;
  char filename[256];
  snprintf (filename, sizeof (filename), "%s-p%d.dat",
            heatmap_prefix, peer_index);

  fp = fopen (filename, "w+");
  if (! fp)
    {
      fprintf (stderr, "can't open file %s: %s\n",
               filename, strerror (errno));
      return;
    }

  for (a0 = 0; a0 < 256; a0++)
    for (a1 = 0; a1 < 256; a1++)
      fprintf (fp, "%u %u %u\n", a0, a1, array[a0][a1]);

  fclose (fp);
  printf ("%s is written.\n", filename);
}

void
heatmap_image_hilbert_data_aspath_max_distance (int peer_index, struct ptree *ptree)
{
  //int peer_index = peer_spec_index[peer_spec_i];
  //struct ptree *ptree = peer_ptree[peer_spec_i];

  unsigned int a0, a1, a2;
  struct in_addr addr = { 0 };
  unsigned long val = 0;
  unsigned char *p = (unsigned char *) &addr;
  struct ptree_node *node;
  //unsigned long count = 0;
  unsigned long max = 0;

  unsigned int array[256][256];

  u_int32_t x, y;
  x = y = 0;

  for (a0 = 0; a0 < 256; a0++)
    {
      p[0] = (unsigned char) a0;
      for (a1 = 0; a1 < 256; a1++)
        {
          p[1] = (unsigned char) a1;

          //count = 0;
          max = 0;

          for (a2 = 0; a2 < 256; a2++)
            {
              p[2] = (unsigned char) a2;
              //printf ("heat: addr: %s\n", inet_ntoa (addr));

              node = ptree_search ((char *)&addr, 24, ptree);
              if (node)
                {
                  struct bgp_route *route = node->data;
                  //route_print (route);
                  //count++;
                  if (max < route->path_size)
                    max = route->path_size;
                }
              else
                {
                  //printf ("no route.\n");
                }
            }

          p[2] = 0;
          val = (a0 << 8) + a1;
          d2xy (1ULL << 16, val, &x, &y);
#if 0
          printf ("val: %lu, x: %lu, y: %lu, count: %lu\n",
                  val, (unsigned long) x, (unsigned long) y,
                  (unsigned long) count);
#endif
#if 1
          printf ("val: %lu, x: %lu, y: %lu, max: %lu\n",
                  val, (unsigned long) x, (unsigned long) y,
                  (unsigned long) max);
#endif

          //array[x][y] = count;
          int adjust = 16;
          if (max * adjust > 255)
            array[x][y] = 255;
          else
            array[x][y] = max * adjust;
        }
    }

  //printf ("\n");

  FILE *fp;
  char filename[256];
  snprintf (filename, sizeof (filename), "%s-p%d.dat",
            heatmap_prefix, peer_index);

  fp = fopen (filename, "w+");
  if (! fp)
    {
      fprintf (stderr, "can't open file %s: %s\n",
               filename, strerror (errno));
      return;
    }

  for (a0 = 0; a0 < 256; a0++)
    for (a1 = 0; a1 < 256; a1++)
      fprintf (fp, "%u %u %u\n", a0, a1, array[a0][a1]);

  fclose (fp);
  printf ("%s is written.\n", filename);
}


