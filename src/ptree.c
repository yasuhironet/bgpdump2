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

/* Patricia-like Tree */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdint.h>
#include <sys/types.h>
#include <assert.h>

#include "queue.h"
#include "ptree.h"

char mask[] = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

static struct ptree_node *
ptree_node_create (char *key, int keylen)
{
  struct ptree_node *x;
  int len;

  len = sizeof (struct ptree_node) + (keylen + 7) / 8;

  XRTMALLOC(x, struct ptree_node *, len);
  if (! x)
    return NULL;

  x->key = (char *)((caddr_t)x + sizeof (struct ptree_node));
  x->keylen = keylen;
  x->parent = NULL;
  x->child[0] = NULL;
  x->child[1] = NULL;
  x->data = NULL;

  /* fill in the key */
  memcpy (x->key, key, (keylen + 7) / 8);
  if (keylen % 8)
    x->key[keylen / 8] = key[keylen / 8] & mask[keylen % 8];

  return x;
}

static void
ptree_node_delete (struct ptree_node *x)
{
  XRTFREE (x);
}

void
ptree_node_print (struct ptree_node *x)
{
  int i;
  printf ("node[%p]: ", x);
  for (i = 0; i < PTREE_KEY_SIZE (x->keylen); i++)
    {
      printf ("%d", (int) (unsigned char) x->key[i]);
      if (i + 1 < PTREE_KEY_SIZE (x->keylen))
        printf (".");
    }
  if (i == 0)
    printf ("0");
  printf ("/%d ", x->keylen);
  printf ("data: %p, ", x->data);
  printf ("pa: %p ch: [%p,%p]\n", x->parent, x->child[0], x->child[1]);
}

/* check_bit() returns the "keylen"-th bit in the key.
   key[keylen] would return the bit just after the key,
   because the index of the key[] starts with 0-origin. */
int
check_bit (char *key, int keylen)
{
  int offset;
  int shift;

  offset = keylen / 8;
  shift = 7 - keylen % 8;

  return (key[offset] >> shift & 1);
}

/* ptree_match() returns 1 iff keyi and keyj are the same
   in keylen bits */
int
ptree_match (char *keyi, char *keyj, int keylen)
{
  int bytes;
  int bits;
  bytes = keylen / 8;
  bits = keylen % 8;
  if (! memcmp (keyi, keyj, bytes) &&
      ! ((keyi[bytes] ^ keyj[bytes]) & mask[bits]))
    return 1;
  return 0;
}

/* ptree_lookup() returns the node with the key if any.
   returned node may be a branching node (that doesn't have data). */
struct ptree_node *
ptree_lookup (char *key, int keylen, struct ptree *t)
{
  struct ptree_node *x;

  x = t->top;
  while (x && x->keylen < keylen &&
         ptree_match (x->key, key, x->keylen))
    x = x->child[check_bit (key, x->keylen)];

  if (x && x->keylen == keylen &&
      ptree_match (x->key, key, x->keylen))
    return x;

  return NULL;
}

/* ptree_search() returns the ptree_node with data
   that matches the key. If data is NULL, it is a branching node,
   and ptree_search() ignores it. */
struct ptree_node *
ptree_search (char *key, int keylen, struct ptree *t)
{
  struct ptree_node *x, *match;

  match = NULL;
  x = t->top;
  while (x && x->keylen <= keylen &&
         ptree_match (x->key, key, x->keylen))
    {
      if (x->data)
        match = x;
      x = x->child[check_bit (key, x->keylen)];
    }

  return match;
}

struct ptree_node *
ptree_search_exact (char *key, int keylen, struct ptree *t)
{
  struct ptree_node *x, *match;

  match = NULL;
  x = t->top;
  while (x && x->keylen <= keylen &&
         ptree_match (x->key, key, x->keylen))
    {
      if (x->data)
        match = x;
      x = x->child[check_bit (key, x->keylen)];
    }

  if (match && match->keylen == keylen)
    return match;

  return NULL;
}

static void
ptree_link (struct ptree_node *v, struct ptree_node *w)
{
  /* check the w's key bit just after the v->key (keylen'th bit) */
  int bit;

  bit = check_bit (w->key, v->keylen);
  v->child[bit] = w;
  w->parent = v;
}

/* key_common_len() returns the bit length in which the keyi and
   the keyj are equal */
static int
key_common_len (char *keyi, int keyilen, char *keyj, int keyjlen)
{
  int nmatch = 0;
  int minkeylen = MIN (keyilen, keyjlen);
  int keylen = 0;
  unsigned char bitmask;
  unsigned char diff;

  nmatch = 0;
  while (nmatch < minkeylen / 8 && keyi[nmatch] == keyj[nmatch])
    {
      nmatch++;
    }

  keylen = nmatch * 8;
  bitmask = 0x80;
  diff = keyi[nmatch] ^ keyj[nmatch];
  while (keylen < minkeylen && ! (bitmask & diff))
    {
      keylen++;
      bitmask >>= 1;
    }

  return keylen;
}

/* ptree_common() creates and returns the branching node
   between keyi and keyj */
static struct ptree_node *
ptree_common (char *keyi, int keyilen, char *keyj, int keyjlen)
{
  int keylen;
  struct ptree_node *x;

  keylen = key_common_len (keyi, keyilen, keyj, keyjlen);
  x = ptree_node_create (keyi, keylen);
  return x;
}

static struct ptree_node *
ptree_get (char *key, int keylen, struct ptree *t)
{
  struct ptree_node *x;
  struct ptree_node *u, *v, *w; /* u->v->w or u->x->{v, w}*/

  u = w = NULL;
  x = t->top;
  while (x && x->keylen <= keylen &&
         ptree_match (x->key, key, x->keylen))
    {
      if (x->keylen == keylen)
        return x;
      u = x;
      x = x->child[check_bit (key, x->keylen)];
    }

  if (! x)
    {
      v = ptree_node_create (key, keylen);
      if (u)
        ptree_link (u, v);
      else
        t->top = v;
    }
  else
    {
      /* we're going to insert between u and w (previously x) */
      w = x;

      /* create branching node */
      x = ptree_common (key, keylen, w->key, w->keylen);
      if (! x)
        {
          XRTLOG (LOG_ERR, "ptree_get(%p,%d): "
                  "ptree_common() failed.\n", key, keylen);
          return NULL;
        }

      /* set lower link */
      ptree_link (x, w);

      /* set upper link */
      if (u)
        ptree_link (u, x);
      else
        t->top = x;

      /* if the branching node is not the corresponding node,
         create the corresponding node to add */
      if (x->keylen == keylen)
        v = x;
      else
        {
          v = ptree_node_create (key, keylen);
          if (! v)
            {
              XRTLOG (LOG_ERR, "ptree_get(%p,%d): "
                      "ptree_common() failed.\n", key, keylen);
              return NULL;
            }

          ptree_link (x, v);
        }
    }

  return v;
}

struct ptree_node *
ptree_add (char *key, int keylen, void *data, struct ptree *t)
{
  struct ptree_node *x;

  x = ptree_get (key, keylen, t);
  if (! x)
    {
      XRTLOG (LOG_ERR, "ptree_add(%p,%d): "
              "ptree_get() failed.\n", key, keylen);
      return NULL;
    }

  x->data = data;

  return x;
}

void
ptree_remove (struct ptree_node *v)
{
  struct ptree_node *w;

  XRTASSERT (! v->data, ("ptree: attempt to remove a node with data"));

  /* do not remove if the node is a branching node */
  if (v->child[0] && v->child[1])
    return;

  /* if a stub node */
  if (! v->child[0] && ! v->child[1])
    {
      if (v->parent->child[0] == v)
        v->parent->child[0] = NULL;
      else
        v->parent->child[1] = NULL;
      ptree_node_delete (v);
      return;
    }

  w = (v->child[0] ? v->child[0] : v->child[1]);
  ptree_link (v->parent, w);
  ptree_node_delete (v);
}

struct ptree_node *
ptree_head (struct ptree *t)
{
  if (! t->top)
    return NULL;
  return t->top;
}

struct ptree_node *
ptree_next (struct ptree_node *v)
{
  struct ptree_node *t;
  struct ptree_node *u;
  struct ptree_node *w;

  /* if the left child exists, go left */
  if (v->child[0])
    {
      w = v->child[0];
      return w;
    }

  if (v->child[1])
    {
      w = v->child[1];
      return w;
    }

  u = v->parent;

  if (! u)
    return NULL;

  if (u->child[0] == v)
    {
      w = u->child[1];
      if (w)
        return w;
    }

  t = u->parent;
  while (t && (t->child[1] == u || ! t->child[1]))
    {
      u = t;
      t = t->parent;
    }

  if (t)
    {
      /* return the not-yet-traversed right-child node */
      w = t->child[1];
      XRTASSERT (w, ("xrt: an impossible end of traverse"));
      return w;
    }

  /* end of traverse */
  return NULL;
}

struct ptree_node *
ptree_next_within (int from, int to, struct ptree_node *v)
{
  struct ptree_node *t;
  struct ptree_node *u;
  struct ptree_node *w;

  /* if the keylen == to, go up */
  if (v->keylen < to)
    {
      /* if the left child exists, go left */
      if (v->child[0])
        {
          w = v->child[0];
          return w;
        }

      /* else, if the right child exists, go right */
      if (v->child[1])
        {
          w = v->child[1];
          return w;
        }
    }

  u = v->parent;

  if (! u)
    return NULL;

  if (u->keylen < from)
    return NULL;

  if (u->child[0] == v)
    {
      w = u->child[1];
      if (w)
        return w;
    }

  t = u->parent;
  while (t && from <= t->keylen &&
         (t->child[1] == u || ! t->child[1]))
    {
      u = t;
      t = t->parent;
    }

  if (t && from <= t->keylen)
    {
      /* return the not-yet-traversed right-child node */
      w = t->child[1];
      XRTASSERT (w, ("xrt: an impossible end of traverse"));
      return w;
    }

  /* end of traverse */
  return NULL;
}

struct ptree *
ptree_create (void)
{
  struct ptree *t;

  XRTMALLOC(t, struct ptree *, sizeof (struct ptree));
  if (! t)
    return NULL;

  t->top = NULL;
  return t;
}

void
ptree_delete (struct ptree *t)
{
  struct ptree_node *x, *next;
  struct queue *q;

  q = queue_create ();

  x = ptree_head (t);
  while (x)
    {
      next = ptree_next (x);

      /* remove actually. */
      x->data = NULL;
      queue_enqueue (q, x);

      x = next;
    }

  while ((x = queue_dequeue (q)) != NULL)
    {
      ptree_node_delete (x);
    }

  queue_delete (q);
  XRTFREE (t);
}

int
ptree_count (struct ptree *t)
{
  int count = 0;
  struct ptree_node *x;
  for (x = ptree_head (t); x; x = ptree_next (x))
    {
      if (x->data)
        count++;
    }
  return count;
}


