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
#include <string.h>
#include <assert.h>

#include "queue.h"

struct queue *
queue_create ()
{
  struct queue *queue;
  queue = (struct queue *) malloc (sizeof (struct queue));
  memset (queue, 0, sizeof (struct queue));
  queue->array_size = QUEUE_INIT_SIZE;
  queue->array = (void **) malloc (queue->array_size * sizeof (void *));
  memset (queue->array, 0, queue->array_size * sizeof (void *));
  return queue;
}

void
queue_delete (struct queue *queue)
{
  free (queue->array);
  free (queue);
}

int
queue_size (struct queue *queue)
{
  int size;
  if (queue->end < queue->start)
    size = queue->array_size - queue->start + queue->end;
  else
    size = queue->end - queue->start;
  assert (size >= 0);
  return size;
}

static void
queue_expand (struct queue *queue)
{
  queue->array = realloc (queue->array,
                          queue->array_size * sizeof (void *) * 2);
  if (queue->end < queue->start)
    {
      memcpy (&queue->array[queue->array_size], &queue->array[0],
              queue->end * sizeof (void *));
      queue->end = queue->array_size + queue->end;
    }
#if QUEUE_DEBUG
  printf ("queue_expand(): queue[%p]: %d -> %d\n",
          queue, queue->array_size, queue->array_size * 2);
#endif /*QUEUE_DEBUG*/
  queue->array_size *= 2;
}

void
queue_enqueue (struct queue *queue, void *data)
{
#if QUEUE_DEBUG
  printf ("queue[%p]: enqueue [%p]\n", queue, data);
#endif /*QUEUE_DEBUG*/
  if (queue_size (queue) == queue->array_size - 1)
    queue_expand (queue);
  queue->array[queue->end++] = data;
  if (queue->end == queue->array_size)
    queue->end = 0;
#if QUEUE_DEBUG
  queue_print (queue);
#endif /*QUEUE_DEBUG*/
}

void *
queue_dequeue (struct queue *queue)
{
  void *data;
  if (queue_size (queue) == 0)
    return NULL;
  data = queue->array[queue->start++];
  if (queue->start == queue->array_size)
    queue->start = 0;
#if QUEUE_DEBUG
  printf ("queue[%p]: dequeue [%p]\n", queue, data);
  queue_print (queue);
#endif /*QUEUE_DEBUG*/
  return data;
}

void
queue_print (struct queue *queue)
{
  int i;
  printf ("queue[%p]: array[%p] array_size: %d\n",
          queue, queue->array, queue->array_size);
  printf ("  start %d end %d size %d\n",
          queue->start, queue->end, queue_size (queue));
  printf ("  array: ");
  if (queue->start < queue->end)
    {
      for (i = queue->start; i < queue->end; i++)
        printf ("[%p]", queue->array[i]);
    }
  else if (queue->end < queue->start)
    {
      for (i = queue->start; i < queue->array_size; i++)
        printf ("[%p]", queue->array[i]);
      for (i = 0; i < queue->end; i++)
        printf ("[%p]", queue->array[i]);
    }
  printf ("\n");
}

