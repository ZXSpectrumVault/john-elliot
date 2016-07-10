AM_CPPFLAGS=-I$(top_srcdir)/765/include -I$(top_srcdir)/SdlTerm  -I$(top_srcdir)/Xbm -I$(top_srcdir)/SdSdl

bin_PROGRAMS=xjoyce xanne
noinst_LIBRARIES=libjoyce.a

if INTERNAL_LIBDSK
  MAYBE_LIBDSK = ../LibDsk/lib/libdsk.la
endif

if INTERNAL_LIB765
  MAYBE_LIB765 = ../765/lib/lib765.la
endif

xjoyce_LDADD=-L. -ljoyce ../SdlTerm/libsdlterm.la ../SdSdl/libsdsdl.la $(MAYBE_LIB765) $(MAYBE_LIBDSK) ../LibLink/lib/liblink.la
xanne_LDADD=-L.  -ljoyce ../SdlTerm/libsdlterm.la ../SdSdl/libsdsdl.la $(MAYBE_LIB765) $(MAYBE_LIBDSK) ../LibLink/lib/liblink.la
xjoyce_SOURCES=Joyce.cxx            Joyce.hxx       \
	       JoyceSystem.cxx      JoyceSystem.hxx \
	       JoyceArgs.cxx        JoyceArgs.hxx   \
	       JoyceAsic.cxx        JoyceAsic.hxx   \
	       JoyceCenPrinters.cxx JoyceCenPrinters.hxx  \
               JoyceCPS.cxx         JoyceCPS.hxx \
	       JoyceDaisy.cxx	    JoyceDaisy.hxx   \
               JoyceJoystick.cxx    JoyceJoystick.hxx \
               JoyceLocoLink.cxx    JoyceLocoLink.hxx \
	       JoyceMatrix.cxx	    JoyceMatrix.hxx   \
	       JoyceMemory.cxx      JoyceMemory.hxx \
               JoyceMenuTerm.cxx    JoyceMenuTerm.hxx \
               JoyceMenuTermPcw.cxx JoyceMenuTermPcw.hxx \
	       JoycePcwKeyboard.cxx JoycePcwKeyboard.hxx \
	       JoycePcwMouse.cxx    JoycePcwMouse.hxx \
	       JoycePcwTerm.cxx     JoycePcwTerm.hxx \
	       JoyceSetup.cxx                       \
	       JoyceFdc.cxx         JoyceFdc.hxx    \
	       JoyceZ80.cxx         JoyceZ80.hxx    \
	       Z80j.cxx             ImcZ80j.hxx

xanne_SOURCES=Anne.cxx         Anne.hxx         \
	      AnneSystem.cxx   AnneSystem.hxx   \
	      AnneAccel.cxx    AnneAccel.hxx    \
	      AnneArgs.cxx     AnneArgs.hxx     \
	      AnneAsic.cxx     AnneAsic.hxx     \
	      AnneFdc.cxx      AnneFdc.hxx   \
	      AnneMemory.cxx   AnneMemory.hxx   \
	      AnneMenuTerm.cxx AnneMenuTerm.hxx \
	      AnneMenuTermPcw.cxx AnneMenuTermPcw.hxx \
	      AnneKeyboard.cxx AnneKeyboard.hxx \
              AnnePrinter.cxx  AnnePrinter.hxx  \
	      AnneRTC.cxx      AnneRTC.hxx      \
              AnneSerial.cxx   AnneSerial.hxx   \
	      AnneSerialMouse.cxx AnneSerialMouse.hxx   \
	      AnneSetup.cxx                     \
	      AnneTerminal.cxx AnneTerminal.hxx \
	      AnneZ80.cxx      AnneZ80.hxx      \
	      DosDisk.cxx      DosDisk.hxx      \
	      DosFile.cxx      DosFile.hxx      \
	      FlashChip.cxx    FlashChip.hxx	\
	      Z80a.cxx         ImcZ80a.hxx

libjoyce_a_SOURCES=        		       \
	Executor.cxx	   Executor.hxx        \
	GdiPrint.cxx	   GdiPrint.hxx	       \
	Path.cxx           Path.hxx            \
	PcKeyboard.cxx	   PcKeyboard.hxx      \
			   Pcw.hxx	       \
        PcwArgs.cxx        PcwArgs.hxx         \
	PcwAsic.cxx        PcwAsic.hxx         \
	PcwBdos.cxx	   PcwBdos.hxx	       \
	PcwBeeper.cxx	   PcwBeeper.hxx       \
	PcwCommsW32.cxx	   PcwComms.hxx	       \
	PcwCommsUnix.cxx		       \
	PcwCommsOld.cxx			       \
	PcwCpmRedir.cxx                        \
	PcwDebugPrint.cxx                      \
	PcwDebugTerm.cxx   PcwDebugTerm.hxx    \
	PcwDevice.cxx	   PcwDevice.hxx       \
	PcwFdc.cxx         PcwFdc.hxx          \
	PcwFid.cxx         PcwFid.hxx          \
	PcwFileChooserUnix.cxx                 \
	PcwFileChooserW32.cxx                  \
			   PcwFileChooser.hxx  \
	PcwFolderChooserUnix.cxx               \
	PcwFolderChooserW32.cxx                \
			   PcwFolderChooser.hxx \
	PcwFileEntry.cxx   PcwFileEntry.hxx    \
	PcwFileList.cxx    PcwFileList.hxx     \
	PcwFiles.cxx                           \
	PcwGSX.cxx	   PcwGSX.hxx          \
	PcwHdc.cxx         PcwHdc.hxx          \
	PcwInput.cxx	   PcwInput.hxx	       \
	PcwJoystick.cxx	   PcwJoystick.hxx     \
	PcwLogoTerm.cxx    PcwLogoTerm.hxx     \
	PcwMemory.cxx      PcwMemory.hxx       \
			   PcwMenuFont.hxx     \
	PcwMenuTerm.cxx    PcwMenuTerm.hxx     \
	PcwPrinter.cxx     PcwPrinter.hxx      \
	PcwLibLink.cxx     PcwLibLink.hxx      \
	PcwSdlContext.cxx  PcwSdlContext.hxx   \
	PcwSdlGfx.cxx      PcwSdlGfx.hxx       \
	PcwSdlTerm.cxx     PcwSdlTerm.hxx      \
	PcwSystem.cxx      PcwSystem.hxx       \
	PcwTerminal.cxx	   PcwTerminal.hxx     \
	PcwUnix.cxx	   PcwUnix.hxx         \
	PcwVgaTerm.cxx	   PcwVgaTerm.hxx      \
	PcwW32.cxx         PcwW32.hxx	       \
	\
	UiCommand.cxx	   UiCommand.hxx       \
	UiContainer.cxx    UiContainer.hxx     \
	UiControl.cxx	   UiControl.hxx       \
	UiColourEntry.cxx  UiColourEntry.hxx   \
	UiLabel.cxx	   UiLabel.hxx	       \
	UiMenu.cxx 	   UiMenu.hxx	       \
	UiMenuBar.cxx      UiMenuBar.hxx       \
	UiMenuBarCommand.cxx UiMenuBarCommand.hxx\
	UiPicture.cxx      UiPicture.hxx       \
	UiScrollingMenu.cxx UiScrollingMenu.hxx \
	UiSeparator.cxx    UiSeparator.hxx     \
	UiSetting.cxx	   UiSetting.hxx       \
	UiTextInput.cxx    UiTextInput.hxx     \
			   UiTypes.hxx	       \
	\
	Z80.hxx      \
	Z80Dart.hxx  \
	cbops.hxx    \
	edops.hxx    \
	colours.hxx  \
	inline.hxx   \
	keyboard.hxx \
	sdldummy.cxx \
	z80ops.hxx   \
	\
	PcwSdlSurface.hxx   

