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
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <locale.h>

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
#include "bgpdump_heatmap.h"

extern int optind;

char *progname = NULL;

struct mrt_info info;
struct ptree *ptree[AF_INET6 + 1];

int qafi = 0;

unsigned long autnums[AUTLIM];
int autsiz = 0;

struct bgp_route *diff_table[2];
struct ptree *diff_ptree[2];

int
bgpdump_process (char *buf, size_t *data_len)
{
  char *p;
  struct mrt_header *h;
  int hsize = sizeof (struct mrt_header);
  char *data_end = buf + *data_len;
  unsigned long len;
  int rest;

  if (debug)
    printf ("%s(): process %'lu bytes.\n", __func__, *data_len);

  p = buf;
  h = (struct mrt_header *) p;
  len = ntohl (h->length);

  if (debug)
    printf ("%s(): mrt message: length: %'lu bytes.\n", __func__, len);

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
          printf ("discarding %'lu bytes data.\n", hsize + len);
          break;
        }

      p += hsize + len;

      len = 0;
      if (p + hsize < data_end)
        {
          h = (struct mrt_header *) p;
          len = ntohl (h->length);
          if (debug >= 3)
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

  if (debug)
    printf ("%s(): processed: %'lu bytes, %'lu bytes remains.\n",
            __func__, (unsigned long) (p - buf), (unsigned long) rest);

  return (p - buf);
}

int
main (int argc, char **argv)
{
  int status = 0;
  int i;
  char *filename = NULL;

  setlocale (LC_ALL, "");

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
      printf ("bufsiz = %'llu\n", bufsiz);
      printf ("nroutes = %'llu\n", nroutes);
    }

  /* default cmd */
  if (! brief && ! show && ! route_count && ! route_count_peers &&
      ! plen_dist && ! udiff &&
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
      if (peer_spec_size >= PEER_INDEX_MAX)
        peer_spec_size = PEER_INDEX_MAX;
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
            printf ("read: %'lu bytes to buf[%lu]. total %'lu bytes\n",
                    ret, datalen, ret + datalen);
          datalen += ret;

          /* end of file. */
          if (ret == 0 && method->feof (file))
            {
              if (debug)
                printf ("read: end-of-file.\n");
              break;
            }

          ret = bgpdump_process (buf, &datalen);
          if (ret <= 0)
            {
              printf ("bgpdump_process(): failed: ret: %ld.\n", ret);
              break;
            }

          if (debug)
            printf ("process rest: %'lu bytes\n", datalen);
        }

      if (datalen)
        {
          printf ("warning: %'lu bytes unprocessed data remains: %s\n",
                  datalen, filename);
        }
      method->fclose (file);

      /* For each end of the processing of files. */
      if (route_count)
        {
          peer_route_count_show ();
          peer_route_count_clear ();
        }
      if (route_count_peers)
        {
          peer_route_count_list ();
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
      if (! qafi && lookup_addr)
        {
          struct in6_addr tmp;
          if (inet_pton (AF_INET6, lookup_addr, &tmp) == 1)
            qafi = AF_INET6;
          else
            qafi = AF_INET;
        }

      if (! qafi)
        qafi = AF_INET;

      printf ("lookup: query afi: %d\n", qafi);

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
      if (benchmark)
        benchmark_start ();

      if (lookup)
        {
          if (! peer_spec_size)
            printf ("warning: no peer spec. lookup needs a specified peer.\n");
          for (i = 0; i < peer_spec_size; i++)
            {
              printf ("peer %d:\n", peer_spec_index[i]);
              if (verbose)
                ptree_list (peer_ptree[i]);
              ptree_query (peer_ptree[i], query_table, query_size);
            }
        }

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
          //heatmap_image_hilbert_data_aspath_max_distance (i);
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


