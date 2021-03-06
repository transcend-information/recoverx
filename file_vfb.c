/*

    File: file_vfb.c

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
#include "filegen.h"

static void register_header_check_vfb(file_stat_t *file_stat);

const file_hint_t file_hint_vfb= {
  .extension="vfb",
  .description="FontLab",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_vfb
};

static void file_check_vfb(file_recovery_t *file_recovery)
{
  const unsigned char vfb_footer[9]= {
    0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
    0x00
  };
  file_search_footer(file_recovery, vfb_footer, sizeof(vfb_footer), 0);
}

static int header_check_vfb(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_vfb.extension;
  file_recovery_new->file_check=&file_check_vfb;
  return 1;
}

static void register_header_check_vfb(file_stat_t *file_stat)
{
  static const unsigned char vfb_header[8]=  {
    0x1a, 'W' , 'L' , 'F' , '1' , '0' , 0x03, 0x00
  };
  register_header_check(0, vfb_header, sizeof(vfb_header), &header_check_vfb, file_stat);
}

