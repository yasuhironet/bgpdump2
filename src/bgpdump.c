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
#include <sys/time.h>
#include <getopt.h>
#include <assert.h>
#include <errno.h>

#include "benchmark.h"
#include "queue.h"
#include "ptree.h"

#include "bgpdump.h"
#include "bgpdump_parse.h"
#include "bgpdump_option.h"
#include "bgpdump_file.h"
#include "bgpdump_data.h"
#include "bgpdump_route.h"

#include "bgpdump_savefile.h"
#include "bgpdump_query.h"
#include "bgpdump_ptree.h"
#include "bgpdump_peer.h"
#include "bgpdump_peerstat.h"

extern int optind;

char *progname = NULL;

struct mrt_info info;
struct ptree *ptree[AF_INET6 + 1];

int safi = AF_INET;
int qaf = AF_INET;

unsigned long autnums[AUTLIM];
int autsiz = 0;

struct bgp_route *diff_table[2];
struct ptree *diff_ptree[2];

void
bgpdump_process (char *buf, size_t *data_len)
{
  char *p;
  struct mrt_header *h;
  int hsize = sizeof (struct mrt_header);
  char *data_end = buf + *data_len;
  unsigned long len;
  int rest;

  if (debug)
    printf ("process %lu bytes.\n", *data_len);

  p = buf;
  h = (struct mrt_header *) p;
  len = ntohl (h->length);

  if (debug)
    printf ("mrt message: length: %lu bytes.\n", len);

  /* Process as long as entire MRT message is in the buffer */
  while (len && p + hsize + len <= data_end)
    {
      bgpdump_process_mrt_header (h, &info);

      switch (mrt_type)
        {
        case BGPDUMP_TYPE_TABLE_DUMP_V2:
          bgpdump_process_table_dump_v2 (h, &info, p + hsize + len);
          break;
        default:
          printf ("Not supported: mrt type: %d\n", mrt_type);
          printf ("discarding %lu bytes data.\n", hsize + len);
          break;
        }

      p += hsize + len;

      len = 0;
      if (p + hsize < data_end)
        {
          h = (struct mrt_header *) p;
          len = ntohl (h->length);
          if (debug)
            {
              printf ("next mrt message: length: %lu bytes.\n", len);
              printf ("p: %p hsize: %d len: %lu mrt-end: %p data_end: %p\n",
                      p, hsize, len, p + hsize + len, data_end);
            }
        }
    }

  /* move the partial, last-part data
     to the beginning of the buffer. */
  rest = data_end - p;
  if (rest)
    memmove (buf, p, rest);
  *data_len = rest;
}

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
heatmap_image_hilbert_gplot (int peer_spec_i)
{
  int peer_index = peer_spec_index[peer_spec_i];

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

  fprintf (fp, "\n");
  fprintf (fp, "set title \"%s p%d bgpid:%s addr:%s AS%d\"\n",
           heatmap_prefix, peer_index, bgpid, bgpaddr, asnum);
  fprintf (fp, "set term postscript eps enhanced color\n");
  fprintf (fp, "set output '%s-p%d.eps'\n", heatmap_prefix, peer_index);
  fprintf (fp, "splot '%s-p%d.dat' u 1:2:3 with image notitle\n",
           heatmap_prefix, peer_index);
  fprintf (fp, "\n");
  fprintf (fp, "set term png\n");
  fprintf (fp, "set output '%s-p%d.png'\n", heatmap_prefix, peer_index);
  fprintf (fp, "replot\n");
  fprintf (fp, "\n");

  fclose (fp);

  printf ("%s is written.\n", filename);
}


void
heatmap_image_hilbert_data (int peer_spec_i)
{
  int peer_index = peer_spec_index[peer_spec_i];
  struct ptree *ptree = peer_ptree[peer_spec_i];

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

int
main (int argc, char **argv)
{
  int status = 0;
  int i;
  char *filename = NULL;

  progname = argv[0];

  bufsiz = resolv_number (BGPDUMP_BUFSIZ_DEFAULT, NULL);
  nroutes = resolv_number (ROUTE_LIMIT_DEFAULT, NULL);

  status = bgpdump_getopt (argc, argv);

  argc -= optind;
  argv += optind;

  if (status)
    return status;

  if (argc == 0)
    {
      printf ("specify rib files.\n");
      usage ();
      exit (-1);
    }

  if (verbose)
    {
      printf ("bufsiz = %llu\n", bufsiz);
      printf ("nroutes = %llu\n", nroutes);
    }

  /* default cmd */
  if (! brief && ! show && ! route_count && ! plen_dist && ! udiff &&
      ! lookup && ! peer_table_only && ! stat && ! compat_mode &&
      ! autsiz && ! heatmap)
    show++;

  if (stat)
    peer_stat_init ();

  char *buf;
  buf = malloc (bufsiz);
  if (! buf)
    {
      printf ("can't malloc %lluB-size buf: %s\n",
              bufsiz, strerror (errno));
      exit (-1);
    }

  peer_table_init ();

  if (peer_spec_size)
    {
      for (i = 0; i < peer_spec_size; i++)
        {
          peer_route_table[i] = route_table_create ();
          peer_route_size[i] = 0;
          peer_ptree[i] = ptree_create ();
        }
    }

  if (lookup)
    {
      route_init ();
      ptree[AF_INET] = ptree_create ();
      ptree[AF_INET6] = ptree_create ();
    }

  if (udiff)
    {
      diff_table[0] = malloc (nroutes * sizeof (struct bgp_route));
      diff_table[1] = malloc (nroutes * sizeof (struct bgp_route));
      assert (diff_table[0] && diff_table[1]);
      memset (diff_table[0], 0, nroutes * sizeof (struct bgp_route));
      memset (diff_table[1], 0, nroutes * sizeof (struct bgp_route));

      if (udiff_lookup)
        {
          diff_ptree[0] = ptree_create ();
          diff_ptree[1] = ptree_create ();
        }
    }

  /* for each rib files. */
  for (i = 0; i < argc; i++)
    {
      filename = argv[i];

      file_format_t format;
      struct access_method *method;
      void *file;
      size_t ret;

      format = get_file_format (filename);
      method = get_access_method (format);
      file = method->fopen (filename, "r");
      if (! file)
        {
          fprintf (stderr, "# could not open file: %s\n", filename);
          continue;
        }

      size_t datalen = 0;

      while (1)
        {
          ret = method->fread (buf + datalen, bufsiz - datalen, 1, file);
          if (debug)
            printf ("read: %lu bytes to buf[%lu]. total %lu bytes\n",
                    ret, datalen, ret + datalen);
          datalen += ret;

          /* end of file. */
          if (ret == 0 && method->feof (file))
            {
              if (debug)
                printf ("read: end-of-file.\n");
              break;
            }

          bgpdump_process (buf, &datalen);

          if (debug)
            printf ("process rest: %lu bytes\n", datalen);
        }

      if (datalen)
        {
          printf ("warning: %lu bytes unprocessed data remains: %s\n",
                  datalen, filename);
        }
      method->fclose (file);

      /* For each end of the processing of files. */
      if (route_count)
        {
          peer_route_count_show ();
          peer_route_count_clear ();
        }

      if (plen_dist)
        {
          peer_route_count_by_plen_show ();
          peer_route_count_by_plen_clear ();
        }
    }

  /* query_table construction. */
  if (lookup)
    {
      query_limit = 0;

      if (lookup_file)
        query_limit = query_file_count (lookup_file);

      if (lookup_addr)
        query_limit++;

      query_init ();

      if (lookup_addr)
        query_addr (lookup_addr);

      if (lookup_file)
        query_file (lookup_file);

      if (debug)
        query_list ();
    }

  /* query to route_table (ptree). */
  if (lookup)
    {
      if (verbose)
        ptree_list (ptree[qaf]);

      if (benchmark)
        benchmark_start ();

      if (lookup)
        ptree_query (ptree[qaf], query_table, query_size);

      if (benchmark)
        {
          benchmark_stop ();
          benchmark_print (query_size);
        }
    }

  if (heatmap)
    {
      for (i = 0; i < peer_spec_size; i++)
        {
          heatmap_image_hilbert_gplot (i);
          heatmap_image_hilbert_data (i);
        }
    }

  if (lookup)
    {
      free (query_table);
      ptree_delete (ptree[AF_INET]);
      ptree_delete (ptree[AF_INET6]);
      route_finish ();
    }

  if (udiff)
    {
      free (diff_table[0]);
      free (diff_table[1]);

      if (lookup)
        {
          ptree_delete (diff_ptree[0]);
          ptree_delete (diff_ptree[1]);
        }
    }

  if (stat)
    {
      peer_stat_show ();
      //peer_stat_finish ();
    }

  free (buf);

  return status;
}


