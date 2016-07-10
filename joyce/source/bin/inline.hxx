
// All inlines now moved to Z80.hxx

extern unsigned char *PCWRAM,	 /* The memory */
		     *PCWRD[4],  /* Read pointers to the four banks */
		     *PCWWR[4],  /* Write pointers to the four banks */
		     bank_lock[4]; /* Force R/W to the same bank? */




