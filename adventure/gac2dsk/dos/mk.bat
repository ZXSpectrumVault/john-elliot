del gacread.c
del gacread.h
del gac2dsk.c
del gac2tap.c
del plus3fs.c
del plus3fs.h
copy ..\gacread.c .
copy ..\gacread.h .
copy ..\gac2tap.c .
copy ..\gac2dsk.c .
copy ..\plus3fs.c .
copy ..\plus3fs.h .
pacc -Bl gac2tap.c gacread.c 
pacc -Bl gac2dsk.c gacread.c plus3fs.c libdsk.lib
