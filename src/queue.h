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

#ifndef _QUEUE_H_
#define _QUEUE_H_

#define QUEUE_DEBUG     0
#define QUEUE_INIT_SIZE 2

struct queue
{
  void **array;
  int array_size;
  int start;
  int end;
};

struct queue *queue_create ();
void queue_delete (struct queue *queue);
int queue_size (struct queue *queue);

void queue_enqueue (struct queue *queue, void *data);
void *queue_dequeue (struct queue *queue);

void queue_print (struct queue *queue);

#endif /*_QUEUE_H_*/

