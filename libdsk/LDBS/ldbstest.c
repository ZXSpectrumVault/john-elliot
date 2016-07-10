/* LDBS: LibDsk Block Store access functions
 *
 *  Copyright (c) 2016 John Elliott <seasip.webmaster@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE. */

/* LDBS example software: Minimal function exerciser for the block layer
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldbs.h"

/* Tests at the block layer */

int main(int argc, char **argv)
{
	char type[5];
	PLDBS ldbs = NULL;
	LDBLOCKID blkid[5];
	dsk_err_t err;
	static char test1[] = "LDBS test block 1";
	static char test2[] = "LDBS test block two";
	static char test3[] = "LDBS test block 3";
	char buf[80];
	size_t len;
	int readonly = 0;
	int n;

	for (n = 0; n < 5; n++) 
	{
		blkid[n] = LDBLOCKID_NULL;
	}
	
	err = ldbs_new(&ldbs, "test.ldbs", "TeST"); 

	printf("ldbs_new: err=%d ldbs=%p\n", err, ldbs);
	err = ldbs_close(&ldbs);
	printf("ldbs_close: err=%d ldbs=%p\n", err, ldbs);

	err = ldbs_open(&ldbs, "test.ldbs", type, &readonly);
	printf("ldbs_open: err=%d ldbs=%p type=%s\n", err, ldbs, type);


	err = ldbs_putblock(ldbs, &blkid[0], "BL0K", test1, 1 + strlen(test1)); 
	printf("ldbs_putblock: err=%d blkid=%ld\n", err, blkid[0]);
	err = ldbs_putblock(ldbs, &blkid[1], "BL0K", test2, 1 + strlen(test2)); 
	printf("ldbs_putblock: err=%d blkid=%ld\n", err, blkid[1]);
	err = ldbs_putblock(ldbs, &blkid[2], "BL0K", test3, 1 + strlen(test3)); 
	printf("ldbs_putblock: err=%d blkid=%ld\n", err, blkid[2]);

	err = ldbs_putblock(ldbs, &blkid[2], "BL0K", "Rewritten", 8);
	printf("ldbs_putblock: err=%d blkid=%ld\n", err, blkid[2]);
	err = ldbs_putblock(ldbs, &blkid[2], "BL0K", test2, strlen(test2));
	printf("ldbs_putblock: err=%d blkid=%ld\n", err, blkid[2]);

	err = ldbs_delblock(ldbs, blkid[0]);
	printf("ldbs_delblock: err=%d blkid=%ld\n", err, blkid[0]);

	err = ldbs_putblock(ldbs, &blkid[3], "BL0K", test2, 1 + strlen(test2)); 
	printf("ldbs_putblock: err=%d blkid=%ld\n", err, blkid[3]);
	err = ldbs_putblock(ldbs, &blkid[4], "BL0K", test3, 1 + strlen(test3)); 
	printf("ldbs_putblock: err=%d blkid=%ld\n", err, blkid[4]);

	len = 0;
	err = ldbs_getblock(ldbs, blkid[3], type, NULL, &len);
	printf("ldbs_getblock: err=%d type=%s len=%zu\n", err, type, len);

	memset(buf, '=', sizeof(buf));
	buf[sizeof(buf)-1] = 0;
	len = sizeof(buf);
	err = ldbs_getblock(ldbs, blkid[3], type, buf, &len);
	printf("ldbs_getblock: err=%d type=%s len=%zu buf=%s\n", 
		err, type, len, buf);

	err = ldbs_delblock(ldbs, blkid[2]);
	printf("ldbs_delblock: err=%d blkid=%ld\n", err, blkid[2]);

	ldbs_setroot(ldbs, (LDBLOCKID)4);

	err = ldbs_fsck(ldbs, stdout);
	printf("ldbs_fsck: err=%d\n", err);

	err = ldbs_close(&ldbs);
	printf("ldbs_close: err=%d ldbs=%p\n", err, ldbs);

	/* Now try a memory-backed tempfile */
	for (n = 0; n < 5; n++) 
	{
		blkid[n] = LDBLOCKID_NULL;
	}

	err = ldbs_new(&ldbs, NULL, "TeST");
	printf("ldbs_new (temp): err=%d ldbs=%p\n", err, ldbs);
	err = ldbs_putblock(ldbs, &blkid[0], "BL0K", test1, 1 + strlen(test1)); 
	printf("ldbs_addblock: err=%d blkid=%ld\n", err, blkid[0]);
	err = ldbs_putblock(ldbs, &blkid[1], "BL0K", test2, 1 + strlen(test2)); 
	printf("ldbs_addblock: err=%d blkid=%ld\n", err, blkid[1]);
	err = ldbs_putblock(ldbs, &blkid[2], "BL0K", test3, 1 + strlen(test3)); 
	printf("ldbs_addblock: err=%d blkid=%ld\n", err, blkid[2]);

	err = ldbs_delblock(ldbs, blkid[1]);
	printf("ldbs_delblock: err=%d blkid=%ld\n", err, blkid[2]);

	return 0;
}


