/*

    File: geometry.h

    Copyright (C) 1998-2009 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#ifdef __cplusplus
extern "C" {
#endif
#include "filegen.h"
void set_cylinders_from_size_up(disk_t *disk);
int change_sector_size(disk_t *disk, const int cyl_modified, const unsigned int sector_size);
int change_geometry_cli(disk_t *disk, char **current_cmd);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
