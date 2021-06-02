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
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

#include <bzlib.h>
#include <zlib.h>

#include "bgpdump_file.h"

struct access_method methods[] =
{
  { (fopen_t)fopen, (fread_t)fread_wrap, (fwrite_t)fwrite,
    (fclose_t)fclose, (feof_t)feof },
  { (fopen_t)bopen, (fread_t)bread, (fwrite_t)bwrite,
    (fclose_t)bclose, (feof_t)bfeof },
  { (fopen_t)gopen, (fread_t)gread, (fwrite_t)gwrite,
    (fclose_t)gclose, (feof_t)gzeof },
};

struct fhandle fhandle;
int bzerror;

size_t
fread_wrap (void *ptr, size_t size, size_t nitems, void *file)
{
  size_t ret;
  FILE *f = (FILE *) file;
  nitems *= size;
  ret = fread (ptr, 1, nitems, f);
  //printf ("fread: ret: %ld\n", ret);
  return ret;
}

void *
bopen (const char *filename, const char *mode)
{
  struct fhandle *f = &fhandle;
  f->file1 = fopen (filename, mode);
  if (! f->file1)
    return NULL;
  f->file2 = BZ2_bzReadOpen (&bzerror, f->file1, 0, 0, NULL, 0);
  if (bzerror != BZ_OK)
    {
      errno = bzerror;
      bclose (f);
      return NULL;
    }
  return f;
}

size_t
bread (void *ptr, size_t size, size_t nitems, void *file)
{
  struct fhandle *f = (struct fhandle *) file;
  size_t ret;
  ret = BZ2_bzRead (&bzerror, (BZFILE *) f->file2, ptr, size * nitems);
  if (bzerror == BZ_STREAM_END)
    {
      char unused[1024];
      int nunused = sizeof (unused);
      BZ2_bzReadGetUnused (&bzerror, (BZFILE *) f->file2,
                           (void **)&unused, &nunused);
    }
  return ret;
}

size_t
bwrite (void *ptr, size_t size, size_t nitems, void *file)
{
  return 0;
}

int
bclose (void *file)
{
  struct fhandle *f = (struct fhandle *) file;
  BZ2_bzReadClose (&bzerror, (BZFILE *) f->file2);
  fclose (f->file1);
  f->file2 = NULL;
  f->file1 = NULL;
  return 0;
}

int
bfeof (void *file)
{
  struct fhandle *f = (struct fhandle *) file;
  int ret;
  ret = feof (f->file1);
  return ret;
}

void *
gopen (const char *filename, const char *mode)
{
  gzFile f;
  f = gzopen (filename, mode);
  if (! f)
    {
      fprintf (stderr, "gzopen() failed: %s\n", strerror (errno));
      return NULL;
    }
  return f;
}

size_t
gread (void *ptr, size_t size, size_t nitems, void *file)
{
  gzFile f = (gzFile) file;
  int ret = 0;
  ret = gzread (f, ptr, size * nitems);
  if (ret < 0)
    {
      fprintf (stderr, "gzread failed.\n");
      return ret;
    }
  return ret;
}

size_t
gwrite (void *ptr, size_t size, size_t nitems, void *file)
{
  return 0;
}

int
gclose (void *file)
{
  gzFile f = (gzFile) file;
  gzclose (f);
  return 0;
}

file_format_t
get_file_format (char *filename)
{
  char *p;
  p = rindex (filename, '.');
  if (p == NULL)
    return FORMAT_RAW;
  if (! strcmp (p, ".bz2"))
    {
      //printf ("file type: bzip2.\n");
      return FORMAT_BZIP2;
    }
  if (! strcmp (p, ".gz"))
    return FORMAT_GZIP;
  return FORMAT_RAW;
}

struct access_method *
get_access_method (file_format_t format)
{
  switch (format)
    {
    case FORMAT_RAW:
      return &methods[FORMAT_RAW];
    case FORMAT_BZIP2:
      return &methods[FORMAT_BZIP2];
    case FORMAT_GZIP:
      return &methods[FORMAT_GZIP];
    default:
      return NULL;
    }
  return NULL;
}


