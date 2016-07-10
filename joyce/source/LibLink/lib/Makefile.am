AM_CPPFLAGS=-I$(top_srcdir)/include
JAVAC=@JAVAC@
JAVAH=@JAVAH@
JAVA=@JAVA@
JAVADOC=@JAVADOC@
JAR=@JAR@
CLASSCPRE=uk.co.demon.seasip.liblink
CLASSDPRE=uk/co/demon/seasip/liblink
HEADERPRE=uk_co_demon_seasip_liblink
lib_LTLIBRARIES=liblink.la
noinst_PROGRAMS=@LLINKJAR@
EXTRA_PROGRAMS=
EXTRA_DIST=$(CLASSDPRE)/LibLink.java \
	   $(CLASSDPRE)/LibLinkException.java 

liblink_la_LDFLAGS=-version-info 1:0:0
liblink_la_SOURCES=ll.h lldrivers.inc lldrivers.h \
		   llopen.c llerror.c llflush.c \
                   llisend.c llirecv.c	\
                   llsend.c llrecv.c	\
		   lllaplink.c lllocolink.c \
		   llparfile.c llparfile.h \
		   llparpipe.c llparpipe.h \
		   llparport.c llparport.h \
		   lljni.c \
		   llparsocket.c llparsocket.h llspacket.h

#llconnect.c llopen.c llerror.c llrecv.c llsend.c \
			#lljni.c llpacket.c llmaster.c llmrpc.c \
		 	#lshm.c llnpipe.c \
			#ldrivers.h lldrivers.inc ll.h


JARCLASSES=$(CLASSDPRE)/LibLink.class \
	   $(CLASSDPRE)/LibLinkException.class 
JARHEADERS=$(HEADERPRE)_LibLink.h
CLEANFILES=$(JARCLASSES) $(JARHEADERS)

liblink.jar:     $(JARCLASSES)
	$(JAR) c0f $@ uk/*
	$(JAVADOC) -d $(top_srcdir)/doc $(CLASSCPRE)

$(CLASSDPRE)/%.class:   $(srcdir)/$(CLASSDPRE)/%.java
	here=`pwd` && cd $(srcdir) && $(JAVAC) -d $$here $<

%.class:        $(srcdir)/%.java
	here=`pwd` && cd $(srcdir) && $(JAVAC) -d $$here $<


$(HEADERPRE)_%.h:         $(CLASSDPRE)/%.class
	$(JAVAH) -jni $(CLASSCPRE).`basename $< .class`
	touch $@

$(srcdir)/lljni.c:     $(JARHEADERS)

install-exec-local:
	$(mkinstalldirs) $(libdir)
	if test x@LLINKJAR@ = x; then true; else $(INSTALL_DATA) @LLINKJAR@ $(libdir); fi

