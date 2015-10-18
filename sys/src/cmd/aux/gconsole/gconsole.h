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

#define IODATASZ	8192

enum
{
	KeyboardBufferSize = 32,
	ScreenBufferSize = KeyboardBufferSize*8,
};

extern int stdio;
extern int rawmode;
extern int blind;	/* no feedback for input, disable rawmode */

extern void enabledebug(void);
extern void debug(const char *, ...);

extern int initialize(int *);
extern void serve(int);
extern int connect(int);

extern void writecga(int, int);

extern void readkeyboard(int, int);

typedef struct Buffer Buffer;
struct Buffer
{
	int size;
	int written;
	int read;
	char *data;
};
extern Buffer* balloc(uint32_t);
extern uint32_t bwrite(Buffer *, char *, uint32_t);
extern void bdelete(Buffer *b, uint32_t len);
extern char *bread(Buffer *, uint32_t *);
#define bempty(b) (b->written == b->read)
