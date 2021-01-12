/*

    File: file_EXTENSION.c

    Copyright (C) YEAR Christophe GRENIER <grenier@cgsecurity.org>

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

static void register_header_check_EXTENSION(file_stat_t *file_stat);

const file_hint_t file_hint_EXTENSION= {
  .extension="EXTENSION",
  .description="EXTENSION",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_EXTENSION
};

static void file_check_EXTENSION(file_recovery_t *file_recovery)
{
  const unsigned char EXTENSION_footer[FOOTER_SIZE]= {
    FOOTER_MAGIC
  };
  file_search_footer(file_recovery, EXTENSION_footer, sizeof(EXTENSION_footer), FOOTER_EXTRA);
}

static int header_check_EXTENSION(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_EXTENSION.extension;
  file_recovery_new->file_check=&file_check_EXTENSION;
  return 1;
}

static void register_header_check_EXTENSION(file_stat_t *file_stat)
{
  static const unsigned char EXTENSION_header[HEADER_SIZE]=  {
    HEADER_MAGIC
  };
  register_header_check(HEADER_LOC, EXTENSION_header, sizeof(EXTENSION_header), &header_check_EXTENSION, file_stat);
}