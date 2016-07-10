/************************************************************************

    TAPTOOLS v1.1.0 - Tapefile manipulation utilities

    Copyright (C) 1996, 2014  John Elliott <jce@seasip.demon.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*************************************************************************/

#define P3FS_ERR_DISKFULL  101
#define P3FS_ERR_DIRFULL   102
#define P3FS_ERR_EXISTS    103
#define P3FS_ERR_MISALIGN  104

typedef struct plus3fs_impl *PLUS3FS;
typedef struct plus3file_impl *PLUS3FILE;

/* Convert a filename to FCB 8.3 format; pass this to p3fs_creat, 
 * p3fs_exists or p3fs_setlabel */

void p3fs_83name(const char *name, char *fcbname);

dsk_err_t p3fs_mkfs(PLUS3FS *fs,
		const char *name, const char *type, const char *compression,
		unsigned char *boot_spec, int timestamped);
dsk_err_t p3fs_umount(PLUS3FS fs);

int p3fs_exists(PLUS3FS fs, int uid, char *name);
dsk_err_t p3fs_creat(PLUS3FS fs, PLUS3FILE *pfp, int uid, char *name);
dsk_err_t p3fs_setlabel(PLUS3FS fs, char *name);
dsk_err_t p3fs_sync(PLUS3FS fs);
dsk_err_t p3fs_dossync(PLUS3FS fs, int format, int dosonly);
dsk_err_t p3fs_flush(PLUS3FILE fp);
dsk_err_t p3fs_close(PLUS3FILE fp);
dsk_err_t p3fs_putc(unsigned char c, PLUS3FILE fp);

const char *p3fs_strerror(int err);

