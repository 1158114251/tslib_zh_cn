/* new add by Late Lee 2011-05-30*/
//#define HZK24
#ifdef HZK24 /* 24 */
#include "ascii24.h"
#define ascii_code ascii24
#define size  24			/* size: 24 */
#else	/* 16 */
#include "ascii16.h"
#define ascii_code ascii16
#define size  16			/* size: 16 */
#endif


#define bytes (size/8)			/* for HZ: 3 bytes  2 bytes*/
#define buf_size (bytes * size)		/* HZ buff 3*24 = 72 bytes 2*16 = 32 bytes */

#define ascii_bytes (bytes-1)		/* 2 1*/
#define ascii_size (size * ascii_bytes)	/* ASCII buffer: 24*2 = 48 bytes 16 * 1 = 16 bytes */
#define ascii_width (size/2)		/* ASCII: 16*8 24*12 */

/* 原始版本： */
static void __display_ascii(int x, int y, char *ascii, unsigned colidx)
{
	int i, j, k;
	for(i=0;i<size;i++)
		for(j=0;j<ascii_bytes;j++)
			for(k=0;k<8;k++)
				if(ascii[i*ascii_bytes+j] & (0x80>>k))
					pixel (x + j*8 + k, y + i, colidx);
}

void put_string_ascii(int x, int y, char *font, unsigned colidx)
{
	char *p_ascii;
	int offset;
	char *p = font;

	while (*p != 0) {
		offset = (*p - 0x20 ) * ascii_size;
		p_ascii = ascii_code + offset;
		__display_ascii(x, y, p_ascii, colidx);
		x += ascii_width+1;
		p++;
	}
}

static void __display_font (int x, int y, unsigned char *mat, unsigned colidx)
{
	int i, j, k;
	for(i=0;i<size;i++)
		for(j=0;j<bytes;j++)
			for(k=0;k<8;k++)
				if(mat [i*bytes+j] & (0x80>>k))
					pixel (x + j*8 + k, y + i, colidx);
}

static void __display_font_24 (int x, int y, unsigned char mat[][3], unsigned colidx)
{
	int i, j, k;
	for(i=0;i<size;i++)
		for(j=0;j<size;j++)
			for(k=0;k<8;k++)
				if(mat[j][i/8] & (0x80>>i%8))
					pixel (x + j, y + i, colidx);
}

void put_string_hz (FILE *fp, int x, int y, unsigned char *s, unsigned colidx)
{
	//#ifdef HZK24
	//unsigned char mat [size][3]={0}; /* you may need to change mat if using HZK24 */
	//#else
	unsigned char mat [buf_size]={0};
	//#endif
	int qh,wh;
	unsigned long offset;
	int i;
	for (i = 0; *s; i++, x += (size+1), s += 2) {
		#ifdef HZK24
		qh = *s   - 0xaf;	/* for HZK24 */
		#else
		qh = *s   - 0xa0;	/* for HZK16 */
		#endif
		wh = *(s+1) - 0xa0;
		offset = ( 94*(qh-1) + (wh-1) ) * buf_size;
		fseek (fp, offset, SEEK_SET);
		fread (mat, buf_size, 1, fp);
		#ifdef HZK24
		__display_font_24 (x, y, mat, colidx);
		#else
		__display_font (x, y, mat, colidx);
		#endif
	}
}

//新版本：

static void __display_ascii(int x, int y, char *ascii, unsigned colidx)
{
	int i, j, k;
	char *p_ascii;
	int offset;	
	
	offset = (*ascii - 0x20 ) * ascii_size;
	p_ascii = ascii_code + offset;

	for(i=0;i<size;i++)
		for(j=0;j<ascii_bytes;j++)
			for(k=0;k<8;k++)
				if( p_ascii[i*ascii_bytes+j] & (0x80>>k) )
				//if(*( p_ascii + i*ascii_bytes+j) & (0x80>>k))
					pixel (x + j*8 + k, y + i, colidx);
}

void put_string_ascii(int x, int y, char *s, unsigned colidx)
{
	while (*s != 0) {
		__display_ascii(x, y, s, colidx);
		x += ascii_width;
		s++;
	}
}

/* not test */
void put_string_center_ascii(int x, int y, char *s, unsigned colidx)
{
	size_t sl = strlen (s);
        put_string_ascii (x - (sl / 2) * ascii_width,
                    y - size / 2, s, colidx);
}

static void __display_font (FILE *fp, int x, int y, unsigned char *font, unsigned colidx)
{
	int i, j, k;
	unsigned char mat[buf_size]={0};
	int qh,wh;
	unsigned long offset;
	qh = *font   - 0xa0;
	wh = *(font+1) - 0xa0;
	offset = ( 94*(qh-1) + (wh-1) ) * buf_size;

	fseek(fp,offset,SEEK_SET);
	fread(mat,buf_size,1,fp);

	for(i=0;i<size;i++)
		for(j=0;j<bytes;j++)
			for(k=0;k<8;k++)
				if(mat [i*bytes+j] & (0x80>>k))
					pixel (x + j*8 + k, y + i, colidx);
}

static void __display_font_24 (FILE *fp, int x, int y, unsigned char *font, unsigned colidx)
{
	unsigned int i, j;
	unsigned char mat[size][bytes]={{0}};
	int qh,wh;
	unsigned long offset;
	qh = *font   - 0xaf;
	wh = *(font+1) - 0xa0;
	offset = ( 94*(qh-1) + (wh-1) ) * buf_size;

	fseek(fp,offset,SEEK_SET);
	fread(mat,buf_size,1,fp);

	for(i=0;i<size;i++)
		for(j=0;j<size;j++)
			if( mat[j][i>>3] & (0x80>>(i&7)) )
			// if ( mat[j][i/8] & (0x80>>i%8) )
				pixel (x + j, y + i, colidx);
}

void put_string_hz (FILE *fp, int x, int y, unsigned char *s, unsigned colidx)
{	
	while (*s != 0) {
		#ifdef HZK24
		__display_font_24(fp, x, y, s, colidx); /* for HZK24 */
		#else
		__display_font(fp, x, y, s, colidx);
		#endif
		x += size;
		s += 2;	/* 2 bytes */
	}
	#if 0
	unsigned char mat [buf_size]={0};
	int qh,wh;
	unsigned long offset;
	int i;
	for (i = 0; *s; i++, x += (size+1), s += 2) {
		#ifdef HZK24
		qh = *s   - 0xaf;	/* for HZK24 */
		#else
		qh = *s   - 0xa0;	/* for HZK16 */
		#endif
		wh = *(s+1) - 0xa0;
		offset = ( 94*(qh-1) + (wh-1) ) * buf_size;
		fseek (fp, offset, SEEK_SET);
		fread (mat, buf_size, 1, fp);
		#ifdef HZK24
		__display_font_24 (x, y, mat, colidx);
		#else
		__display_font (x, y, mat, colidx);
		#endif
	}
	#endif
}

void put_font(FILE *fp, int x, int y, unsigned char *s, unsigned colidx)
{
	while (*s != 0) {
		if ( (*s - 0xaf) > 0 && (*(s) - 0xa0) > 0) {
			#ifdef HZK24
			__display_font_24(fp, x, y, s, colidx); /* for HZK24 */
			#else
			__display_font(fp, x, y, s, colidx);
			#endif
			x += size;
			s += 2;	/* 2 bytes */
		} else {
			__display_ascii(x, y, (char *)s, colidx);
			x += ascii_width;
			s++;	/* 1 byte */
		}
	}
}

// 再次修改的版本
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
