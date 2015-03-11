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

#ifndef _BGPDUMP_FILE_H_
#define _BGPDUMP_FILE_H_

typedef enum
{
  FORMAT_RAW,
  FORMAT_BZIP2,
  FORMAT_GZIP,
  FORMAT_UNKNOWN
} file_format_t;

typedef void * (*fopen_t) (const char *, const char *);
typedef size_t (*fread_t) (void *, size_t, size_t, void *);
typedef size_t (*fwrite_t) (void *, size_t, size_t, void *);
typedef int (*fclose_t) (void *);
typedef int (*feof_t) (void *);

struct access_method
{
  fopen_t fopen;
  fread_t fread;
  fwrite_t fwrite;
  fclose_t fclose;
  feof_t feof;
};

struct fhandle
{
  FILE *file1;
  void *file2;
};

void *bopen (const char *filename, const char *mode);
size_t bread (void *ptr, size_t size, size_t nitems, void *file);
size_t bwrite (void *ptr, size_t size, size_t nitems, void *file);
int bclose (void *file);
int bfeof (void *file);

void *gopen (const char *filename, const char *mode);
size_t gread (void *ptr, size_t size, size_t nitems, void *file);
size_t gwrite (void *ptr, size_t size, size_t nitems, void *file);
int gclose (void *file);

file_format_t get_file_format (char *filename);
struct access_method *get_access_method (file_format_t format);

#endif /*_BGPDUMP_FILE_H_*/

