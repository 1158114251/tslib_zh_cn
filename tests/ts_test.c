/*
 *  tslib/src/ts_test.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Basic test program for touchscreen library.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "tslib.h"
#include "fbutils.h"

#undef TS_DEBUG
#define TS_DEBUG

//#define DEBUG_MSG
#if define TS_DEBUG && DEBUG_MSG
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif /* debug msg */

/* debug */

#ifdef TS_DEBUG
/* test */
/* 0.back 1.xx 2.white 3.xx 4.xx 5.xx 6.purple 7.red 8.green 9.blue 10. yellow*/
static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0, 0x6600cc, 0xff0000, 0x00ff00, 0x0000ff, 0xffff00
};
#else
/* org */
static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0
};
#endif

#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

struct ts_button {
	int x, y, w, h;
	char *text;
	int flags;
#define BUTTON_ACTIVE 0x00000001
};

/* [inactive] border fill text [active] border fill text */
static int button_palette [6] =
{
	1, 4, 2,
	1, 5, 0
};

#define NR_BUTTONS 3
static struct ts_button buttons [NR_BUTTONS];

static FILE *fp; 	/* HZK file pointer by Late Lee */
#define HZK "HZK/HZK16"	/* HZK16 or HZK24K/HZK24S */

static void sig(int sig)
{
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fclose(fp);	/* by Late Lee */
	fflush(stdout);
	exit(1);
}

static void button_draw (struct ts_button *button)
{
	int s = (button->flags & BUTTON_ACTIVE) ? 3 : 0;
	rect (button->x, button->y, button->x + button->w - 1,
	      button->y + button->h - 1, button_palette [s]);
	fillrect (button->x + 1, button->y + 1,
		  button->x + button->w - 2,
		  button->y + button->h - 2, button_palette [s + 1]);
	put_string_center (button->x + button->w / 2,
			   button->y + button->h / 2,
			   button->text, button_palette [s + 2]);
}

static int button_handle (struct ts_button *button, struct ts_sample *samp)
{
	int inside = (samp->x >= button->x) && (samp->y >= button->y) &&
		(samp->x < button->x + button->w) &&
		(samp->y < button->y + button->h);

	if (samp->pressure > 0) {
		if (inside) {
			if (!(button->flags & BUTTON_ACTIVE)) {
				button->flags |= BUTTON_ACTIVE;
				debug("(inside button: !BUTTON_ACTIVE)button->flags:%d\n", button->flags);
				button_draw (button);
			}
		} else if (button->flags & BUTTON_ACTIVE) {
			button->flags &= ~BUTTON_ACTIVE;
			debug("(outside button BUTTON_ACTIVE)button->flags:%d\n", button->flags);
			button_draw (button);
		}
	} else if (button->flags & BUTTON_ACTIVE) {
		button->flags &= ~BUTTON_ACTIVE;
		debug("(pressure == 0 return here)button->flags:%d\n", button->flags);
		button_draw (button);
                return 1;
	}
	debug("do nothing...\n");
        return 0;
}

static void refresh_screen ()
{
	int i;

	fillrect (0, 0, xres - 1, yres - 1, 0);
	put_string_center (xres/2, yres/4,   "TSLIB test program", 1);
	put_string_center (xres/2, yres/4+20,"Touch screen to move crosshair", 2);

	#ifdef TS_DEBUG
	/* just a test */
	unsigned char incode[] = "▲！ADC■测镕试◎示例";
	int y = yres/4+50;
	put_string_ascii(0, y, "Powered by Late Lee", 9);
	put_string_hz(fp, 0, y+30, "波神留我看斜阳听取蛙声一片", 2);
	put_font(fp, 0, y+56, incode, 5);
	/* end of the test */
	#endif
	for (i = 0; i < NR_BUTTONS; i++)
		button_draw (&buttons [i]);
}

int main()
{
	struct tsdev *ts;
	int x, y;
	unsigned int i;
	unsigned int mode = 0;
	int quit_pressed = 0;

	char *tsdevice=NULL;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	if( (tsdevice = getenv("TSLIB_TSDEVICE")) != NULL ) {
		ts = ts_open(tsdevice,0);
	} else {
		if (!(ts = ts_open("/dev/input/event0", 0)))
			ts = ts_open("/dev/touchscreen/ucb1x00", 0);
	}

	if (!ts) {
		perror("ts_open");
		exit(1);
	}

	if (ts_config(ts)) {
		perror("ts_config");
		exit(1);
	}

	if (open_framebuffer()) {
		close_framebuffer();
		exit(1);
	}

	if ( (fp = fopen(HZK, "rb")) == NULL ) {
		perror("Can't open HZK");
		exit(1);
	}

	x = xres/2;
	y = yres/2;

	for (i = 0; i < NR_COLORS; i++)
		setcolor (i, palette [i]);

	/* Initialize buttons */
	memset (&buttons, 0, sizeof (buttons));
	buttons [0].w = buttons [1].w = buttons [2].w = xres / 4;
	buttons [0].h = buttons [1].h = buttons [2].h = 20;
	buttons [0].x = 0;
	buttons [1].x = (3 * xres) / 8;
	buttons [2].x = (3 * xres) / 4;
	buttons [0].y = buttons [1].y = buttons [2].y = 10;
	buttons [0].text = "Drag";
	buttons [1].text = "Draw";
	buttons [2].text = "Quit";

	refresh_screen ();

	while (1) {
		struct ts_sample samp;
		int ret;

		/* Show the cross */
		if ((mode & 15) != 1) {
			put_cross(x, y, 2 | XORMODE);
			debug("show the cross x: %d y: %d   ", x, y);
		}

		ret = ts_read(ts, &samp, 1);

		/* Hide it */
		if ((mode & 15) != 1) {
			put_cross(x, y, 2 | XORMODE);
			debug("hide it x: %d y: %d   ", x, y);
		}

		if (ret < 0) {
			perror("ts_read");
			close_framebuffer();
			exit(1);
		}

		if (ret != 1)
			continue;

		for (i = 0; i < NR_BUTTONS; i++)
			if (button_handle (&buttons [i], &samp))
				switch (i) {
				case 0:
					mode = 0;
					refresh_screen ();
					break;
				case 1:
					mode = 1;
					refresh_screen ();
					break;
				case 2:
					quit_pressed = 1;
				}

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec,
			samp.x, samp.y, samp.pressure);

		if (samp.pressure > 0) {
			if (mode == 0x80000001)
				line (x, y, samp.x, samp.y, 2);
			x = samp.x;
			y = samp.y;
			mode |= 0x80000000;
			debug("mode(pressure>0): %x x: %d y: %d\n", mode, x, y);
		} else
			mode &= ~0x80000000;
		debug("the end of while (1), mode: %x\n", mode);
		if (quit_pressed)
			break;
	}
	fclose(fp);	/* add by Late Lee */
	close_framebuffer();

	return 0; /* add by Late Lee */
}
