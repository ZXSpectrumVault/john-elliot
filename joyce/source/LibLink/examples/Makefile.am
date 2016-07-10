## Process this file with automake to produce Makefile.in

AM_CPPFLAGS=-I$(top_srcdir)/include
LDADD=../lib/liblink.la

CLASSPATH=@CLASSPATH@
JAVAC=@JAVAC@ 

noinst_PROGRAMS=@EGCLASSES@ precho prhello events lltest partest \
		llmaster llslave #filelink flspy ttyspy #pkt

precho_SOURCES=precho.c
events_SOURCES=events.c utils.h utils.c
prhello_SOURCES=prhello.c
llmaster_SOURCES=llmaster.c
llslave_SOURCES=llslave.c
lltest_SOURCES=lltest.c utils.h utils.c
partest_SOURCES=partest.c utils.h utils.c 
#filelink_SOURCES=filelink.c utils.h utils.c 
#flspy_SOURCES=flspy.c utils.h utils.c 
#ttyspy_SOURCES=ttyspy.c 
#pkt_SOURCES=pkt.c

EXTRA_PROGRAMS=
EXTRA_DIST=ParTest.java #Master.java Slave.java Browse.java

CLEANFILES=*.class

%.class:        $(srcdir)/%.java
	here=`pwd` && cd $(srcdir) && $(JAVAC) -classpath $(CLASSPATH):$$here/../lib/liblink.jar -d $$here $<

