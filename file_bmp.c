/*

    File: file_bmp.c

    Copyright (C) 1998-2007 Christophe GRENIER <grenier@cgsecurity.org>
  
    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
  
    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include "types.h"
#include "filegen.h"
#include "common.h"
#if defined(__FRAMAC__)
#include "__fc_builtin.h"
#endif

static void register_header_check_bmp(file_stat_t *file_stat);

const file_hint_t file_hint_bmp= {
  .extension="bmp",
  .description="BMP bitmap image",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_bmp
};

static const unsigned char bmp_header[2]= {'B','M'};

struct bmp_header
{
  uint16_t magic;
  uint32_t size;
  uint32_t reserved;
  uint32_t offset;
  uint32_t hdr_size;
} __attribute__ ((gcc_struct, __packed__));

/*@
  @ requires buffer_size >= 18;
  @ requires \valid_read(buffer+(0..buffer_size-1));
  @ requires \valid_read(file_recovery);
  @ requires \valid(file_recovery_new);
  @ requires file_recovery_new->blocksize > 0;
  @ ensures \result == 0 || \result == 1;
  @ ensures (\result == 1) ==> (file_recovery_new->extension == file_hint_bmp.extension);
  @ ensures (\result == 1) ==> (file_recovery_new->calculated_file_size >= 65);
  @ ensures (\result == 1) ==> (file_recovery_new->file_size == 0);
  @ ensures (\result == 1) ==> (file_recovery_new->min_filesize == 65);
  @ ensures (\result == 1) ==> (file_recovery_new->data_check == &data_check_size);
  @ ensures (\result == 1) ==> (file_recovery_new->file_check == &file_check_size);
  @*/
static int header_check_bmp(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct bmp_header *bm=(const struct bmp_header *)buffer;
  if(buffer[0]!='B' || buffer[1]!='M')
    return 0;
  if(bm->reserved!=0)
    return 0;
  if(
      (buffer[14]==12 || buffer[14]==64 || buffer[14]==40 || buffer[14]==52 ||
       buffer[14]==56 || buffer[14]==108 || buffer[14]==124) &&
      buffer[15]==0 && buffer[16]==0 && buffer[17]==0 &&
      le32(bm->offset) < le32(bm->size) &&
      le32(bm->size) >= 65 &&
      le32(bm->hdr_size) < le32(bm->size))
  {
    /* See http://en.wikipedia.org/wiki/BMP_file_format */
    reset_file_recovery(file_recovery_new);
    file_recovery_new->extension=file_hint_bmp.extension;
    file_recovery_new->min_filesize=65;
    file_recovery_new->calculated_file_size=(uint64_t)le32(bm->size);
    file_recovery_new->data_check=&data_check_size;
    file_recovery_new->file_check=&file_check_size;
    /*@ assert file_recovery_new->extension == file_hint_bmp.extension; */
    /*@ assert file_recovery_new->calculated_file_size >= 65; */
    /*@ assert file_recovery_new->file_size == 0; */
    /*@ assert file_recovery_new->min_filesize == 65; */
    /*@ assert file_recovery_new->data_check == &data_check_size; */
    /*@ assert file_recovery_new->file_check == &file_check_size; */
    return 1;
  }
  return 0;
}

static void register_header_check_bmp(file_stat_t *file_stat)
{
  register_header_check(0, bmp_header,sizeof(bmp_header), &header_check_bmp, file_stat);
}

#if defined(MAIN_bmp)
#define BLOCKSIZE 65536u
int main()
{
  unsigned char buffer[BLOCKSIZE];
  int res;
  file_recovery_t file_recovery_new;
  file_recovery_t file_recovery;
  file_stat_t file_stats;

  /*@ assert \valid(buffer + (0 .. (BLOCKSIZE - 1))); */
#if defined(__FRAMAC__)
  Frama_C_make_unknown((char *)buffer, BLOCKSIZE);
#endif

  reset_file_recovery(&file_recovery);
  file_recovery.blocksize=BLOCKSIZE;
  file_recovery_new.blocksize=BLOCKSIZE;
  file_recovery_new.data_check=NULL;
  file_recovery_new.file_stat=NULL;
  file_recovery_new.file_check=NULL;
  file_recovery_new.file_rename=NULL;
  file_recovery_new.calculated_file_size=0;
  file_recovery_new.file_size=0;
  file_recovery_new.location.start=0;

  file_stats.file_hint=&file_hint_bmp;
  file_stats.not_recovered=0;
  file_stats.recovered=0;
  file_hint_bmp.register_header_check(&file_stats);
  if(header_check_bmp(buffer, BLOCKSIZE, 0u, &file_recovery, &file_recovery_new)!=1)
    return 0;
  memcpy(file_recovery_new.filename, "demo", 5);
  /*@ assert file_recovery_new.extension == file_hint_bmp.extension; */
  /*@ assert file_recovery_new.calculated_file_size >= 65; */
  /*@ assert file_recovery_new.file_size == 0;	*/
  /*@ assert file_recovery_new.min_filesize == 65;	*/
  /*@ assert file_recovery_new.file_check == &file_check_size; */
  /*@ assert file_recovery_new.data_check == &data_check_size; */
  file_recovery_new.file_stat=&file_stats;
  /*@ assert file_recovery_new.file_stat->file_hint!=NULL; */
  if(file_recovery_new.data_check!=NULL)
  {
    unsigned char big_buffer[2*BLOCKSIZE];
    data_check_t res_data_check=DC_CONTINUE;
    memset(big_buffer, 0, BLOCKSIZE);
    memcpy(big_buffer + BLOCKSIZE, buffer, BLOCKSIZE);
    /*@ assert file_recovery_new.data_check == &data_check_size; */
    /*@ assert file_recovery_new.file_size == 0; */;
    /*@ assert file_recovery_new.file_size <= file_recovery_new.calculated_file_size; */;
    res_data_check=file_recovery_new.data_check(big_buffer, 2*BLOCKSIZE, &file_recovery_new);
    file_recovery_new.file_size+=BLOCKSIZE;
    if(res_data_check == DC_CONTINUE)
    {
      memcpy(big_buffer, big_buffer + BLOCKSIZE, BLOCKSIZE);
#if defined(__FRAMAC__)
      Frama_C_make_unknown((char *)big_buffer + BLOCKSIZE, BLOCKSIZE);
#endif
      file_recovery_new.data_check(big_buffer, 2*BLOCKSIZE, &file_recovery_new);
    }
  }
  {
    file_recovery_t file_recovery_new2;
    file_recovery_new2.blocksize=BLOCKSIZE;
    file_recovery_new2.file_stat=NULL;
    file_recovery_new2.file_check=NULL;
    file_recovery_new2.location.start=BLOCKSIZE;
    file_recovery_new.handle=NULL;	/* In theory should be not null */
#if defined(__FRAMAC__)
    Frama_C_make_unknown((char *)buffer, BLOCKSIZE);
#endif
    header_check_bmp(buffer, BLOCKSIZE, 0, &file_recovery_new, &file_recovery_new2);
  }
  if(file_recovery_new.file_check!=NULL)
  {
    file_recovery_new.handle=fopen("demo", "rb");
    if(file_recovery_new.handle!=NULL)
    {
      (file_recovery_new.file_check)(&file_recovery_new);
      fclose(file_recovery_new.handle);
    }
  }
  if(file_recovery_new.file_rename!=NULL)
  {
    /*@ assert valid_read_string((char *)&file_recovery_new.filename); */
    (file_recovery_new.file_rename)(&file_recovery_new);
  }
  return 0;
}
#endif
