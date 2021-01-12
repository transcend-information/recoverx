/*

    File: file_d2s.c

    Copyright (C) 2009 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#include "common.h"
#include "filegen.h"

static void register_header_check_d2s(file_stat_t *file_stat);

const file_hint_t file_hint_d2s= {
  .extension="d2s",
  .description="Diablo II",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_d2s
};

struct d2s_header {
  char magic[8];
  uint32_t size;
  uint32_t unk1;
  uint32_t unk2;
  char name[0];
} __attribute__ ((gcc_struct, __packed__));

static void file_rename_d2s(file_recovery_t *file_recovery)
{
  unsigned char buffer[512];
  FILE *file;
  int buffer_size;
  if((file=fopen(file_recovery->filename, "rb"))==NULL)
    return;
  buffer_size=fread(buffer, 1, sizeof(buffer), file);
  fclose(file);
  file_rename(file_recovery, buffer, buffer_size, 0x14, NULL, 1);
}

static int header_check_d2s(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct d2s_header *d2s=(const struct d2s_header*)buffer;
  if(le32(d2s->size) < sizeof(struct d2s_header))
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_d2s.extension;
  file_recovery_new->calculated_file_size=le32(d2s->size);
  file_recovery_new->data_check=&data_check_size;
  file_recovery_new->file_check=&file_check_size;
  file_recovery_new->file_rename=&file_rename_d2s;
  return 1;
}

static void register_header_check_d2s(file_stat_t *file_stat)
{
  static const unsigned char d2s_header[8]= {
    0x55, 0xaa, 0x55, 0xaa, 0x60, 0x00, 0x00, 0x00
  };
  register_header_check(0, d2s_header,sizeof(d2s_header), &header_check_d2s, file_stat);
}
