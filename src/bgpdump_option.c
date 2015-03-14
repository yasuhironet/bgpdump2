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
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bgpdump_parse.h"
#include "bgpdump_option.h"
#include "bgpdump_peer.h"
#include "bgpdump_route.h"

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

const char *optstring = "hVvdmbPp:a:uUrckN:M:gl:L:46";
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
  { "diff-verbose", no_argument,       NULL, 'U' },
  { "diff-table",   no_argument,       NULL, 'r' },
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
-U, --diff-verbose        Shows the detailed info of unified diff.\n\
-r, --diff-table          Specify to create diff route_table.\n\
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
int udiff_verbose = 0;
int udiff_lookup = 0;
int route_count = 0;
int stat = 0;
unsigned long long bufsiz = 0;
unsigned long long nroutes = 0;
int benchmark = 0;
int lookup = 0;
char *lookup_addr = NULL;
char *lookup_file = NULL;

extern char *progname;
extern int safi;
extern int qaf;

extern unsigned long autnums[];
extern int autsiz;

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

int
bgpdump_getopt (int argc, char **argv)
{
  int ch;
  int status = 0;
  char *endptr;
  int val;

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
        case 'U':
          udiff++;
          udiff_verbose++;
          break;
        case 'r':
          udiff_lookup++;
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

  return status;
}

