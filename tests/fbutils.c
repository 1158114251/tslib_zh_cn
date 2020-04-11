/*
 * fbutils.c
 *
 * Utility routines for framebuffer interaction
 *
 * Copyright 2002 Russell King and Doug Lowder
 *
 * This file is placed under the GPL.  Please see the
 * file COPYING for details.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/fb.h>

#include "font.h"
#include "fbutils.h"

/* new add by Late Lee 2011-05-30*/
//#define HZK24
#ifdef HZK24 /* 24 */
#include "ascii24.h"
#define ASCII_CODE ascii24
#define FONT_SIZE  24			/* size: 24 */
#else	/* 16 */
#include "ascii16.h"
#define ASCII_CODE ascii16
#define FONT_SIZE  16			/* size: 16 */
#endif


#define BYTES (FONT_SIZE/8)			/* for HZ: 3 bytes  2 bytes*/
#define BUF_SIZE (BYTES * FONT_SIZE)		/* HZ buff 3*24 = 72 bytes 2*16 = 32 bytes */

#define ASCII_BYTES (BYTES-1)		/* 2 1*/
#define ASCII_SIZE (FONT_SIZE * ASCII_BYTES)	/* ASCII buffer: 24*2 = 48 bytes 16 * 1 = 16 bytes */
#define ASCII_WIDTH (FONT_SIZE/2)		/* ASCII: 16*8 24*12 */

/* end here Late Lee*/

union multiptr {
	unsigned char *p8;
	unsigned short *p16;
	unsigned long *p32;
};

static int con_fd, fb_fd, last_vt = -1;
static struct fb_fix_screeninfo fix;
static struct fb_var_screeninfo var;
static unsigned char *fbuffer;
static unsigned char **line_addr;
static int fb_fd=0;
static int bytes_per_pixel;
static unsigned colormap [256];
__u32 xres, yres;

static char *defaultfbdevice = "/dev/fb0";
static char *defaultconsoledevice = "/dev/tty";
static char *fbdevice = NULL;
static char *consoledevice = NULL;

int open_framebuffer(void)
{
	struct vt_stat vts;
	char vtname[128];
	int fd, nr;
	unsigned y, addr;

	if ((fbdevice = getenv ("TSLIB_FBDEVICE")) == NULL)
		fbdevice = defaultfbdevice;

	if ((consoledevice = getenv ("TSLIB_CONSOLEDEVICE")) == NULL)
		consoledevice = defaultconsoledevice;

	if (strcmp (consoledevice, "none") != 0) {
		sprintf (vtname,"%s%d", consoledevice, 1);
        	fd = open (vtname, O_WRONLY);
        	if (fd < 0) {
        	        perror("open consoledevice");
        	        return -1;
        	}

		if (ioctl(fd, VT_OPENQRY, &nr) < 0) {
        	        perror("ioctl VT_OPENQRY");
        	        return -1;
        	}
        	close(fd);

        	sprintf(vtname, "%s%d", consoledevice, nr);

        	con_fd = open(vtname, O_RDWR | O_NDELAY);
        	if (con_fd < 0) {
        	        perror("open tty");
        	        return -1;
        	}

        	if (ioctl(con_fd, VT_GETSTATE, &vts) == 0)
        	        last_vt = vts.v_active;

        	if (ioctl(con_fd, VT_ACTIVATE, nr) < 0) {
        	        perror("VT_ACTIVATE");
        	        close(con_fd);
        	        return -1;
        	}

#ifndef TSLIB_NO_VT_WAITACTIVE
        	if (ioctl(con_fd, VT_WAITACTIVE, nr) < 0) {
        	        perror("VT_WAITACTIVE");
        	        close(con_fd);
        	        return -1;
        	}
#endif

        	if (ioctl(con_fd, KDSETMODE, KD_GRAPHICS) < 0) {
        	        perror("KDSETMODE");
        	        close(con_fd);
        	        return -1;
        	}

	}

	fb_fd = open(fbdevice, O_RDWR);
	if (fb_fd == -1) {
		perror("open fbdevice");
		return -1;
	}

	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix) < 0) {
		perror("ioctl FBIOGET_FSCREENINFO");
		close(fb_fd);
		return -1;
	}

	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var) < 0) {
		perror("ioctl FBIOGET_VSCREENINFO");
		close(fb_fd);
		return -1;
	}
	xres = var.xres;
	yres = var.yres;

	fbuffer = mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fb_fd, 0);
	if (fbuffer == (unsigned char *)-1) {
		perror("mmap framebuffer");
		close(fb_fd);
		return -1;
	}
	memset(fbuffer,0,fix.smem_len);

	bytes_per_pixel = (var.bits_per_pixel + 7) / 8;
	line_addr = malloc (sizeof (__u32) * var.yres_virtual);
	addr = 0;
	for (y = 0; y < var.yres_virtual; y++, addr += fix.line_length)
		line_addr [y] = fbuffer + addr;

	return 0;
}

void close_framebuffer(void)
{
	munmap(fbuffer, fix.smem_len);
	close(fb_fd);


	if(strcmp(consoledevice,"none")!=0) {
	
        	if (ioctl(con_fd, KDSETMODE, KD_TEXT) < 0)
        	        perror("KDSETMODE");

        	if (last_vt >= 0)
        	        if (ioctl(con_fd, VT_ACTIVATE, last_vt))
        	                perror("VT_ACTIVATE");

        	close(con_fd);
	}

        free (line_addr);
}

void put_cross(int x, int y, unsigned colidx)
{
	line (x - 10, y, x - 2, y, colidx);
	line (x + 2, y, x + 10, y, colidx);
	line (x, y - 10, x, y - 2, colidx);
	line (x, y + 2, x, y + 10, colidx);

#if 1
	line (x - 6, y - 9, x - 9, y - 9, colidx + 1);
	line (x - 9, y - 8, x - 9, y - 6, colidx + 1);
	line (x - 9, y + 6, x - 9, y + 9, colidx + 1);
	line (x - 8, y + 9, x - 6, y + 9, colidx + 1);
	line (x + 6, y + 9, x + 9, y + 9, colidx + 1);
	line (x + 9, y + 8, x + 9, y + 6, colidx + 1);
	line (x + 9, y - 6, x + 9, y - 9, colidx + 1);
	line (x + 8, y - 9, x + 6, y - 9, colidx + 1);
#else
	line (x - 7, y - 7, x - 4, y - 4, colidx + 1);
	line (x - 7, y + 7, x - 4, y + 4, colidx + 1);
	line (x + 4, y - 4, x + 7, y - 7, colidx + 1);
	line (x + 4, y + 4, x + 7, y + 7, colidx + 1);
#endif
}

void put_char(int x, int y, int c, int colidx)
{
	int i,j,bits;

	for (i = 0; i < font_vga_8x8.height; i++) {
		bits = font_vga_8x8.data [font_vga_8x8.height * c + i];
		for (j = 0; j < font_vga_8x8.width; j++, bits <<= 1)
			if (bits & 0x80)
				pixel (x + j, y + i, colidx);
	}
}

void put_string(int x, int y, char *s, unsigned colidx)
{
	int i;
	for (i = 0; *s; i++, x += font_vga_8x8.width, s++)
		put_char (x, y, *s, colidx);
}

void put_string_center(int x, int y, char *s, unsigned colidx)
{
	size_t sl = strlen (s);
        put_string (x - (sl / 2) * font_vga_8x8.width,
                    y - font_vga_8x8.height / 2, s, colidx);
}

void setcolor(unsigned colidx, unsigned value)
{
	unsigned res;
	unsigned short red, green, blue;
	struct fb_cmap cmap;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color index = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

	switch (bytes_per_pixel) {
	default:
	case 1:
		res = colidx;
		red = (value >> 8) & 0xff00;
		green = value & 0xff00;
		blue = (value << 8) & 0xff00;
		cmap.start = colidx;
		cmap.len = 1;
		cmap.red = &red;
		cmap.green = &green;
		cmap.blue = &blue;
		cmap.transp = NULL;

        	if (ioctl (fb_fd, FBIOPUTCMAP, &cmap) < 0)
        	        perror("ioctl FBIOPUTCMAP");
		break;
	case 2:
	case 4:
		red = (value >> 16) & 0xff;
		green = (value >> 8) & 0xff;
		blue = value & 0xff;
		res = ((red >> (8 - var.red.length)) << var.red.offset) |
                      ((green >> (8 - var.green.length)) << var.green.offset) |
                      ((blue >> (8 - var.blue.length)) << var.blue.offset);
	}
        colormap [colidx] = res;
}

static inline void __setpixel (union multiptr loc, unsigned xormode, unsigned color)
{
	switch(bytes_per_pixel) {
	case 1:
	default:
		if (xormode)
			*loc.p8 ^= color;
		else
			*loc.p8 = color;
		break;
	case 2:
		if (xormode)
			*loc.p16 ^= color;
		else
			*loc.p16 = color;
		break;
	case 4:
		if (xormode)
			*loc.p32 ^= color;
		else
			*loc.p32 = color;
		break;
	}
}

void pixel (int x, int y, unsigned colidx)
{
	unsigned xormode;
	union multiptr loc;

	if ((x < 0) || ((__u32)x >= var.xres_virtual) ||
	    (y < 0) || ((__u32)y >= var.yres_virtual))
		return;

	xormode = colidx & XORMODE;
	colidx &= ~XORMODE;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color value = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

	loc.p8 = line_addr [y] + x * bytes_per_pixel;
	__setpixel (loc, xormode, colormap [colidx]);
}

void line (int x1, int y1, int x2, int y2, unsigned colidx)
{
	int tmp;
	int dx = x2 - x1;
	int dy = y2 - y1;

	if (abs (dx) < abs (dy)) {
		if (y1 > y2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			dx = -dx; dy = -dy;
		}
		x1 <<= 16;
		/* dy is apriori >0 */
		dx = (dx << 16) / dy;
		while (y1 <= y2) {
			pixel (x1 >> 16, y1, colidx);
			x1 += dx;
			y1++;
		}
	} else {
		if (x1 > x2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			dx = -dx; dy = -dy;
		}
		y1 <<= 16;
		dy = dx ? (dy << 16) / dx : 0;
		while (x1 <= x2) {
			pixel (x1, y1 >> 16, colidx);
			y1 += dy;
			x1++;
		}
	}
}

void rect (int x1, int y1, int x2, int y2, unsigned colidx)
{
	line (x1, y1, x2, y1, colidx);
	line (x2, y1, x2, y2, colidx);
	line (x2, y2, x1, y2, colidx);
	line (x1, y2, x1, y1, colidx);
}

void fillrect (int x1, int y1, int x2, int y2, unsigned colidx)
{
	int tmp;
	unsigned xormode;
	union multiptr loc;

	/* Clipping and sanity checking */
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	if (x1 < 0) x1 = 0; if ((__u32)x1 >= xres) x1 = xres - 1;
	if (x2 < 0) x2 = 0; if ((__u32)x2 >= xres) x2 = xres - 1;
	if (y1 < 0) y1 = 0; if ((__u32)y1 >= yres) y1 = yres - 1;
	if (y2 < 0) y2 = 0; if ((__u32)y2 >= yres) y2 = yres - 1;

	if ((x1 > x2) || (y1 > y2))
		return;

	xormode = colidx & XORMODE;
	colidx &= ~XORMODE;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color value = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

	colidx = colormap [colidx];

	for (; y1 <= y2; y1++) {
		loc.p8 = line_addr [y1] + x1 * bytes_per_pixel;
		for (tmp = x1; tmp <= x2; tmp++) {
			__setpixel (loc, xormode, colidx);
			loc.p8 += bytes_per_pixel;
		}
	}
}

/*****************************************************************************
*           new add by Late Lee 2011-05-30
*****************************************************************************/

/**
 * __display_ascii - Display an ASCII code on touch screen
 * @x: Column
 * @y: Row
 * @ascii: Which ASCII code to display 
 * @colidx: Color index(?)
 * This routine display an ASCII code that stored in an array(eg, ASCII_CODE).
 * 16x8 ASCII code takes 1 byte, 24*12 ASCII code takes 2 bytes, so we need 
 * -ASCII_BYTES-.
 */
static void __display_ascii(int x, int y, char *ascii, unsigned colidx)
{
	int i, j, k;
	unsigned char *p_ascii;
	int offset;	
	
	offset = (*ascii - 0x20 ) * ASCII_SIZE; /* find the code in the array */
	p_ascii = ASCII_CODE + offset;

	for(i=0;i<FONT_SIZE;i++)
		for(j=0;j<ASCII_BYTES;j++)
			for(k=0;k<8;k++)
				if( p_ascii[i*ASCII_BYTES+j] & (0x80>>k) )
				//if(*( p_ascii + i*ASCII_BYTES+j) & (0x80>>k))
					pixel (x + j*8 + k, y + i, colidx);
}

/**
 * put_string_ascii - Display an ASCII string on touch screen
 * @x: Column
 * @y: Row
 * @s: Which string to display
 * @colidx: Color index
 */
void put_string_ascii(int x, int y, char *s, unsigned colidx)
{
	while (*s != 0) {
		__display_ascii(x, y, s, colidx);
		x += ASCII_WIDTH;
		s++;
	}
}

/* not test */
void put_string_center_ascii(int x, int y, char *s, unsigned colidx)
{
	size_t sl = strlen (s);
        put_string_ascii (x - (sl / 2) * ASCII_WIDTH,
                    y - FONT_SIZE / 2, s, colidx);
}

/**
 * __display_font_16 - Display a 16x16 (chinese) character on touch screen
 * @fp: File pointer points to HZK(ie, HZK16)
 * @x: Column
 * @y: Row
 * @font: Which (chinese) character to display
 * @colidx: Color index
 * This routine ONLY display 16*16 character.
 * Every character takes two bytes, we show the first 8 bits, then the second 8 bits,
 * then the whole world will be shown before us.
 */
static void __display_font_16 (FILE *fp, int x, int y, unsigned char *font, unsigned colidx)
{
	int i, j, k;
	unsigned char mat[BUF_SIZE]={0};
	int qh,wh;
	unsigned long offset;
	qh = *font   - 0xa0;
	wh = *(font+1) - 0xa0;
	offset = ( 94*(qh-1) + (wh-1) ) * BUF_SIZE; /* offset of the character in HZK */

	/* read it */
	fseek(fp,offset,SEEK_SET);
	fread(mat,BUF_SIZE,1,fp);

	/* show it */
	for(i=0;i<FONT_SIZE;i++)
		for(j=0;j<BYTES;j++)
			for(k=0;k<8;k++)
				if(mat [i*BYTES+j] & (0x80>>k))
					pixel (x + j*8 + k, y + i, colidx);
}

/**
 * __display_font_24 - Display a 24x24 (chinese) character on touch screen
 * @fp: File pointer points to HZK(ie, HZK24)
 * @x: Column
 * @y: Row
 * @font: Which (chinese) character to display
 * @colidx: Color index
 */
static void __display_font_24 (FILE *fp, int x, int y, unsigned char *font, unsigned colidx)
{
	unsigned int i, j;
	unsigned char mat[FONT_SIZE][BYTES]={{0}};
	int qh,wh;
	unsigned long offset;
	qh = *font   - 0xaf;
	wh = *(font+1) - 0xa0;
	offset = ( 94*(qh-1) + (wh-1) ) * BUF_SIZE;

	fseek(fp,offset,SEEK_SET);
	fread(mat,BUF_SIZE,1,fp);

	for(i=0;i<FONT_SIZE;i++)
		for(j=0;j<FONT_SIZE;j++)
			if( mat[j][i>>3] & (0x80>>(i&7)) )
			// if ( mat[j][i/8] & (0x80>>i%8) ) /* org */
				pixel (x + j, y + i, colidx);
}

/**
 * put_string_hz - Display a (chinese) character string on touch screen
 * @fp: File pointer points to HZK(ie, HZK24 or HZK16)
 * @x: Column
 * @y: Row
 * @s: Which string to display(must be 'unsigned char*')
 * @colidx: Color index
 */
void put_string_hz (FILE *fp, int x, int y, unsigned char *s, unsigned colidx)
{	
	while (*s != 0) {
		#ifdef HZK24
		__display_font_24 (fp, x, y, s, colidx); /* for HZK24 */
		#else
		__display_font_16 (fp, x, y, s, colidx);
		#endif
		x += FONT_SIZE;
		s += 2;	/* 2 bytes */
	}
}

/* not test */
void put_string_center_hz (FILE *fp, int x, int y, unsigned char *s, unsigned colidx)
{
	size_t sl = strlen ((char *)s);
        put_string_hz (fp, x - (sl/2) * FONT_SIZE, y - FONT_SIZE/2, s, colidx);
}

/**
 * put_font - Display an ASCII or/and (chinese) character string on touch screen
 * @fp: File pointer points to HZK(ie, HZK24 or HZK16)
 * @x: Column
 * @y: Row
 * @s: Which string to display
 * @colidx: Color index
 */
void put_font(FILE *fp, int x, int y, unsigned char *s, unsigned colidx)
{
	while (*s != 0) {
		if ( (*s>0xa0) && (*(s+1)>0xa0) ) {
			#ifdef HZK24
			__display_font_24 (fp, x, y, s, colidx); 	/* for HZK24 */
			#else
			__display_font_16 (fp, x, y, s, colidx);	/* for HZK16 */
			#endif
			x += FONT_SIZE;
			s += 2;	/* 2 bytes */
		} else {
			__display_ascii (x, y, (char *)s, colidx);
			x += ASCII_WIDTH;
			s++;	/* 1 byte */
		}
	}
}
/* not test */
void put_font_center(FILE *fp, int x, int y, unsigned char *s, unsigned colidx)
{
	size_t sl = strlen ((char *)s);
        put_font (fp, x - (sl/2) * 16, y - 16/2, s, colidx);
}
