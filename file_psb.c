/*

    File: file_psb.c

    Copyright (C) 2006-2009,2013 Christophe GRENIER <grenier@cgsecurity.org>
  
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

#ifdef DEBUG_PSD
#include "log.h"
#endif

static void register_header_check_psb(file_stat_t *file_stat);
static data_check_t psb_skip_color_mode(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery);
static void file_check_psb(file_recovery_t *file_recovery);

const file_hint_t file_hint_psb= {
  .extension="psb",
  .description="Adobe Photoshop Image",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_psb
};

static uint64_t psb_image_data_size_max=0;

struct psb_file_header
{
  char signature[4];
  uint16_t version;	/* must be 2 */
  char reserved[6];	/* must be 0 */
  uint16_t channels;	/* between 1 and 56 */
  uint32_t height;	/* max of 300,000 */
  uint32_t width;	/* max of 300,000 */
  uint16_t depth;	/* 1, 8, 16 or 32 */
  uint16_t color_mode;	/* Bitmap = 0; Grayscale = 1; Indexed = 2; RGB = 3; CMYK = 4; Multichannel = 7; Duotone = 8; Lab = 9 */
} __attribute__ ((gcc_struct, __packed__));

static int header_check_psb(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct psb_file_header *hdr=(const struct psb_file_header *)buffer;
#ifdef DEBUG_PSD
  log_info("channels %u\n", be16(hdr->channels));
  log_info("height %u\n", be32(hdr->height));
  log_info("width  %u\n", be32(hdr->width));
  log_info("depth  %u\n", be16(hdr->depth));
  log_info("color_mode %u\n", be16(hdr->color_mode));
#endif
  if(be16(hdr->channels)==0 || be16(hdr->channels)>56 ||
      be32(hdr->height)==0 || be32(hdr->height)>300000 ||
      be32(hdr->width)==0 || be32(hdr->width)>300000 ||
      be16(hdr->depth)==0 || (be16(hdr->depth)!=1 && be16(hdr->depth)%8!=0))
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->min_filesize=70;
  file_recovery_new->extension=file_hint_psb.extension;
  if(file_recovery_new->blocksize < 16)
    return 1;
  /* File header */
  file_recovery_new->calculated_file_size=0x1a;
  file_recovery_new->data_check=&psb_skip_color_mode;
  file_recovery_new->file_check=&file_check_psb;
  return 1;
}

static uint64_t get_be64(const void *buffer, const unsigned int offset)
{
  const uint64_t *val=(const uint64_t *)((const unsigned char *)buffer+offset);
  return be64(*val);
}

static data_check_t psb_skip_image_data(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery)
{
  file_recovery->file_check=NULL;
  return DC_CONTINUE;
}

static data_check_t psb_skip_layer_info(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery)
{
  if(file_recovery->calculated_file_size + buffer_size/2  >= file_recovery->file_size &&
      file_recovery->calculated_file_size + 16 < file_recovery->file_size + buffer_size/2)
  {
    const unsigned int i=file_recovery->calculated_file_size - file_recovery->file_size + buffer_size/2;
    const unsigned int l=get_be64(buffer, i)+8;
#ifdef DEBUG_PSD
    log_info("Image data at 0x%lx\n", (long unsigned)(l + file_recovery->calculated_file_size));
#endif
    if(l<4)
      return DC_STOP;
    file_recovery->calculated_file_size+=l;
    file_recovery->data_check=&psb_skip_image_data;
    return psb_skip_image_data(buffer, buffer_size, file_recovery);
  }
  return DC_CONTINUE;
}

static data_check_t psb_skip_image_resources(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery)
{
  if(file_recovery->calculated_file_size + buffer_size/2  >= file_recovery->file_size &&
      file_recovery->calculated_file_size + 16 < file_recovery->file_size + buffer_size/2)
  {
    const unsigned int i=file_recovery->calculated_file_size - file_recovery->file_size + buffer_size/2;
    const unsigned int l=get_be64(buffer, i)+8;
#ifdef DEBUG_PSD
    log_info("Layer info at 0x%lx\n", (long unsigned)(l + file_recovery->calculated_file_size));
#endif
    if(l<4)
      return DC_STOP;
    file_recovery->calculated_file_size+=l;
    file_recovery->data_check=&psb_skip_layer_info;
    return psb_skip_layer_info(buffer, buffer_size, file_recovery);
  }
  return DC_CONTINUE;
}

static data_check_t psb_skip_color_mode(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery)
{
  const struct psb_file_header *psb=(const struct psb_file_header *)&buffer[buffer_size/2];
  psb_image_data_size_max=(uint64_t)be16(psb->channels) * be32(psb->height) * be32(psb->width) * be16(psb->depth) / 8;
#ifdef DEBUG_PSD
  log_info("psb_image_data_size_max %lu\n", (long unsigned)psb_image_data_size_max);
#endif
  if(file_recovery->calculated_file_size + buffer_size/2  >= file_recovery->file_size &&
      file_recovery->calculated_file_size + 16 < file_recovery->file_size + buffer_size/2)
  {
    const unsigned int i=file_recovery->calculated_file_size - file_recovery->file_size + buffer_size/2;
    const unsigned int l=get_be64(buffer, i)+8;
#ifdef DEBUG_PSD
    log_info("Color mode at 0x%lx\n", (long unsigned)(l + file_recovery->calculated_file_size));
#endif
    if(l<4)
      return DC_STOP;
    file_recovery->calculated_file_size+=l;
    file_recovery->data_check=&psb_skip_image_resources;
    return psb_skip_image_resources(buffer, buffer_size, file_recovery);
  }
  return DC_CONTINUE;
}

static void file_check_psb(file_recovery_t *file_recovery)
{
  if(file_recovery->file_size < file_recovery->calculated_file_size)
    file_recovery->file_size=0;
  else if(file_recovery->file_size > file_recovery->calculated_file_size + psb_image_data_size_max)
    file_recovery->file_size=file_recovery->calculated_file_size + psb_image_data_size_max;
}

static void register_header_check_psb(file_stat_t *file_stat)
{
  static const unsigned char psb_header[6]={'8', 'B', 'P', 'S', 0x00, 0x02};
  register_header_check(0, psb_header,sizeof(psb_header), &header_check_psb, file_stat);
}
