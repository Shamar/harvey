/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * Harvey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Harvey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Harvey.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <u.h>
#include <libc.h>

#include "gconsole.h"

#define WAIT_FOR(v) while(rendezvous(&v, (void*)0x12345) == (void*)~0)
#define PROVIDE(v) while(rendezvous(&v, (void*)0x11111) == (void*)~0)

int stdio;
int rawmode;
int blind;

/* read from input, write to output */
void
joint(char *name, int input, int output)
{
	int32_t pid, r, w;
	static uint8_t buf[IODATASZ];

	pid = getpid();

	debug("%s %d started\n", name, pid);
	do
	{
		w = 0;
		r = read(input, buf, IODATASZ);
		debug("%s %d: read(%d) returns %d\n", name, pid, input, r);
		if(r > 0){
			w = write(output, buf, r);
			debug("%s %d: write(%d, buf, %d) returns %d\n", name, pid, output, r, w);
		}
	}
	while(r > 0 && w == r);

	close(input);
	debug("%s %d: close(%d)\n", name, pid, input);
	close(output);
	debug("%s %d: close(%d)\n", name, pid, output);

	debug("%s %d: shut down (r = %d, w = %d)\n", name, pid, r, w);
	if(r < 0)
		exits("read");
	if(w < 0)
		exits("write");
	if(w < r)
		exits("i/o error");
	exits(nil);
}

void
fortify(char *name)
{
	int r;

	/* ensure that our services can not become evil */
	rfork(RFCENVG|RFCNAMEG|RFREND);
	if((r = bind("#p", "/proc", MREPL)) < 0)
		sysfatal("%s %p: fortify: bind(#p) returns %d, %r", name, getpid(), r);
	rfork(RFNOMNT);
}

void
main(int argc, char *argv[])
{
	int mypid, input, output, devmnt, fs;

	blind = 0;
	ARGBEGIN{
	case 'd':
		enabledebug();
		break;
	case 's':
		stdio = 1;
		break;
	case 'b':
		blind = 1;
		stdio = 1;
		break;
	default:
		fprint(2, "usage: %s [-bds] program args\n", argv0);
		exits("usage");
	}ARGEND;

	if(argc == 0){
		fprint(2, "usage: %s [-bds] program args\n", argv0);
		exits("usage");
	}

	mypid = getpid();

	debug("%s %d: started, stdio = %d, blind = %d\n", argv0, mypid, stdio, blind);

	rfork(RFNAMEG);

	fs = initialize(&devmnt);

	/* start the file system */
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("rfork (file server)");
			break;
		case 0:
			PROVIDE(fs);
			fortify("file server");
			serve(fs);
			break;
		default:
			close(fs);
			break;
	}

	WAIT_FOR(fs);

	/* start output device writer */
	if(stdio)
		output = 1;
	else if((output = open("#P/cgamem", OWRITE)) == -1)
		sysfatal("open #P/cgamem: %r");
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("rfork (%s)", stdio ? "stdout writer" : "writecga");
			break;
		case 0:
			if(mount(connect(fs), -1, "/dev", MBEFORE, "", devmnt) == -1)
				sysfatal("mount (%s)", stdio ? "stdout writer" : "writecga");
			if((input = open("/dev/gconsout", OREAD)) == -1)
				sysfatal("open /dev/gconsout: %r");
			PROVIDE(output);
			fortify(stdio ? "stdout writer" : "writecga");
			if(stdio)
				joint("stdout writer", input, output);
			else
				writecga(input, output);
			break;
		default:
			close(input);
			close(output);
			break;
	}

	/* start input device reader */
	if(stdio)
		input = 0;
	else if((input = open("#P/ps2keyb", OREAD)) == -1)
		sysfatal("open #P/ps2keyb: %r");
	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			sysfatal("rfork (%s)", stdio ? "stdin reader" : "readkeyboard");
			break;
		case 0:
			if(mount(connect(fs), -1, "/dev", MBEFORE, "", devmnt) == -1)
				sysfatal("mount (%s)", stdio ? "stdin reader" : "readkeyboard");
			if((output = open("/dev/gconsin", OWRITE)) == -1)
				sysfatal("open /dev/gconsin: %r");
			PROVIDE(input);
			fortify(stdio ? "stdin reader" : "readkeyboard");
			if(stdio)
				joint("stdin reader", input, output);
			else
				readkeyboard(input, output);
			break;
		default:
			close(input);
			break;
	}

	WAIT_FOR(input);
	WAIT_FOR(output);

	debug("%s %d: all services started, ready to exec(%s)\n", argv0, mypid, argv[0]);

	/* become the requested program */
	rfork(RFNAMEG|RFNOTEG|RFREND);

	if(mount(connect(fs), -1, "/dev", MBEFORE, "", devmnt) == -1)
		sysfatal("mount (%s)", stdio ? "stdin reader" : "readkeyboard");

	rfork(RFCFDG);

	input = open("/dev/cons", OREAD);
	output = open("/dev/cons", OWRITE);
	if(dup(output, 2) != 2)
		sysfatal("bad FDs");

	exec(argv[0], argv);
	sysfatal("exec (%s): %r", argv[0]);
}
