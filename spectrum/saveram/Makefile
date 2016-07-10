
ZIPFILES=svram.zsm svram.zxb testsv.bas testld.bas svram.tap ldram.tap \
	 README COPYING Makefile

all:	svram.tap ldram.tap

zip:	$(ZIPFILES)
	zip saveram.zip $(ZIPFILES)

run:	all
	xzx -ma 128 -scale 2 -geometry +100+100

svram.tap:	testsv.bas svram.zxb
	tapcat -N $@ testsv.bas svram.zxb

ldram.tap:	testld.bas svram.zxb
	tapcat -N $@ testld.bas svram.zxb

svram.zxb:	svram.bin
	specform $<

svram.bin:	svram.zsm
	z80asm -l -fb $<
