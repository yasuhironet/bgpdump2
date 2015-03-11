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

#ifndef _PTREE_H_
#define _PTREE_H_

struct ptree_node {
  char *key;
  int   keylen;
  struct ptree_node *parent;
  struct ptree_node *child[2];
  void *data;
};

#define PTREE_KEY_SIZE(len) (((len) + 7) / 8)

#if 0
#define PTREE_LEFT(x) (&(x)->child[0])
#define PTREE_RIGHT(x) (&(x)->child[1])
#endif /*0*/

struct ptree {
  struct ptree_node *top;
};

#define XRTMALLOC(p, t, n) (p = (t) malloc ((unsigned int)(n)))
#define XRTFREE(p) free((char *)p)
#define XRTLOG(pri, fmt, ...) printf (fmt, __VA_ARGS__)
#define XRTASSERT(exp, string) assert (exp)

#ifndef MIN
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif /*MIN*/

void ptree_node_print (struct ptree_node *x);

struct ptree_node *ptree_search (char *key, int keylen, struct ptree *t);
struct ptree_node *ptree_search_exact (char *key, int keylen, struct ptree *t);

struct ptree_node *ptree_add (char *key, int keylen, void *data, struct ptree *t);
void ptree_remove (struct ptree_node *v);

struct ptree_node *ptree_head (struct ptree *t);
struct ptree_node *ptree_next (struct ptree_node *v);
struct ptree_node *ptree_next_within (int from, int to, struct ptree_node *v);

struct ptree *ptree_create ();
void ptree_delete (struct ptree *t);

int ptree_count (struct ptree *t);

#endif /*_PTREE_H_*/

