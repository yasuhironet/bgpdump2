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
#include "bgpdump_file.h"
#include "bgpdump_data.h"
#include "bgpdump_peer.h"
#include "bgpdump_route.h"

#include "bgpdump_savefile.h"
#include "bgpdump_query.h"
#include "bgpdump_ptree.h"
#include "bgpdump_peerstat.h"

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

char *progname = NULL;

#define BGPDUMP_VERSION "v2.0.1"

#define BGPDUMP_BUFSIZ_DEFAULT "16MiB"

struct mrt_info info;
struct ptree *ptree[AF_INET6 + 1];

const char *optstring = "hVvdmbPp:a:uckN:M:gl:L:46";
const struct option longopts[] =
{
  { "help",         no_argument,       NULL, 'h' },
  { "version",      no_argument,       NULL, 'V' },
  { "verbose",      no_argument,       NULL, 'v' },
  { "debug",        no_argument,       NULL, 'd' },
  { "compat-mode",  no_argument,       NULL, 'm' },
  { "brief",        no_argument,       NULL, 'b' },
  { "peer-table",   no_argument,       NULL, 'P' },
  { "peer",         required_argument, NULL, 'p' },
  { "autnum",       required_argument, NULL, 'a' },
  { "diff",         no_argument,       NULL, 'u' },
  { "count",        no_argument,       NULL, 'c' },
  { "peer-stat",    no_argument,       NULL, 'k' },
  { "bufsiz",       required_argument, NULL, 'N' },
  { "nroutes",      required_argument, NULL, 'M' },
  { "benchmark",    no_argument,       NULL, 'g' },
  { "lookup",       required_argument, NULL, 'l' },
  { "lookup-file",  required_argument, NULL, 'L' },
  { "ipv4",         no_argument,       NULL, '4' },
  { "ipv6",         no_argument,       NULL, '6' },
  { NULL,           0,                 NULL, 0   }
};

const char opthelp[] = "\
-h, --help                Display this help and exit.\n\
-V, --version             Print the program version.\n\
-v, --verbose             Print verbose information.\n\
-d, --debug               Display debug information.\n\
-m, --compat-mode         Display in libbgpdump -m compatible mode.\n\
-b, --brief               List information (i.e., simple prefix-nexthops).\n\
-P, --peer-table          Display the peer table and exit.\n\
-p, --peer <peer_index>   Specify peers by peer_index.\n\
                          At most %d peers can be specified.\n\
-u, --diff                Shows unified diff. Specify two peers.\n\
-c, --count               Count route number.\n\
-k, --peer-stat           Shows prefix-length distribution.\n\
-N, --bufsiz              Specify the size of read buffer.\n\
                          (default: %s)\n\
-M, --nroutes             Specify the size of the route_table.\n\
                          (default: %s)\n\
-g, --benchmark           Measure the time to lookup.\n\
-l, --lookup <addr>       Specify lookup address.\n\
-L, --lookup-file <file>  Specify lookup address from a file.\n\
-4, --ipv4                Specify that the query is IPv4. (default)\n\
-6, --ipv6                Specify that the query is IPv6.\n\
";

int longindex;

int verbose = 0;
int detail = 0;
int debug = 0;
int show = 0;
int compat_mode = 0;
int brief = 0;
int peer_table_only = 0;
int udiff = 0;
int route_count = 0;
int stat = 0;
unsigned long long bufsiz = 0;
unsigned long long nroutes = 0;
int benchmark = 0;
int lookup = 0;
char *lookup_addr = NULL;
char *lookup_file = NULL;

int safi = AF_INET;
int qaf = AF_INET;

#define AUTLIM 8
unsigned long autnums[AUTLIM];
int autsiz = 0;

void
usage ()
{
  printf ("Usage: %s [options] <file1> <file2> ...\n", progname);
  printf (opthelp, PEER_INDEX_MAX, BGPDUMP_BUFSIZ_DEFAULT,
          ROUTE_LIMIT_DEFAULT);
}

void
version ()
{
  printf ("Version: %s.\n", BGPDUMP_VERSION);
}

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

  while (len && p + hsize + len <= data_end)
    {
      bgpdump_process_mrt_header (h, &info);

      switch (mrt_type)
        {
        case BGPDUMP_TYPE_TABLE_DUMP_V2:
          bgpdump_process_table_dump_v2 (h, &info, data_end);
          break;
        default:
          printf ("Not supported: mrt type: %d\n", mrt_type);
          printf ("discarding %lu bytes data.\n", data_end - p);
          *data_len = 0;
          return;
          break;
        }

      p += hsize + len;

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
      else
        len = 0;
    }

  /* move the partial, last-part data
     to the beginning of the buffer. */
  rest = data_end - p;
  if (rest)
    memmove (buf, p, rest);
  *data_len = rest;
}

unsigned long long
resolv_number (char *notation, char **endptr)
{
  unsigned long long number, unit;
  char digits[64];
  unsigned long digit;
  int len;
  char *p;

  snprintf (digits, sizeof (digits), "%s", notation);
  len = strlen (digits);

  unit = 1;
  if (len > 3)
    {
      p = strstr (digits, "KiB");
      if (! p)
        p = strstr (digits, "MiB");
      if (! p)
        p = strstr (digits, "GiB");
      if (! p)
        p = strstr (digits, "TiB");
      if (p)
        {
          switch (*p)
            {
            case 'K':
              unit = 1024ULL;
              break;
            case 'M':
              unit = 1024ULL * 1024ULL;
              break;
            case 'G':
              unit = 1024ULL * 1024ULL * 1024ULL;
              break;
            case 'T':
              unit = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
              break;
            default:
              unit = 1ULL;
              break;
            }
          *p = '\0';
          len = strlen (digits);
        }
    }
  if (len > 1)
    {
      switch (digits[len - 1])
        {
        case 'k':
        case 'K':
          unit = 1000ULL;
          digits[len - 1] = '\0';
          break;
        case 'm':
        case 'M':
          unit = 1000ULL * 1000ULL;
          digits[len - 1] = '\0';
          break;
        case 'g':
        case 'G':
          unit = 1000ULL * 1000ULL * 1000ULL;
          digits[len - 1] = '\0';
          break;
        case 'T':
          unit = 1000ULL * 1000ULL * 1000ULL * 1000ULL;
          digits[len - 1] = '\0';
          break;
        case 'P':
          unit = 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL;
          digits[len - 1] = '\0';
          break;
        default:
          break;
        }
    }

  digit = strtoul (digits, endptr, 0);
  number = digit * unit;

  return number;
}

int
main (int argc, char **argv)
{
  int ch;
  int status = 0;
  int i;
  char *endptr;
  int val;
  char *filename = NULL;

  progname = argv[0];

  bufsiz = resolv_number (BGPDUMP_BUFSIZ_DEFAULT, NULL);
  nroutes = resolv_number (ROUTE_LIMIT_DEFAULT, NULL);

  while (1)
    {
      ch = getopt_long (argc, argv, optstring, longopts, &longindex);

      if (ch == -1)
        break;

      switch (ch)
        {
        case 'h':
          usage ();
          exit (0);
          break;
        case 'V':
          version ();
          exit (0);
          break;
        case 'v':
          verbose++;
          if (verbose >= 2)
            detail++;
          break;
        case 'd':
          debug++;
          break;
        case 'm':
          compat_mode++;
          break;
        case 'b':
          brief++;
          break;

        case 'P':
          peer_table_only++;
          break;
        case 'p':
          val = strtoul (optarg, &endptr, 0);
          if (*endptr != '\0')
            {
              printf ("malformed peer_index: %s\n", optarg);
              exit (-1);
            }
          peer_spec_index[peer_spec_size % PEER_INDEX_MAX] = val;
          peer_spec_size++;
          break;
        case 'a':
          val = strtoul (optarg, &endptr, 0);
          if (*endptr != '\0')
            {
              printf ("malformed autnum: %s\n", optarg);
              exit (-1);
            }
          autnums[autsiz % AUTLIM] = val;
          autsiz++;
          break;

        case 'u':
          udiff++;
          break;
        case 'c':
          route_count++;
          break;
        case 'k':
          stat++;
          break;

        case 'N':
          bufsiz = resolv_number (optarg, &endptr);
          if (*endptr != '\0')
            {
              printf ("malformed bufsiz: %s\n", optarg);
              exit (-1);
            }
          break;
        case 'M':
          nroutes = resolv_number (optarg, &endptr);
          if (*endptr != '\0')
            {
              printf ("malformed nroutes: %s\n", optarg);
              exit (-1);
            }
          break;

        case 'l':
          lookup++;
          lookup_addr = optarg;
          break;
        case 'L':
          lookup++;
          lookup_file = optarg;
          break;
        case '4':
          qaf = AF_INET;
          break;
        case '6':
          qaf = AF_INET6;
          break;

        case 0:
          /* Process flag pointer. */
          break;
        case ':':
          fprintf (stderr, "A missing option argument.\n");
          status = -1;
          break;
        case '?':
          fprintf (stderr, "An unknown/ambiguous option.\n");
          status = -1;
          break;
        default:
          usage ();
          status = -1;
          break;
        }
    }

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
  if (! brief && ! show && ! route_count && ! udiff &&
      ! lookup && ! peer_table_only && ! stat && ! compat_mode &&
      ! autsiz)
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
  if (lookup)
    {
      route_init ();
      ptree[AF_INET] = ptree_create ();
      ptree[AF_INET6] = ptree_create ();
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
            break;

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

  if (lookup)
    {
      free (query_table);
      ptree_delete (ptree[AF_INET]);
      ptree_delete (ptree[AF_INET6]);
      route_finish ();
    }

  if (stat)
    {
      peer_stat_show ();
      //peer_stat_finish ();
    }

  free (buf);

  return status;
}


