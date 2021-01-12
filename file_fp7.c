/*

    File: file_fp7.c

    Copyright (C) 2009,2015 Christophe GRENIER <grenier@cgsecurity.org>
  
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

static void register_header_check_fp7(file_stat_t *file_stat);
static int header_check_fp7(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new);

const file_hint_t file_hint_fp7= {
  .extension="fp7",
  .description="File Maker Pro",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_fp7
};

static void file_check_fp7(file_recovery_t *file_recovery)
{
  file_recovery->file_size=file_recovery->file_size/4096*4096;
}

static int header_check_fp7(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  if(buffer_size < 0x230 || memcmp(&buffer[0x20d], "HBAM", 4)!=0)
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->min_filesize=4096;
  file_recovery_new->file_check=&file_check_fp7;
  if(memcmp(&buffer[0x21e], "Pro 12", 6)==0)
    file_recovery_new->extension="fmp12";
  else
    file_recovery_new->extension=file_hint_fp7.extension;
  return 1;
}

static void register_header_check_fp7(file_stat_t *file_stat)
{
  static const unsigned char fp7_header[0x14]= {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01,
    0x00, 0x05, 0x00, 0x02, 0x00, 0x02, 0xc0,  'H',
    'B',  'A',  'M',  '7'
  };
  register_header_check(0, fp7_header,sizeof(fp7_header), &header_check_fp7, file_stat);
}
