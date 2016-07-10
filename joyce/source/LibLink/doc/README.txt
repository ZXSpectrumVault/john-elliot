

LibLink v0.0.1

John Elliott

Abstract

LibLink is a library intended to support emulation of hardware
that is used to communicate with peripherals or other computers.
This version contains support for parallel ports and LocoLink.

Table of Contents

1 Introduction
    1.1 Unidirectional and bidirectional
    1.2 LocoLink
2 Driver types
    2.1 Output to a file or pipeline
    2.2 Direct parallel port access 
    2.3 Communication using sockets
3 Function reference
    3.1 ll_open
    3.2 ll_close
    3.3 ll_flush
    3.4 ll_rctl_poll, ll_rctl_wait
    3.5 ll_recv_poll, ll_recv_wait
    3.6 ll_sctl
    3.7 ll_send
    3.8 ll_set_cable, ll_get_cable
    3.9 ll_lrecv_poll, ll_lrecv_wait, ll_lsend
    3.10 ll_strerror



1 Introduction

LibLink provides a general set of operations for emulating
one or more parallel ports (this may be expanded in future
to cover other hardware, such as serial ports). Each emulated
parallel port can be mapped to:

* A disc file; 

* A pipe to another process; 

* The computer's parallel port (requires Linux 2.4 or later); 

* A Unix domain socket.

1.1 Unidirectional and bidirectional

LibLink only emulates unidirectional parallel ports. It has
no bidirectional capability. However, the socket driver
("parsocket") can emulate both ends of a unidirectional
connection; so it can be used to write a program that emulates
a printer. This obviously involves sending signals in both
directions, but that doesn't make it bidirectional as the
term is usually understood.

1.2 <locolink>LocoLink

LocoLink is a system by which the expansion port of an Amstrad
PCW computer (the "slave")
can be connected to the parallel port of another computer
(the "master"). The two then exchange data
using four wires of the parallel cable (two in each direction).

Now, if the LocoLink cable and interface are replaced with
a LapLink cable, then any computer with a parallel port
can be either a "master"
or a "slave". Because of the pinout of a
LapLink cable, the "master"
and "slave" ends have to set different data
lines, and different control lines; the master uses DATA
0, DATA 1, BUSY and ACK, while the slave uses DATA 3, DATA
4, SELECT and ERROR. Thus the LocoLink functions have to
distinguish whether they are acting in a "master"
or "slave" role. 

2 Driver types

2.1 Output to a file or pipeline

The simplest form of printer emulation is to send the output
to a file, or through a pipe to another program. To do this,
call ll_open() with the driver name of "parfile"
or "parpipe". The filename parameter is, respectively,
the filename to append to or the program name through which
data should be piped.

The Centronics emulation for these two devices is fairly
basic. When the STROBE signal goes from low to high (or
is it high to low?) the current data byte is appended to
the file or pipe.

2.2 Direct parallel port access 

Direct access to the parallel port requires Linux 2.4 with
a kernel that supports user-space parallel port device drivers
(CONFIG_PPDEV). To use direct access, call ll_open() with
the driver name of "parport".
The filename must be for a parallel port device such as
"/dev/parport0". If the device is a file or
otherwise unsuitable, the error LL_E_NOTDEV will be returned.

The direct parallel port driver supports all data and control
lines.

2.3 <socketcomms>Communication using sockets

LibLink supports communication with other instances of LibLink
using UNIX domain sockets or TCP/IP sockets. In theory you
could implement LocoLink over the Internet; but security
considerations make this a most unwise thing to do! 

A parallel connection can be client-to-server (eg: computer
to printer; the printer acts as a server) or peer-to-peer
(eg: one computer to another). Sockets are always client/server,
so you (or the ultimate user) have to decide which end of
a peer-to-peer connection will be the client, and which
the server.

To open a socket connection, call ll_open() with the driver
name of "parsocket". The filename consists of three fields,
separated by colons. For example:

"unix:S:/tmp/mysocket" or "tcp:C:localhost:1234"

The first field ("unix") specifies that this is a UNIX domain
socket. It could also be "tcp"
for a TCP/IP socket.

The second field can either be "S" for a server, or "C" for
a client. In server mode, a new socket is created that waits
for connections; while in client mode, an attempt is made
to connect to an existing socket.

The third field is the name of the socket to open. For a
UNIX domain socket this is a filename; for a TCP/IP socket,
it is of the form "hostname"
or "hostname:port". If no port number
is supplied the default value of 8256 will be used.

Opening a socket in server mode will create a new socket,
while in client mode the system will attempt to connect
to the socket the server has created.

3 Function reference

3.1 ll_open

ll_error_t ll_open(LL_PDEV *self, const char *filename, const
char *devtype);

This function opens an emulated parallel port. 

self: a pointer to an LL_PDEV variable, which will serve
  as a handle to the requested port.

filename: is the name of the port to open. The exact meaning
  of this string depends on what type of port is being opened.
  It can be one of:

  "parfile" - Emulate a parallel port by appending
    to a text file. "filename"
    is the name of the file to which the data will be appended.

  "parpipe" - Data will be sent to a program
    through a Unix pipeline. "filename"
    is the name of the program that will process the output.
    For example, if the filename is "/usr/bin/lpr",
    output will be sent to the default printer queue.

  "parport" - On Linux 2.4, opens the specified
    parallel port device for raw access. "filename"
    must be "/dev/parport0",
    "/dev/parport1", etc.

  "parsocket" - Emulate a parallel port using
    Unix domain sockets or TCP/IP sockets. For the full
    syntax of the filename to use, see section [socketcomms].

3.2 ll_close

ll_error_t ll_close(LL_PDEV *self);

Closes and releases a parallel port. 

self: must point to an opened LibLink handle, which will
  be set to NULL once it has been closed.

3.3 ll_flush

ll_error_t ll_flush(LL_PDEV self);

Some of the emulated ports store bytes in an internal buffer;
for example, the "parfile"
driver uses the buffered file I/O functions of the standard
I/O library. Call this function to ensure that all bytes
have been written.

self: an open LibLink connection.

3.4 <ll_rctl_poll>ll_rctl_poll, ll_rctl_wait

ll_error_t ll_rctl_poll(LL_PDEV self, unsigned  *value);

ll_error_t ll_rctl_wait(LL_PDEV self, unsigned  *value);

These functions both attempt to read the parallel port's
control lines, and return their current values in 'value'.
The difference between them is that ll_rctl_poll() always
returns immediately, while ll_rctl_wait() waits until "something
interesting" happens. The obvious interesting
thing is that the control lines change; but the driver might
find something else interesting; for example, the "llparsocket"
driver finds it equally interesting if the data lines change. 

self: an open LibLink connection. 

value: points to an unsigned integer that will receive
  the state of the control lines.

The data signals that you can test for are:

#define LL_CTL_BUSY     256     /* BUSY */

#define LL_CTL_ACK      512     /* ACK */

#define LL_CTL_NOPAPER 1024     /* NO PAPER */

#define LL_CTL_ISELECT 2048     /* SELECT */

#define LL_CTL_ERROR   4096     /* ERROR */

In addition, if you are implenting the 'printer' end of a
connection, you may receive these signals sent by the 'computer'
end:

#define LL_CTL_STROBE     1     /* STROBE */

#define LL_CTL_AUFEED     2     /* AUTO FEED XT */

#define LL_CTL_OSELECT    4     /* SELECT */

#define LL_CTL_INIT       8     /* INIT */

3.5 ll_recv_poll, ll_recv_wait

ll_error_t ll_recv_poll(LL_PDEV self, unsigned char *value);

ll_error_t ll_recv_wait(LL_PDEV self, unsigned char *value);

These functions attempt to read the data lines of the parallel
port. They are only implemented in the "parsocket"
driver, for an emulated printer to read the bytes it is
being sent. The other three drivers return LL_E_NOTIMPL.

self: an open LibLink connection. 

value: points to a byte that will receive the state of
  the data lines. 

3.6 ll_sctl

ll_error_t ll_sctl(LL_PDEV self, unsigned ctl);

Set the control lines. The bits in the "ctl"
byte are defined in section [ll_rctl_poll].

self: an open LibLink connection. 

ctl: The values to which the control lines are to be set.

3.7 ll_send

ll_error_t ll_send(LL_PDEV self, unsigned char data);

Set the data lines. 

self: an open LibLink connection. 

data: The values to which the data lines are to be set.

3.8 ll_set_cable, ll_get_cable

ll_error_t ll_set_cable(LL_PDEV self, LL_CABLE cable);

ll_error_t ll_get_cable(LL_PDEV self, LL_CABLE *cable);

These functions only really make sense on a "parsocket"
connection, although they can be called for all connection
types. What they do is set the type of cable that connects
the two ends. On a "parsocket"
connection, setting the cable type sets it for both ends.

Parameters are:

self: an open LibLink connection. It only makes sense to
  do this for the "parsocket"
  driver; the "parport"
  driver uses a real cable rather than an emulated one,
  and it isn't possible to have a meaningful LapLink conversation
  with a file or pipeline.

cable: LL_CABLE_NORMAL for a normal cable, or LL_CABLE_LAPLINK
  for a LapLink crossover cable.

3.9 ll_lrecv_poll, ll_lrecv_wait, ll_lsend

ll_error_t ll_lrecv_poll(LL_PDEV self, int master, unsigned
char *v);

ll_error_t ll_lrecv_wait(LL_PDEV self, int master, unsigned
char *v);

ll_error_t ll_lsend(LL_PDEV self, int master, unsigned char
v);

These functions are helper functions, which convert the two-bit
numbers used in LocoLink conversations to/from the combinations
of data and control line signals used by an actual LocoLink
connection. The parameters for this function are:

master: Pass 1 if this end of the connection is acting
  as the master, 0 if it is the slave. See section [locolink]
  for an explanation of LocoLink masters and slaves.

v: either a pointer to the byte being received, or the
  byte being sent. 

Note that these functions are only convenience functions.
It isn't necessary to use them to emulate either a LocoLink
master or a slave. 

3.10 ll_strerror

const char *ll_strerror(ll_error_t error);

Convert the error code returned by any of the other LibLink
functions into a string.

error: a LibLink error code.

LL_E_OK: No error (OK)

LL_E_UNKNOWN: Unknown error

LL_E_NOMEM: Out of memory

LL_E_BADPTR: Bad pointer passed to LibLink (usually a null
  pointer)

LL_E_SYSERR: System call failed. errno holds the error
  number.

LL_E_EXIST: File already exists (it shouldn't)

LL_E_NOMAGIC: File does not contain expected magic number

LL_E_CLOSED: The other end of a "parsocket"
  link has shut down unexpectedly.

LL_E_OPEN: Trying to open a link that's open already.

LL_E_INUSE: Device is in use by another program.

LL_E_TIMEOUT: Operation timed out.

LL_E_BADPKT: Received packet has incorrect length.

LL_E_CRC: Received packet has incorrect CRC.

LL_E_UNEXPECT: Unexpected packet type received.

LL_E_NODRIVER: Driver name passed to ll_open() not recognised.

LL_E_NOTIMPL: The driver does not implement this function.

LL_E_NOTDEV: The "parport"
  driver is has been asked to open something that isn't
  a parallel port.

LL_E_BADNAME: Bad filename

LL_E_PASSWORD: Incorrect or missing password.

LL_E_AGAIN: Non-blocking I/O not completed.

LL_E_SOCKERR: Error looking up an Internet hostname.
