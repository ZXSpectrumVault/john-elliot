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

#define MGTFS_ERR_DISKFULL  101
#define MGTFS_ERR_DIRFULL   102
#define MGTFS_ERR_EXISTS    103

typedef struct mgtfs_impl *MGTFS;
typedef struct mgtfile_impl *MGTFILE;

/* Convert a filename to fixed 10-char format; pass this to mgtfs_creat, 
 * mgtfs_exists or mgtfs_setlabel */

void mgtfs_makename(const char *name, char *fcbname);

dsk_err_t mgtfs_mkfs(MGTFS *fs, const char *name, const char *type, 
			const char *compression, int capacity);
dsk_err_t mgtfs_umount(MGTFS fs);

int mgtfs_exists(MGTFS fs, const char *name);
dsk_err_t mgtfs_creat(MGTFS fs, MGTFILE *pfp, const char *name);
dsk_err_t mgtfs_setlabel(MGTFS fs, const char *name);
dsk_err_t mgtfs_sync(MGTFS fs);
dsk_err_t mgtfs_flush(MGTFILE fp);
dsk_err_t mgtfs_close(MGTFILE fp);
dsk_err_t mgtfs_putc(unsigned char c, MGTFILE fp);

const char *mgtfs_strerror(int err);

