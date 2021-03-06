/*

    File: file_pf.c

    Copyright (C) 2016 Christophe GRENIER <grenier@cgsecurity.org>
    and Ralf Almon usd AG 2016
  
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

static void register_header_check_pf(file_stat_t *file_stat);

const file_hint_t file_hint_pf= {
  .extension="pf",
  .description="Windows prefetch file",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_pf
};

struct pf_header
{
  uint32_t version;
  uint32_t magic;
  uint32_t unknown;
  uint32_t size;
  char     name[60];
  uint32_t hash;
  uint32_t unknown2;
} __attribute__ ((gcc_struct, __packed__));

static void file_rename_pf(file_recovery_t *file_recovery)
{
  FILE *file;
  struct pf_header hdr;
  if((file=fopen(file_recovery->filename, "rb"))==NULL)
    return;
  if(fread(&hdr, sizeof(hdr), 1, file) <= 0)
  {
    fclose(file);
    return ;
  }
  fclose(file);
  file_rename_unicode(file_recovery, &hdr.name, sizeof(hdr.name), 0, "pf", 0);
}

static int header_check_pf(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct pf_header *pf=(const struct pf_header *)buffer;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_pf.extension;
  file_recovery_new->calculated_file_size=(uint64_t)le32(pf->size);
  file_recovery_new->file_rename=&file_rename_pf;
  file_recovery_new->data_check=&data_check_size;
  file_recovery_new->file_check=&file_check_size;
  return 1;
}

static void register_header_check_pf(file_stat_t *file_stat)
{
  static const unsigned char pf_header[7] = {0x00, 0x00, 0x00, 'S', 'C', 'C', 'A'};
  register_header_check(1, pf_header,sizeof(pf_header), &header_check_pf, file_stat);
}
