/*

    File: file_mp3.c

    Copyright (C) 1998-2008 Christophe GRENIER <grenier@cgsecurity.org>

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
#include "log.h"
#if defined(__FRAMAC__)
#include "__fc_builtin.h"
#endif

#if !defined(MAIN_mp3) && !defined(MAIN_id3)
extern const file_hint_t file_hint_mkv;
extern const file_hint_t file_hint_tiff;
#endif

static void register_header_check_mp3(file_stat_t *file_stat);
static data_check_t data_check_id3(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery);
static data_check_t data_check_mp3(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery);
static unsigned int pos_in_mem(const unsigned char *haystack, const unsigned int haystack_size, const unsigned char *needle, const unsigned int needle_size);
static unsigned int search_MMT(const unsigned char *buffer, const unsigned int i, const unsigned int buffer_size);

const file_hint_t file_hint_mp3= {
  .extension="mp3",
  .description="MP3 audio (MPEG ADTS, layer III, v1)",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=0,
  .register_header_check=&register_header_check_mp3
};


#define MPEG_V25	0
#define MPEG_V2		0x2
#define MPEG_V1		0x3
#define MPEG_L3	0x01
#define MPEG_L2	0x02
#define MPEG_L1	0x03

static const unsigned int sample_rate_table[4][4]={
  {11025, 12000,  8000, 0},	/* MPEG_V25 */
  {    0,     0,     0, 0},
  {22050, 24000, 16000, 0},	/* MPEG_V2 */
  {44100, 48000, 32000, 0}	/* MPEG_V1 */
};
static const unsigned int bit_rate_table[4][4][16]=
{
  /* MPEG_V25 */
  /* MPEG_V_INVALID */
  {
    /* MPEG_L_INVALID */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* MPEG_L3 */
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    /* MPEG_L2 */
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    /* MPEG_L1 */
    { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0}
  },
  {
    /* MPEG_L_INVALID */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* MPEG_L3 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* MPEG_L2 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* MPEG_L1 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  },
  /* MPEG_V2 */
  {
    /* MPEG_L_INVALID */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* MPEG_L3 */
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    /* MPEG_L2 */
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    /* MPEG_L1 */
    { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0}
  },
  /* MPEG_V1 */
  {
    /* MPEG_L_INVALID */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* MPEG_L3 */
    { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0},
    /* MPEG_L2 */
    { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
    /* MPEG_L1 */
    { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 294, 416, 448, 0}
  },
};

#ifndef MAIN_mp3
/*@
  @ requires buffer_size >= 10;
  @ requires \valid_read(buffer+(0..buffer_size-1));
  @ requires \valid_read(file_recovery);
  @ requires \valid(file_recovery_new);
  @ requires file_recovery_new->blocksize > 0;
  @ ensures \result == 0 || \result == 1;
  @ ensures (\result == 1) ==> (file_recovery_new->extension == file_hint_mp3.extension);
  @ ensures (\result == 1) ==> (file_recovery_new->calculated_file_size > 0);
  @ ensures (\result == 1) ==> (file_recovery_new->file_size == 0);
  @ ensures (\result == 1) ==> (file_recovery_new->min_filesize == 287);
  @ ensures (\result == 1) ==> (file_recovery_new->file_check == &file_check_size);
  @ ensures (\result == 1) ==> (file_recovery_new->data_check == &data_check_id3);
  @*/
static int header_check_id3(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  if(buffer[0]=='I' && buffer[1]=='D' && buffer[2]=='3' && (buffer[3]==2 || buffer[3]==3 || buffer[3]==4) && buffer[4]==0)
  {
    unsigned int potential_frame_offset=0;
    /*
     * TODO Handle ID3 tag
     * http://www.id3.org/id3v2-00
     * http://www.id3.org/id3v2.3.0
     */
    if(buffer[3]==4 && (buffer[5]&0x10)==0x10) /* a footer is present http://www.id3.org/id3v2.4.0-structure chap. 3.1 */
      potential_frame_offset = 10;

    potential_frame_offset+=((buffer[6]&0x7f)<<21) + ((buffer[7]&0x7f)<<14)
      + ((buffer[8]&0x7f)<<7) + (buffer[9]&0x7f)+ 10;

    /*
       log_info("ID3v2.%u found \n potential_frame_offset at 0x%x\n",buffer[3], potential_frame_offset);
     */
    reset_file_recovery(file_recovery_new);
    file_recovery_new->calculated_file_size=potential_frame_offset;
    /*@ assert file_recovery_new->calculated_file_size > 0; */
    file_recovery_new->min_filesize=287;
    file_recovery_new->data_check=&data_check_id3;
    file_recovery_new->extension=file_hint_mp3.extension;
    file_recovery_new->file_check=&file_check_size;
    return 1;
  }
  return 0;
}
#endif

#ifndef MAIN_id3
/*@
  @ requires buffer_size >= 6;
  @ requires \valid_read(buffer+(0..buffer_size-1));
  @ requires \valid_read(file_recovery);
  @ requires \valid(file_recovery_new);
  @ requires file_recovery_new->blocksize > 0;
  @ ensures \result == 0 || \result == 1;
  @ ensures (\result == 1) ==> (file_recovery_new->extension == file_hint_mp3.extension);
  @ ensures (\result == 1) ==> (file_recovery_new->calculated_file_size > 0);
  @ ensures (\result == 1) ==> (file_recovery_new->file_size == 0);
  @ ensures (\result == 1) ==> (file_recovery_new->min_filesize == 287);
  @ ensures (\result == 1 && file_recovery_new->blocksize >= 16) ==> (file_recovery_new->file_check == &file_check_size);
  @ ensures (\result == 1 && file_recovery_new->blocksize >= 16) ==> (file_recovery_new->data_check == &data_check_mp3);
  @ ensures (\result == 1 && file_recovery_new->blocksize < 16) ==> (file_recovery_new->file_check == \null);
  @ ensures (\result == 1 && file_recovery_new->blocksize < 16) ==> (file_recovery_new->data_check == \null);
  @*/
static int header_check_mp3(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  unsigned int potential_frame_offset=0;
  unsigned int nbr=0;
  /*
     	A Frame sync  				11 (length in bits)
    	B MPEG audio version (MPEG-1, 2, etc.) 	2
    	C MPEG layer (Layer I, II, III, etc.) 	2
    	D Protection (if on, then checksum follows header) 	1
	AAAA AAAA AAAB BCCD
	1111 1111 1111 1010 = FA = MPEG-1 layer 3
	1111 1111 1111 0010 = F2 = MPEG-2 layer 3
	1111 1111 1110 0010 = E2 = MPEG-2.5 layer 3

	http://www.dv.co.yu/mpgscript/mpeghdr.htm
  */
  if(!(buffer[0]==0xFF &&
	((buffer[1]&0xFE)==0xFA ||
	 (buffer[1]&0xFE)==0xF2 ||
	 (buffer[1]&0xFE)==0xE2)))
    return 0;
  if(file_recovery->file_stat!=NULL)
  {
    if(file_recovery->file_stat->file_hint==&file_hint_mp3
#if !defined(MAIN_mp3) && !defined(MAIN_id3)
      || file_recovery->file_stat->file_hint==&file_hint_mkv
#endif
      )
    {
      header_ignored(file_recovery_new);
      return 0;
    }
#if !defined(MAIN_mp3) && !defined(MAIN_id3)
    /* RGV values from TIFF may be similar to the beginning of an mp3 */
    if(file_recovery->file_stat->file_hint==&file_hint_tiff &&
	buffer[0]==buffer[3] && buffer[1]==buffer[4] && buffer[2]==buffer[5])
    {
      if(header_ignored_adv(file_recovery, file_recovery_new)==0)
	return 0;
    }
#endif
  }
  /*@ assert nbr == 0; */
  /*@
    @ loop invariant 0 <= nbr <= potential_frame_offset <= 2048 + 8065;
    @*/
  while(potential_frame_offset+1 < buffer_size &&
      potential_frame_offset+1 < 2048)
  {
    if(buffer[potential_frame_offset+0]!=0xFF)
      return 0;
    {
      const unsigned int mpeg_version	=(buffer[potential_frame_offset+1]>>3)&0x03;
      const unsigned int mpeg_layer	=(buffer[potential_frame_offset+1]>>1)&0x03;
      const unsigned int bit_rate_key	=(buffer[potential_frame_offset+2]>>4)&0x0F;
      const unsigned int sampling_rate_key=(buffer[potential_frame_offset+2]>>2)&0x03;
      const unsigned int padding	=(buffer[potential_frame_offset+2]>>1)&0x01;
      const unsigned int bit_rate	=bit_rate_table[mpeg_version][mpeg_layer][bit_rate_key];
      const unsigned int sample_rate	=sample_rate_table[mpeg_version][sampling_rate_key];
      unsigned int frameLengthInBytes=0;
      if(sample_rate==0 || bit_rate==0 || mpeg_layer==MPEG_L1)
	return 0;
      /*@ assert 8 <= bit_rate <= 448; */
      /*@ assert 8000 <= sample_rate <= 48000; */
      if(mpeg_layer==MPEG_L3)
      {
	if(mpeg_version==MPEG_V1)
	  frameLengthInBytes = 144000 * bit_rate / sample_rate + padding;
	else
	  frameLengthInBytes = 72000 * bit_rate / sample_rate + padding;
      }
      else if(mpeg_layer==MPEG_L2)
	frameLengthInBytes = 144000 * bit_rate / sample_rate + padding;
      else
	frameLengthInBytes = (12000 * bit_rate / sample_rate + padding)*4;
#ifdef DEBUG_MP3
      log_info("framesize: %u, layer: %u, bitrate: %u, padding: %u\n",
	  frameLengthInBytes, 4-mpeg_layer, bit_rate, padding);
#endif
      if(frameLengthInBytes==0)
	return 0;
      /*@ assert 0 < frameLengthInBytes <= 8065; */
      potential_frame_offset+=frameLengthInBytes;
      /*@ assert potential_frame_offset > 0; */
      nbr++;
    }
  }
  if(nbr>1)
  {
    /*@ assert nbr > 1; */
    /*@ assert potential_frame_offset > 0; */
#ifdef DEBUG_MP3
    log_info("header_check_mp3 mp3 found\n");
#endif
    reset_file_recovery(file_recovery_new);
    /*@ assert file_recovery_new->file_check == \null; */
    /*@ assert file_recovery_new->data_check == \null; */
    file_recovery_new->calculated_file_size=potential_frame_offset;
    /*@ assert file_recovery_new->calculated_file_size > 0; */
    file_recovery_new->min_filesize=287;
    file_recovery_new->extension=file_hint_mp3.extension;
    if(file_recovery_new->blocksize >= 16)
    {
      file_recovery_new->data_check=&data_check_mp3;
      file_recovery_new->file_check=&file_check_size;
    }
    return 1;
  }
  return 0;
}
#endif

#ifndef MAIN_mp3
/*@
  @ requires buffer_size >= 32;
  @ requires \valid_read(buffer+(0..buffer_size-1));
  @ requires \valid(file_recovery);
  @ requires file_recovery->data_check==&data_check_id3;
  @ ensures \result == DC_CONTINUE || \result == DC_STOP;
  @ ensures \result == DC_CONTINUE && file_recovery->data_check==&data_check_id3 ==> (file_recovery->calculated_file_size >= file_recovery->file_size + buffer_size/2 - 1);
  @ ensures \result == DC_CONTINUE ==> (file_recovery->calculated_file_size >= file_recovery->file_size + buffer_size/2 - 16);
  @*/
static data_check_t data_check_id3(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery)
{
  while(file_recovery->calculated_file_size + buffer_size/2  >= file_recovery->file_size &&
      file_recovery->calculated_file_size + 1 < file_recovery->file_size + buffer_size/2)
  {
    const unsigned int i=file_recovery->calculated_file_size + buffer_size/2 - file_recovery->file_size;
    /*@ assert 0 <= i < buffer_size - 1 ; */
    if(buffer[i]==0)
    { /* Padding is present */
      file_recovery->calculated_file_size++;
    }
    else
    { /* no more padding or no padding */
      file_recovery->data_check=&data_check_mp3;
      file_recovery->file_check=&file_check_size;
      if(data_check_mp3(buffer, buffer_size, file_recovery)!=DC_CONTINUE)
	return DC_STOP;
      /*@ assert file_recovery->data_check==&data_check_mp3; */
      /*@ assert file_recovery->calculated_file_size >= file_recovery->file_size + buffer_size/2 - 16; */
      return DC_CONTINUE;
    }
  }
  /*@ assert file_recovery->calculated_file_size < file_recovery->file_size - buffer_size/2 || file_recovery->calculated_file_size >= file_recovery->file_size + buffer_size/2 - 1; */
  /*@ assert file_recovery->calculated_file_size >= file_recovery->file_size + buffer_size/2 - 1; */
  return DC_CONTINUE;
}
#endif

/*@
  @ requires buffer_size >= 32;
  @ requires \valid_read(buffer+(0..buffer_size-1));
  @ requires \valid(file_recovery);
  @ requires file_recovery->data_check==&data_check_mp3;
  @ requires file_recovery->file_size == 0 || file_recovery->calculated_file_size >= file_recovery->file_size - 16;
  @ ensures \result == DC_CONTINUE || \result == DC_STOP;
  @ ensures \result == DC_CONTINUE ==> (file_recovery->calculated_file_size >= file_recovery->file_size + buffer_size/2 - 16);
  @*/
  /* TODO: assigns file_recovery->calculated_file_size; */
static data_check_t data_check_mp3(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery)
{
#ifdef DEBUG_MP3
  log_info("data_check_mp3  file_size=%llu, calculated_file_size=%llu\n",
      (long long unsigned)file_recovery->file_size,
      (long long unsigned)file_recovery->calculated_file_size);
#endif
  while(file_recovery->calculated_file_size + buffer_size/2  >= file_recovery->file_size &&
      file_recovery->calculated_file_size + 16 < file_recovery->file_size + buffer_size/2)
  {
    const unsigned int i=file_recovery->calculated_file_size + buffer_size/2 - file_recovery->file_size;
    /*@ assert 0 <= i < buffer_size - 16 ; */
#ifdef DEBUG_MP3
    log_info("data_check_mp3 start i=0x%x buffer_size=0x%x calculated_file_size=%lu file_size=%lu\n",
	i, buffer_size,
	(long unsigned)file_recovery->calculated_file_size,
	(long unsigned)file_recovery->file_size);
#endif
    if(buffer[i+0]==0xFF && ((buffer[i+1]&0xE0)==0xE0))
    {
      const unsigned int mpeg_version	=(buffer[i+1]>>3)&0x03;
      const unsigned int mpeg_layer	=(buffer[i+1]>>1)&0x03;
      const unsigned int bit_rate_key	=(buffer[i+2]>>4)&0x0F;
      const unsigned int sampling_rate_key=(buffer[i+2]>>2)&0x03;
      const unsigned int padding	=(buffer[i+2]>>1)&0x01;
      const unsigned int bit_rate	=bit_rate_table[mpeg_version][mpeg_layer][bit_rate_key];
      const unsigned int sample_rate	=sample_rate_table[mpeg_version][sampling_rate_key];
      unsigned int frameLengthInBytes=0;
      /*
      log_info("frame_offset=%u\n",i);
      log_info("bit_rate=%u\n",bit_rate);
      log_info("sample_rate=%u\n",sample_rate);
      log_info("frameLengthInBytes=%u\n",frameLengthInBytes);
      */
      if(sample_rate==0 || bit_rate==0 || mpeg_layer==MPEG_L1)
	return DC_STOP;
      /*@ assert 8 <= bit_rate <= 448; */
      /*@ assert 8000 <= sample_rate <= 48000; */
      if(mpeg_layer==MPEG_L3)
      {
	if(mpeg_version==MPEG_V1)
	  frameLengthInBytes = 144000 * bit_rate / sample_rate + padding;
	else
	  frameLengthInBytes = 72000 * bit_rate / sample_rate + padding;
      }
      else if(mpeg_layer==MPEG_L2)
	frameLengthInBytes = 144000 * bit_rate / sample_rate + padding;
      else
	frameLengthInBytes = (12000 * bit_rate / sample_rate + padding)*4;
      if(frameLengthInBytes<3)
	return DC_STOP;
      /*@ assert 3 < frameLengthInBytes <= 8065; */
      file_recovery->calculated_file_size+=frameLengthInBytes;
      /*@ assert file_recovery->calculated_file_size > 0; */
    }
    else if(buffer[i]=='L' && buffer[i+1]=='Y' && buffer[i+2]=='R' && buffer[i+3]=='I' && buffer[i+4]=='C' && buffer[i+5]=='S' && buffer[i+6]=='B' && buffer[i+7]=='E' && buffer[i+8]=='G' && buffer[i+9]=='I' && buffer[i+10]=='N')
    {
      /*
	 Lyrics3 tag		http://www.id3.org/Lyrics3
	 Lyrics3 v2.00 tag	http://www.id3.org/Lyrics3v2
	 Lyrics Header :
	 'LYRICSBEGIN' (both version)
	 Lyrics Footer :
	 'LYRICSEND'  Lyrics3 tags
	 'LYRICS200'  Lyrics3v2 Tags
	 The maximum length of the lyrics is 5100 bytes for Lyrics3 and 4096 bytes for Lyrics3 v2.
       */
      unsigned int pos_lyrics=0;
      if(i + 5100 > buffer_size)
	return DC_STOP;
      /*@ assert i + 5100 <= buffer_size; */
      if((pos_lyrics=pos_in_mem(&buffer[i], 4096, (const unsigned char*)"LYRICS200", 9)) != 0)
      {
	/*@ assert pos_lyrics > 0; */
	file_recovery->calculated_file_size+=pos_lyrics;
	/*@ assert file_recovery->calculated_file_size > 0; */
      }
      else if((pos_lyrics=pos_in_mem(&buffer[i], 5100, (const unsigned char*)"LYRICSEND", 9)) != 0)
      {
	/*@ assert pos_lyrics > 0; */
	file_recovery->calculated_file_size+=pos_lyrics;
	/*@ assert file_recovery->calculated_file_size > 0; */
      }
      else
      {
	/*
	   log_warning("End of Lyrics not found \n");
	 */
	return DC_STOP;
      }
    }
    else if(buffer[i]=='A' && buffer[i+1]=='P' && buffer[i+2]=='E' && buffer[i+3]=='T' && buffer[i+4]=='A' && buffer[i+5]=='G' && buffer[i+6]=='E' && buffer[i+7]=='X')
    { /* APE Tagv2 (APE Tagv1 has no header) http://wiki.hydrogenaudio.org/index.php?title=APE_Tags_Header */
      const uint64_t ape_tag_size = (buffer[i+12] | (buffer[i+13]<<8) | (buffer[i+14]<<16) | ((uint64_t)buffer[i+15]<<24))+(uint64_t)32;
      file_recovery->calculated_file_size+=ape_tag_size;
      /*@ assert file_recovery->calculated_file_size > 0; */
    }
    else if(buffer[i]=='T' && buffer[i+1]=='A' && buffer[i+2]=='G')
    { /* http://www.id3.org/ID3v1 TAGv1 size = 128 bytes with header "TAG" */
      file_recovery->calculated_file_size+=128;
      /*@ assert file_recovery->calculated_file_size > 0; */
    }
    else
    {
      const unsigned int MMT_size=search_MMT(buffer,i,buffer_size);
      if(MMT_size==0)
	return DC_STOP;
      /*@ assert MMT_size > 0; */
      /*
	 log_info("MusicMatch Tag found at offset 0x%x with size 0x%x \n", file_recovery->calculated_file_size, MMT_size);
	 */
      file_recovery->calculated_file_size+=MMT_size;
      /*@ assert file_recovery->calculated_file_size > 0; */
    }
  }
  /*@ assert file_recovery->calculated_file_size < file_recovery->file_size - buffer_size/2 || file_recovery->calculated_file_size >= file_recovery->file_size + buffer_size/2 - 16; */
  /*@ assert file_recovery->calculated_file_size >= file_recovery->file_size + buffer_size/2 - 16; */
  return DC_CONTINUE;
}

/*@
  @ requires needle_size > 0;
  @ requires haystack_size > 0;
  @ requires \valid_read(needle+(0..needle_size-1));
  @ requires \valid_read(haystack+(0..haystack_size-1));
  @ ensures \result == 0 || needle_size <= \result <= haystack_size;
  @ assigns \nothing;
  @*/
static unsigned int pos_in_mem(const unsigned char *haystack, const unsigned int haystack_size, const unsigned char *needle, const unsigned int needle_size)
{
  unsigned int i;
  if(haystack_size < needle_size)
    return 0;
  /*@ assert haystack_size >= needle_size; */
  /*@
    @ loop assigns i;
    @ loop invariant 0 <= i <= haystack_size - needle_size + 1;
    @ loop variant haystack_size - needle_size - i;
    @*/
  for(i=0; i <= haystack_size - needle_size; i++)
    if(memcmp(&haystack[i],needle,needle_size)==0)
      return (i+needle_size);
  return 0;
}

/*@
  @ requires buffer_size > 0;
  @ requires i <= buffer_size;
  @ requires \valid_read(buffer+(0..buffer_size-1));
  @ assigns \nothing;
  @*/
static unsigned int search_MMT(const unsigned char *buffer, const unsigned int i, const unsigned int buffer_size)
{
  /*
     Check for MusicMatch Tag
	http://freenet-homepage.de/StefanRypalla/stuff/musicmatch.txt
	min size = 8192bytes
	header is optional
	structure :
	header				256 bytes optional
	image extension	4 bytes
	image binary		>= 4 bytes
	unused				4 bytes
	version info		256 bytes
	audio meta-data	>= 7868 bytes
	In all versions of the MusicMatch format up to and including 3.00,
	this section (audio meta-data) is always 7868 bytes in length.
	All subsequent versions allowed three possible lengths for this section: 7936, 8004, and 8132 bytes.
	data offsets		20 bytes
	Footer				48 bytes (optional?!)
   */
  const unsigned char mm_header[10]= {'1','8','2','7','3','6','4','5',0x00, 0x00};
  const unsigned char mm_pad_version_info[14] = {0x00,0x00,0x00,0x00,'1','8','2','7','3','6','4','5',0x00,0x00};
  const char mm_footer[]="Brava Software Inc.";
  const char mm_footer_tag[]="TAG";
  unsigned int size=0;
  if(i+sizeof(mm_header)>buffer_size)
    return 0;
  /*@ assert i + sizeof(mm_header) <= buffer_size; */
  if(memcmp(&buffer[i],mm_header,sizeof(mm_header))==0)	// Optional Header
  {
    size=256;
    /* Don't check image extension */
    /* log_info("search_MMT: mm_header present\n"); */
  }
  else
  {
    /* Check image extension */
    if( memcmp(&buffer[i],"    ",4)!=0 &&
	memcmp(&buffer[i],"bmp ",4)!=0 &&
	memcmp(&buffer[i],"jpg ",4)!=0)
      return 0;
    /* log_info("search_MMT: image extension present\n"); */
  }
  {
    const unsigned int tmp=i+size;
    const uint32_t *image_size_ptr;
    uint32_t image_size;
    if(tmp+8>buffer_size)
      return 0;
    /*@ assert tmp + 8 <= buffer_size; */
    image_size_ptr = (const uint32_t *)&buffer[tmp+4];
    image_size = le32(*image_size_ptr);
    /* Check if the image size */
    if(image_size > 10 * 1024 * 1024)
      return 0;
    /*@ assert image_size <= 10 * 1024 * 1024; */
    /* Image binary */
    size+=8+image_size;
  }
  {
    const unsigned int tmp=i+size;
    /* check null padding + version_info */
    if(tmp+sizeof(mm_pad_version_info)>buffer_size)
    { /* FIXME: Is it better to have a partial MusicMatch Tag or none ? */
      /* log_trace("search_MMT: partial MusicMatch Tag 1\n"); */
      return 0;
    }
    /*@ assert tmp + sizeof(mm_pad_version_info) <= buffer_size; */
    if(memcmp(&buffer[tmp], mm_pad_version_info, sizeof(mm_pad_version_info))!=0)
    {
      /* log_trace("search_MMT: mm_pad_version_info not present\n"); */
      return 0;
    }
  }
  size+=4+256;	/* padding + version_info */
  size+=20;	/* data offset */
  {
    const unsigned int tmp=i+size;
    /* check footer for various audio meta-data size: 7868, 7936, 8004, 8132 */
    if(tmp+8132+sizeof(mm_footer) > buffer_size)
    { /* FIXME: Is it better to have a partial MusicMatch Tag or none ? */
      /* log_trace("search_MMT: partial MusicMatch 2\n"); */
      return 0;
    }
    /*@ assert tmp + 8132 + sizeof(mm_footer) <= buffer_size; */
    if( memcmp(&buffer[tmp+7868], mm_footer, sizeof(mm_footer)-1)==0 ||
	memcmp(&buffer[tmp+7868], mm_footer_tag, sizeof(mm_footer_tag) - 1)==0)
      size+=7868;
    else if(memcmp(&buffer[tmp+7936], mm_footer, sizeof(mm_footer)-1)==0 ||
	memcmp(&buffer[tmp+7936], mm_footer_tag, sizeof(mm_footer_tag) - 1)==0)
      size+=7936;
    else if(memcmp(&buffer[tmp+8004], mm_footer, sizeof(mm_footer)-1)==0 ||
	memcmp(&buffer[tmp+8004], mm_footer_tag, sizeof(mm_footer_tag) - 1)==0)
      size+=8004;
    else if(memcmp(&buffer[tmp+8132], mm_footer, sizeof(mm_footer)-1)==0 ||
	memcmp(&buffer[tmp+8132], mm_footer_tag, sizeof(mm_footer_tag)-1)==0)
      size+=8132;
    else
    {
      /* log_trace("search_MMT: no mm_footer present\n"); */
      return 0;
    }
  }
  {
    const unsigned int tmp=i+size;
    if(tmp + sizeof(mm_footer) > buffer_size)
      return 0;
    /*@ assert tmp + sizeof(mm_footer) <= buffer_size; */
    /* dump_log(&buffer[tmp], 16); */
    if(memcmp(&buffer[tmp],mm_footer, sizeof(mm_footer)-1)==0)
      size+=48;	/* footer */
    else
      size+=0x80;	/* TAG footer */
  }
  /* log_trace("search_MMT: MMT found size=%u (0x%x)\n", size, size); */
  return(size);
}

static void register_header_check_mp3(file_stat_t *file_stat)
{
  static const unsigned char mpeg1_L3_header1[2]= {0xFF, 0xFA};
  static const unsigned char mpeg1_L3_header2[2]= {0xFF, 0xFB};
  static const unsigned char mpeg2_L3_header1[2]= {0xFF, 0xF2};
  static const unsigned char mpeg2_L3_header2[2]= {0xFF, 0xF3};
  static const unsigned char mpeg25_L3_header1[2]={0xFF, 0xE2};
  static const unsigned char mpeg25_L3_header2[2]={0xFF, 0xE3};
#ifndef MAIN_mp3
  register_header_check(0, "ID3", 3, &header_check_id3, file_stat);
#endif
#ifndef MAIN_id3
  register_header_check(0, mpeg1_L3_header1, sizeof(mpeg1_L3_header1), &header_check_mp3, file_stat);
  register_header_check(0, mpeg1_L3_header2, sizeof(mpeg1_L3_header2), &header_check_mp3, file_stat);
  register_header_check(0, mpeg2_L3_header1, sizeof(mpeg2_L3_header1), &header_check_mp3, file_stat);
  register_header_check(0, mpeg2_L3_header2, sizeof(mpeg2_L3_header2), &header_check_mp3, file_stat);
  register_header_check(0, mpeg25_L3_header1, sizeof(mpeg25_L3_header1), &header_check_mp3, file_stat);
  register_header_check(0, mpeg25_L3_header2, sizeof(mpeg25_L3_header2), &header_check_mp3, file_stat);
#endif
}

#ifdef MAIN_id3
#define BLOCKSIZE 65536u
int main()
{
  unsigned char buffer[BLOCKSIZE];
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

  file_stats.file_hint=&file_hint_mp3;
  file_stats.not_recovered=0;
  file_stats.recovered=0;
  file_hint_mp3.register_header_check(&file_stats);
  if(header_check_id3(buffer, BLOCKSIZE, 0u, &file_recovery, &file_recovery_new) != 1)
    return 0;
  memcpy(file_recovery_new.filename, "demo", 5);
  /*@ assert file_recovery_new.extension == file_hint_mp3.extension;	*/
  /*@ assert file_recovery_new.calculated_file_size > 0; */
  /*@ assert file_recovery_new.file_size == 0;	*/
  /*@ assert file_recovery_new.min_filesize == 287;	*/
  /*@ assert file_recovery_new.data_check == &data_check_id3; */
  file_recovery_new.file_stat=&file_stats;
  if(file_recovery_new.file_stat!=NULL && file_recovery_new.file_stat->file_hint!=NULL &&
    file_recovery_new.data_check!=NULL)
  {
    unsigned char big_buffer[2*BLOCKSIZE];
    data_check_t res_data_check=DC_CONTINUE;
    memset(big_buffer, 0, BLOCKSIZE);
    memcpy(big_buffer + BLOCKSIZE, buffer, BLOCKSIZE);
    /*@ assert file_recovery_new.file_size <= file_recovery_new.calculated_file_size; */;
    res_data_check=data_check_id3(big_buffer, 2*BLOCKSIZE, &file_recovery_new);
    /*@ assert file_recovery_new.data_check == &data_check_id3 || file_recovery_new.data_check == &data_check_mp3; */
    /*@ assert res_data_check == DC_CONTINUE && file_recovery_new.data_check == &data_check_mp3 ==> (file_recovery_new.calculated_file_size >= file_recovery_new.file_size + BLOCKSIZE - 16); */
    /*@ assert res_data_check == DC_CONTINUE && file_recovery_new.data_check == &data_check_id3 ==> (file_recovery_new.calculated_file_size >= file_recovery_new.file_size + BLOCKSIZE - 1); */
    /*@ assert res_data_check == DC_CONTINUE ==> (file_recovery_new.calculated_file_size >= file_recovery_new.file_size + BLOCKSIZE - 16); */
    file_recovery_new.file_size+=BLOCKSIZE;
    if(res_data_check == DC_CONTINUE)
    {
      /*@ assert file_recovery_new.calculated_file_size >= file_recovery_new.file_size - 16; */
      memcpy(big_buffer, big_buffer + BLOCKSIZE, BLOCKSIZE);
#if defined(__FRAMAC__)
      Frama_C_make_unknown((char *)big_buffer + BLOCKSIZE, BLOCKSIZE);
#endif
      file_recovery_new.data_check(big_buffer, 2*BLOCKSIZE, &file_recovery_new);
    }
  }
  if(file_recovery_new.file_stat!=NULL)
  {
    file_recovery_t file_recovery_new2;
    /* Test when another file of the same is detected in the next block */
    file_recovery_new2.blocksize=BLOCKSIZE;
    file_recovery_new2.file_stat=NULL;
    file_recovery_new2.file_check=NULL;
    file_recovery_new2.location.start=BLOCKSIZE;
    file_recovery_new.handle=NULL;	/* In theory should be not null */
    header_check_id3(buffer, BLOCKSIZE, 0, &file_recovery_new, &file_recovery_new2);
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
#elif defined(MAIN_mp3)
#define BLOCKSIZE 65536u
int main()
{
  unsigned char buffer[BLOCKSIZE];
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

  file_stats.file_hint=&file_hint_mp3;
  file_stats.not_recovered=0;
  file_stats.recovered=0;
  file_hint_mp3.register_header_check(&file_stats);
  if(header_check_mp3(buffer, BLOCKSIZE, 0u, &file_recovery, &file_recovery_new)!=1)
    return 0;
  memcpy(file_recovery_new.filename, "demo", 5);
  /*@ assert file_recovery_new.file_size == 0;	*/
  /*@ assert file_recovery_new.min_filesize == 287;	*/
  /*@ assert file_recovery_new.extension == file_hint_mp3.extension;	*/
  /*@ assert file_recovery_new.calculated_file_size > 0; */
  file_recovery_new.file_stat=&file_stats;
  if(file_recovery_new.file_stat!=NULL && file_recovery_new.file_stat->file_hint!=NULL &&
    file_recovery_new.data_check!=NULL)
  {
    unsigned char big_buffer[2*BLOCKSIZE];
    data_check_t res_data_check=DC_CONTINUE;
    memset(big_buffer, 0, BLOCKSIZE);
    memcpy(big_buffer + BLOCKSIZE, buffer, BLOCKSIZE);
    /*@ assert file_recovery_new.data_check == &data_check_mp3; */
    /*@ assert file_recovery_new.file_size == 0; */;
    /*@ assert file_recovery_new.file_size <= file_recovery_new.calculated_file_size; */;
    res_data_check=data_check_mp3(big_buffer, 2*BLOCKSIZE, &file_recovery_new);
    /*@ assert res_data_check == DC_CONTINUE ==> (file_recovery_new.calculated_file_size >= file_recovery_new.file_size + BLOCKSIZE - 16); */
    file_recovery_new.file_size+=BLOCKSIZE;
    if(res_data_check == DC_CONTINUE)
    {
      /*@ assert file_recovery_new.calculated_file_size >= file_recovery_new.file_size - 16; */
      memcpy(big_buffer, big_buffer + BLOCKSIZE, BLOCKSIZE);
#if defined(__FRAMAC__)
      Frama_C_make_unknown((char *)big_buffer + BLOCKSIZE, BLOCKSIZE);
#endif
      file_recovery_new.data_check(big_buffer, 2*BLOCKSIZE, &file_recovery_new);
    }
  }
  if(file_recovery_new.file_stat!=NULL)
  {
    file_recovery_t file_recovery_new2;
    /* Test when another file of the same is detected in the next block */
    file_recovery_new2.blocksize=BLOCKSIZE;
    file_recovery_new2.file_stat=NULL;
    file_recovery_new2.file_check=NULL;
    file_recovery_new2.location.start=BLOCKSIZE;
    file_recovery_new.handle=NULL;	/* In theory should be not null */
    header_check_mp3(buffer, BLOCKSIZE, 0, &file_recovery_new, &file_recovery_new2);
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
