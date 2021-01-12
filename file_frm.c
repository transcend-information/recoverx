/*

    File: file_frm.c

    Copyright (C) 2008 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#include <ctype.h>
#include "types.h"
#include "filegen.h"


static void register_header_check_frm(file_stat_t *file_stat);
static int header_check_frm(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new);
static void file_check_frm(file_recovery_t *file_recovery);

const file_hint_t file_hint_frm= {
  .extension="frm",
  .description="Pro/ENGINEER Drawing Form",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_frm
};

static void file_check_frm(file_recovery_t *file_recovery)
{
  const unsigned char frm_footer[11]= {
    '#', 'E', 'N', 'D', '_', 'O', 'F', '_',
    'U', 'G', 'C'};
  file_search_footer(file_recovery, frm_footer, sizeof(frm_footer), 1);
}

static int header_check_frm(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  if(!isprint(buffer[14]) || !isprint(buffer[15]) || !isprint(buffer[16]) || !isprint(buffer[17]))
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->file_check=&file_check_frm;
  file_recovery_new->extension=file_hint_frm.extension;
  return 1;
}

static void register_header_check_frm(file_stat_t *file_stat)
{
  static const unsigned char frm_header[14]= {
    '#', 'U', 'G', 'C', ':', '2', ' ', 'D',
    'W', 'G', 'F', 'O', 'R', 'M'};
  register_header_check(0, frm_header,sizeof(frm_header), &header_check_frm, file_stat);
}
