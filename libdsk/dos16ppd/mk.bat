@echo off
SET CC=pacc
SET CFLAGS=-Bl -I. -I../include -I../lib -DDOS16FLOPPY -D__MSDOS__

if "%1"=="utils" goto utils
if "%1"=="lib" goto lib


rem %CC% %CFLAGS% -c ../lib/compdskf.c
rem if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/compress.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/compsq.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/comptlzh.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/crc16.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/crctable.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvadisk.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvcfi.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvcpcem.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvdos16.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvdskf.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvimd.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvint25.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvldbs.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvlogi.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvmyz80.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvjv3.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvnwasp.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvposix.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvqm.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvqrst.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvsimh.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvtele.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/drvydsk.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskcheck.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskcmt.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskcopy.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskdirty.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskerror.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskfmt.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskgeom.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskiconv.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dsklphys.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskopen.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskpars.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskread.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskreprt.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskretry.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskrtrd.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dsksecid.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskseek.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dsksgeom.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskstat.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dsktread.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dsktrkid.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/dskwrite.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/ldbs.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/remote.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/rpccli.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/rpcfossl.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/rpcmap.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/rpcpack.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../lib/rpcserv.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c int25l.as
if errorlevel 1 goto abort
rem %CC% %CFLAGS% -c ../tools/apriboot.c
rem if errorlevel 1 goto abort

rem
rem Modules built. Combine them into a library.
rem

:lib
libr r libdsk.lib compress.obj
if errorlevel 1 goto abort
libr r libdsk.lib compsq.obj
if errorlevel 1 goto abort
libr r libdsk.lib comptlzh.obj
if errorlevel 1 goto abort
libr r libdsk.lib crc16.obj
if errorlevel 1 goto abort
libr r libdsk.lib crctable.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvadisk.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvadisk.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvcfi.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvcpcem.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvdos16.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvdskf.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvimd.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvint25.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvldbs.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvjv3.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvlogi.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvmyz80.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvnwasp.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvposix.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvqm.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvqrst.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvsimh.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvtele.obj
if errorlevel 1 goto abort
libr r libdsk.lib drvydsk.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskcheck.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskcmt.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskcopy.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskdirty.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskerror.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskfmt.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskgeom.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskiconv.obj
if errorlevel 1 goto abort
libr r libdsk.lib dsklphys.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskopen.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskpars.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskread.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskreprt.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskretry.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskrtrd.obj
if errorlevel 1 goto abort
libr r libdsk.lib dsksecid.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskseek.obj
if errorlevel 1 goto abort
libr r libdsk.lib dsksgeom.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskstat.obj
if errorlevel 1 goto abort
libr r libdsk.lib dsktread.obj
if errorlevel 1 goto abort
libr r libdsk.lib dsktrkid.obj
if errorlevel 1 goto abort
libr r libdsk.lib dskwrite.obj
if errorlevel 1 goto abort
libr r libdsk.lib int25l.obj
if errorlevel 1 goto abort
libr r libdsk.lib ldbs.obj
if errorlevel 1 goto abort
libr r libdsk.lib remote.obj
if errorlevel 1 goto abort
libr r libdsk.lib rpccli.obj
if errorlevel 1 goto abort
libr r libdsk.lib rpcfossl.obj
if errorlevel 1 goto abort
libr r libdsk.lib rpcmap.obj
if errorlevel 1 goto abort
libr r libdsk.lib rpcpack.obj
if errorlevel 1 goto abort
libr r libdsk.lib rpcserv.obj
if errorlevel 1 goto abort
rem
rem Build the utilities
rem
:utils
%CC% %CFLAGS% -c ../tools/utilopts.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../tools/formname.c
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../tools/bootsec.c
if errorlevel 1 goto abort
%CC% %CFLAGS% ../tools/dskid.c utilopts.obj libdsk.lib
if errorlevel 1 goto abort
%CC% %CFLAGS% ../tools/dskconv.c utilopts.obj formname.obj libdsk.lib
if errorlevel 1 goto abort
%CC% %CFLAGS% ../tools/dskform.c utilopts.obj formname.obj libdsk.lib
if errorlevel 1 goto abort
%CC% %CFLAGS% -c ../tools/dsktrans.c 
if errorlevel 1 goto abort
%CC% %CFLAGS% dsktrans.obj utilopts.obj formname.obj bootsec.obj libdsk.lib
if errorlevel 1 goto abort
%CC% %CFLAGS% ../tools/dskdump.c utilopts.obj formname.obj libdsk.lib
if errorlevel 1 goto abort
%CC% %CFLAGS% ../tools/dskscan.c utilopts.obj formname.obj libdsk.lib
if errorlevel 1 goto abort
%CC% %CFLAGS% ../tools/dskutil.c utilopts.obj formname.obj libdsk.lib
if errorlevel 1 goto abort


:abort

