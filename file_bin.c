/*

    File: file_bin.c

    Copyright (C) 2015 Christophe GRENIER <grenier@cgsecurity.org>

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

static void register_header_check_bin(file_stat_t *file_stat);

const file_hint_t file_hint_bin= {
  .extension="bin",
  .description="DINO",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_bin
};

struct ticket_header
{
  uint16_t magic;	// 01 04
  uint32_t size;
  uint32_t unk;		// 00 07 00 07
  char	   data_len;	// 9
  char	   data[9];	// TaTickets
} __attribute__ ((gcc_struct, __packed__));

static int header_check_bin(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct ticket_header *hdr=(const struct ticket_header *)buffer;
  if(le32(hdr->size) < 65)
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension="Ticket.bin";
  file_recovery_new->calculated_file_size=(uint64_t)le32(hdr->size);
  file_recovery_new->data_check=&data_check_size;
  file_recovery_new->file_check=&file_check_size;
  file_recovery_new->min_filesize=65;
  return 1;
}

static void register_header_check_bin(file_stat_t *file_stat)
{
  static const unsigned char bin_header[13]= {
    0x07, 0x00, 0x07, 0x09, 'T' , 'a' , 'T' ,
    'i' , 'c' , 'k' , 'e' , 't' , 's' };
  register_header_check(6, bin_header, sizeof(bin_header), &header_check_bin, file_stat);
}
