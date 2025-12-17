#include <stdio.h>
#include <sys/sgtty.h>
#include <graphics.h>
#include <signal.h>
#include "doomkeys.h"
#include "m_fixed.h"

extern char *DG_ScreenBuffer;
extern unsigned char colors[256][4];


struct tgaheader
{
	unsigned char identsize;
	unsigned char cmaptype;
	unsigned char imagetype;

	unsigned char cmapstart[2];
	unsigned char cmaplen[2];
	unsigned char cmapbits;
	
	unsigned char xstart[2];
	unsigned char ystart[2];
	unsigned char width[2];
	unsigned char height[2];

	unsigned char bits;
	unsigned char desc;
};

int framenum = 0;
int
writeBGR(const char *filename, int vpw, int vph, unsigned char *rgbData)
{
unsigned char pal[256][3];
char name[128];

	sprintf(name, "doom%d.tga", (framenum++) & 7);
	FILE *fp = fopen(name, "w");
	if (fp)
	{
		struct tgaheader header;
		int i;
		
		header.identsize = 0;
		header.cmaptype = 1;
		header.imagetype = 1;			/* cmap256 */
		header.cmaplen[0] = 0;
		header.cmaplen[1] = 1;
		header.cmapbits = 24;
		/* tga expects BGR */
		for(i=0; i<256; i++)
		{
			pal[i][2] = colors[i][2];
			pal[i][1] = colors[i][1];
			pal[i][0] = colors[i][0];
		}


		header.cmapstart[0] = 0;
		header.cmapstart[1] = 0;
		header.xstart[0] = 0;
		header.xstart[1] = 0;
		header.ystart[0] = 0;
		header.ystart[1] = 0;
		header.width[0] = vpw & 255;
		header.width[1] = vpw/256;
		header.height[0] = vph & 255;
		header.height[1] = vph/256;

		header.bits = 8;
		header.desc = 32;

		fwrite(&header, 1, sizeof(header), fp);
		fwrite(&pal, 1, sizeof(pal), fp);
		fwrite(rgbData, 1, vpw*vph, fp);
		fclose(fp);
		
		/*printf("wrote %s\015", name); */
	}
	
	return 1;
}

#define USE_STDINxx
#define USE_EVENTS

#define KEYQUEUE_SIZE 16
static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

#define TEKKEY_CTRL 1
#define TEKKEY_SHIFT 2
unsigned int modifiers = 0;
unsigned char shifted[128] =
{
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\"',
  '\0','\0','\0','\0', '<', '_', '>', '?',
   ')', '!', '@', '#', '$', '%', '^', '&',
   '*', '(','\0', ':','\0', '+','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0', '{', '`', '}','\0','\0',
  '\0', 'A', 'B','C','D','E','F','G',
  'H','I','J','K','L','M','N','O',
  'P','Q','R','S','T','U','V','W',
  'X','Y','Z','~','\0','\0','\0',0x7f
};


static unsigned char convertToDoomKey(unsigned char key)
{
	switch (key)
	{
	case 0x0d:
		key = KEY_ENTER;
		break;
	case 0x1b:
		key = KEY_ESCAPE;
		break;
	case 0x61:	/* A */
		key = KEY_LEFTARROW;
		break;
	case 0x64:	/* D */
		key = KEY_RIGHTARROW;
		break;
	case 0x77:	/* W */
		key = KEY_UPARROW;
		break;
	case 0x73:	/* S */
		key = KEY_DOWNARROW;
		break;
	case 0x82:	/* Left Mouse Button */
	case 0x8a:	/* Ctrl */
		key = KEY_FIRE;
		break;
	case 0x20:	/* space */
		key = KEY_USE;
		break;
	case 0x88:	/* Shift */
	case 0x89:	/* Shift */
		key = KEY_RSHIFT;
		break;
		
	case 0xc9:
	case 0xca:
	case 0xcb:
	case 0xcc:
	case 0xcd:
	case 0xce:
	case 0xcf:
	case 0xd0:
	case 0xd1:
	case 0xd2:
	case 0xd3:
	case 0xd4:
		key = KEY_F1 + (key - 0xc9);
		break;
		
	default:
		key = (modifiers & TEKKEY_SHIFT) ? shifted[key] : key;
		break;
	}

	return key;
}

static void addKeyToQueue(int pressed, unsigned char keyCode)
{
	unsigned char key = convertToDoomKey(keyCode);

	unsigned short keyData = (pressed << 8) | key;

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
	
}

unsigned long timestamp = 0;
void readkeyboardevents()
{
  int ecount;
  union EVENTUNION ev;
  unsigned long timelo,timehi;
  
  ecount = EGetCount();
  while (ecount-- > 0)
  {
    ev.evalue = EGetNext();
    
    switch(ev.estruct.etype)
    {
      case E_ABSTIME:
        timehi = EGetNext();
        timelo = EGetNext();
        ecount -= 2;
        timestamp = (timehi << 16) | timelo;
        break;
      case E_DELTATIME:
        timestamp += ev.estruct.eparam;
        break;

      case E_PRESS:
				addKeyToQueue(1, ev.estruct.eparam);
				
				if (ev.estruct.eparam == 0x88)
					modifiers |= TEKKEY_SHIFT;
				if (ev.estruct.eparam == 0x8a)
					modifiers |= TEKKEY_CTRL;
				/* Ctrl-C quits */
				if (ev.estruct.eparam == 'c' && (modifiers & TEKKEY_CTRL))
					kill(getpid(), SIGINT);
				if (ev.estruct.eparam == 's' && (modifiers & TEKKEY_CTRL))
					putchar('s' - 0x60);
				if (ev.estruct.eparam == 'q' && (modifiers & TEKKEY_CTRL))
					putchar('q' - 0x60);
        break;
        
      case E_RELEASE:
				addKeyToQueue(0, ev.estruct.eparam);

				if (ev.estruct.eparam == 0x88)
					modifiers &= ~TEKKEY_SHIFT;
				if (ev.estruct.eparam == 0x8a)
					modifiers &= ~TEKKEY_CTRL;

				if (ev.estruct.eparam == 'p')
					writeBGR("doom.tga", DOOMGENERIC_RESX, DOOMGENERIC_RESY, (unsigned char *)DG_ScreenBuffer);
				
        break;
        
      case E_XMOUSE:
        break;
      case E_YMOUSE:
        break;
    }
  }
}

struct sgttyb orig_term_settings;
struct sgttyb term_settings;
struct FORM *screen;
unsigned char pal2grey[4][256];
unsigned char fatbits[2][256];
int initlookup = 0;

/* 0% 25% 50% 100%   NB should be 0%,33%,66%,100%  but can't do that in a 2x2 block! */
unsigned char halftone[2][4] = {{0x0, 0x1, 0x1, 0x3},{0x0, 0x0, 0x2, 0x3}};

unsigned char gamma_lut[256] = {
#if 0
/* inverse gamma 1.2 */
     0,   3,   4,   6,   8,  10,  11,  13,  14,  16,  17,  19,  20,  21,  23,  24,
    25,  27,  28,  29,  31,  32,  33,  34,  36,  37,  38,  39,  40,  42,  43,  44,
    45,  46,  48,  49,  50,  51,  52,  53,  55,  56,  57,  58,  59,  60,  61,  62,
    63,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  79,  80,
    81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,
    97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
   113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
   128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
   144, 145, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158,
   158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 168, 169, 170, 171, 172,
   173, 174, 175, 176, 177, 177, 178, 179, 180, 181, 182, 183, 184, 185, 185, 186,
   187, 188, 189, 190, 191, 192, 193, 193, 194, 195, 196, 197, 198, 199, 200, 200,
   201, 202, 203, 204, 205, 206, 207, 207, 208, 209, 210, 211, 212, 213, 213, 214,
   215, 216, 217, 218, 219, 219, 220, 221, 222, 223, 224, 225, 225, 226, 227, 228,
   229, 230, 231, 231, 232, 233, 234, 235, 236, 237, 237, 238, 239, 240, 241, 242,
   242, 243, 244, 245, 246, 247, 247, 248, 249, 250, 251, 252, 252, 253, 254, 255,
#endif

#if 0
/* inverse gamma 1.4 */
     0,   5,   8,  11,  13,  16,  18,  20,  22,  24,  26,  27,  29,  31,  32,  34,
    36,  37,  39,  40,  42,  43,  45,  46,  48,  49,  50,  52,  53,  54,  56,  57,
    58,  60,  61,  62,  64,  65,  66,  67,  68,  70,  71,  72,  73,  74,  76,  77,
    78,  79,  80,  81,  82,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,
    96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
   112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 126,
   127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 139, 140, 141,
   142, 143, 144, 145, 146, 147, 148, 148, 149, 150, 151, 152, 153, 154, 155, 155,
   156, 157, 158, 159, 160, 161, 161, 162, 163, 164, 165, 166, 167, 167, 168, 169,
   170, 171, 172, 172, 173, 174, 175, 176, 177, 177, 178, 179, 180, 181, 182, 182,
   183, 184, 185, 186, 186, 187, 188, 189, 190, 190, 191, 192, 193, 194, 194, 195,
   196, 197, 198, 198, 199, 200, 201, 201, 202, 203, 204, 205, 205, 206, 207, 208,
   208, 209, 210, 211, 212, 212, 213, 214, 215, 215, 216, 217, 218, 218, 219, 220,
   221, 221, 222, 223, 224, 224, 225, 226, 227, 227, 228, 229, 230, 230, 231, 232,
   233, 233, 234, 235, 236, 236, 237, 238, 238, 239, 240, 241, 241, 242, 243, 244,
   244, 245, 246, 246, 247, 248, 249, 249, 250, 251, 251, 252, 253, 254, 254, 255,
#endif

#if 0
/* inverse gamma 1.6 */
     0,   8,  12,  16,  19,  22,  24,  27,  29,  32,  34,  36,  38,  40,  42,  43,
    45,  47,  49,  50,  52,  54,  55,  57,  58,  60,  61,  63,  64,  66,  67,  68,
    70,  71,  72,  74,  75,  76,  78,  79,  80,  81,  83,  84,  85,  86,  87,  89,
    90,  91,  92,  93,  94,  96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
   107, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
   124, 125, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
   138, 139, 140, 141, 142, 143, 144, 145, 146, 146, 147, 148, 149, 150, 151, 152,
   152, 153, 154, 155, 156, 157, 158, 158, 159, 160, 161, 162, 162, 163, 164, 165,
   166, 167, 167, 168, 169, 170, 171, 171, 172, 173, 174, 175, 175, 176, 177, 178,
   178, 179, 180, 181, 181, 182, 183, 184, 185, 185, 186, 187, 188, 188, 189, 190,
   191, 191, 192, 193, 194, 194, 195, 196, 196, 197, 198, 199, 199, 200, 201, 202,
   202, 203, 204, 204, 205, 206, 207, 207, 208, 209, 209, 210, 211, 211, 212, 213,
   214, 214, 215, 216, 216, 217, 218, 218, 219, 220, 220, 221, 222, 222, 223, 224,
   225, 225, 226, 227, 227, 228, 229, 229, 230, 231, 231, 232, 233, 233, 234, 235,
   235, 236, 236, 237, 238, 238, 239, 240, 240, 241, 242, 242, 243, 244, 244, 245,
   246, 246, 247, 247, 248, 249, 249, 250, 251, 251, 252, 252, 253, 254, 254, 255,
#endif

#if 1
/* inverse gamma 1.8 */
     0,  12,  18,  22,  26,  29,  32,  35,  38,  41,  43,  45,  47,  50,  52,  54,
    56,  58,  59,  61,  63,  65,  66,  68,  70,  71,  73,  74,  76,  77,  79,  80,
    81,  83,  84,  86,  87,  88,  90,  91,  92,  93,  95,  96,  97,  98,  99, 101,
   102, 103, 104, 105, 106, 107, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
   119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
   135, 136, 137, 138, 138, 139, 140, 141, 142, 143, 144, 145, 146, 146, 147, 148,
   149, 150, 151, 152, 152, 153, 154, 155, 156, 157, 157, 158, 159, 160, 161, 161,
   162, 163, 164, 165, 165, 166, 167, 168, 168, 169, 170, 171, 172, 172, 173, 174,
   175, 175, 176, 177, 178, 178, 179, 180, 180, 181, 182, 183, 183, 184, 185, 186,
   186, 187, 188, 188, 189, 190, 190, 191, 192, 193, 193, 194, 195, 195, 196, 197,
   197, 198, 199, 199, 200, 201, 201, 202, 203, 203, 204, 205, 205, 206, 207, 207,
   208, 209, 209, 210, 211, 211, 212, 212, 213, 214, 214, 215, 216, 216, 217, 218,
   218, 219, 219, 220, 221, 221, 222, 222, 223, 224, 224, 225, 226, 226, 227, 227,
   228, 229, 229, 230, 230, 231, 232, 232, 233, 233, 234, 235, 235, 236, 236, 237,
   237, 238, 239, 239, 240, 240, 241, 242, 242, 243, 243, 244, 244, 245, 246, 246,
   247, 247, 248, 248, 249, 249, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255,
#endif
   
#if 0
/* inverse gamma 2.2 */
     0,  21,  29,  35,  39,  43,  47,  51,  54,  57,  59,  62,  64,  67,  69,  71,
    73,  75,  77,  79,  81,  83,  85,  86,  88,  90,  91,  93,  94,  96,  97,  99,
   100, 102, 103, 104, 106, 107, 108, 110, 111, 112, 113, 114, 116, 117, 118, 119,
   120, 121, 122, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
   137, 138, 139, 140, 141, 142, 143, 143, 144, 145, 146, 147, 148, 149, 150, 150,
   151, 152, 153, 154, 155, 156, 156, 157, 158, 159, 160, 160, 161, 162, 163, 164,
   164, 165, 166, 167, 167, 168, 169, 170, 170, 171, 172, 173, 173, 174, 175, 175,
   176, 177, 178, 178, 179, 180, 180, 181, 182, 182, 183, 184, 184, 185, 186, 186,
   187, 188, 188, 189, 190, 190, 191, 192, 192, 193, 193, 194, 195, 195, 196, 197,
   197, 198, 198, 199, 200, 200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206,
   207, 207, 208, 208, 209, 210, 210, 211, 211, 212, 212, 213, 214, 214, 215, 215,
   216, 216, 217, 217, 218, 219, 219, 220, 220, 221, 221, 222, 222, 223, 223, 224,
   224, 225, 225, 226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 231, 232, 232,
   233, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238, 239, 239, 240, 240,
   241, 241, 242, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247, 248,
   248, 249, 249, 250, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255, 255,
#endif

  };

void cleanup(void)
{
		fprintf(stderr,"tek4404: cleanup\015");
		
#ifdef USE_EVENTS
		EventDisable();
		SetKBCode(1);
#endif
		stty(fileno(stdin), &orig_term_settings);
}

void sig_handler(int sig)
{
		fprintf(stderr,"tek4404: signal caught: %d\015", sig);

		cleanup();
		exit(-1);
}

void DG_Init()
{
	short i;
	
	// FIXME: not working
	
	I_AtExit(cleanup, TRUE);

	screen = InitGraphics(FALSE);

#ifdef USE_EVENTS
	EventEnable();
	SetKBCode(0);
#endif

	for(i=0; i<256; i++)
	{
		fatbits[0][i] = halftone[0][i & 3] | (halftone[0][((i>>2) & 3)]<<2) | (halftone[0][((i>>4) & 3)]<<4) | (halftone[0][((i>>6) & 3)]<<6);
		fatbits[1][i] = halftone[1][i & 3] | (halftone[1][((i>>2) & 3)]<<2) | (halftone[1][((i>>4) & 3)]<<4) | (halftone[1][((i>>6) & 3)]<<6);
	}
}

void DG_SetWindowTitle(const char *title)
{
	printf("title: %s\015", title);
}


void DG_DrawFrame()
{
	register unsigned char *dst;
	register unsigned char *src;
	register short v,x,y;

	/* now we have the palette */
	if (initlookup == 0)
	{
		int i,lum;
		unsigned char gamma[256];
		
		/* now we have palette, calc 2-bit gray from gamma corrected average */
		for (i=0; i<256; i++)
		{
				/* BGR */
				lum = (gamma_lut[colors[i][2]] * 3 + gamma_lut[colors[i][1]] * 5 + gamma_lut[colors[i][0]] * 2) / 10;
				// rely on doom gamma
				//lum = (colors[i][2] * 3 + colors[i][1] * 5 + colors[i][0] * 2) / 10;
				
				// inverse video
				lum = 255 - lum;
				
				lum = (lum >> 6) & 3;
		
				pal2grey[0][i] = lum << 0;
				pal2grey[1][i] = lum << 2;
				pal2grey[2][i] = lum << 4;
				pal2grey[3][i] = lum << 6;
		}
	
		initlookup = 1;
	}
	
	dst = (unsigned char *)screen->addr;
	src = (unsigned char *)DG_ScreenBuffer;
	for(y=0; y<DOOMGENERIC_RESY; y++)
	{
		x = DOOMGENERIC_RESX / 4;
		while(x-- > 0)
		{
			v = pal2grey[3][*src++] | pal2grey[2][*src++] | pal2grey[1][*src++] | pal2grey[0][*src++];
			dst[0] = fatbits[0][v];
			dst[128] = fatbits[1][v];
			dst++;
		}
		/* 2x2 fat pixels */
		dst += 128 * 2 - (DOOMGENERIC_RESX / 4) ;
	}
}

/* NB slow clock by ... */
#define SLOWCLK 4

void DG_SleepMs(unsigned int ms)
{
	unsigned int future = EGetTime() + (ms << SLOWCLK);
	
	while (future > EGetTime());
}

unsigned int DG_GetTicksMs()
{
	return (EGetTime() >> SLOWCLK);
}



int DG_GetKey(int* pressed, unsigned char* key)
{
	static char lastkey;
	
#ifdef USE_EVENTS
	readkeyboardevents();
		
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
	{
		//key queue is empty

		return 0;
	}
	else
	{
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*pressed = keyData >> 8;
		*key = keyData & 0xFF;

		return 1;
	}
#endif

#ifdef USE_STDIN
	gtty(fileno(stdin), &term_settings);
	if (term_settings.sg_speed & INCHR)
	{
		char buffer[4];
		int rc = read(fileno(stdin), buffer, 1);
		if (rc == 1)
		{
			*pressed = 1;
			if (buffer[0] == 0x1b)
			{
				*key = KEY_ESCAPE;
			}
			else
			if (buffer[0] == 0x0d)
			{
				*key = KEY_ENTER;
			}
			else
			if (buffer[0] == 'a')
			{
				*key = KEY_LEFTARROW;
			}
			else
			if (buffer[0] == 'd')
			{
				*key = KEY_RIGHTARROW;
			}
			else
			if (buffer[0] == 'w')
			{
				*key = KEY_UPARROW;
			}
			else
			if (buffer[0] == 's')
			{
				*key = KEY_DOWNARROW;
			}
			else
			if (buffer[0] == ' ')
			{
				*key = KEY_USE;
			}
			else
			{
				*key = buffer[0];
			}
			
			lastkey = *key;
			return 1;
		}
	}
	else
	if (lastkey)
	{
		/* force release of key */
		*key = lastkey;
		*pressed = 0;
		
		lastkey = 0;
		return 1;
	}
	else
	{
	
	}
	
	if (GetButtons() & M_LEFT)
	{
		*pressed = 1;
		*key = KEY_FIRE;
		
		lastkey = *key;
		return 1;
	}
#endif

	return 0;
}

int main(int argc, char **argv)
{
	fixed_t a = 10 << 16;
	fixed_t b = 50 << 16;
	fixed_t c;
	
	c = FixedDiv(a,b);
	printf("c = %8.8x\015",c);
	c = FixedDiv(10<<16, 1<<14);
	printf("c = %8.8x\015",c);
	c = FixedDiv(200<<16, 1<<14);
	printf("c = %8.8x\015",c);
	c = FixedDiv(300<<16, 1<<14);
	printf("c = %8.8x\015",c);

getchar();

		// limit stdout to 5 last lines
		printf("\033[28;5r");


    doomgeneric_Create(argc, argv);

		gtty(fileno(stdin), &orig_term_settings);
		term_settings = orig_term_settings;
		term_settings.sg_flag |= CBREAK | ECHO;
//		term_settings.sg_flag &= ~ECHO;
		stty(fileno(stdin), &term_settings);

		signal(SIGDIV, sig_handler);
		signal(SIGHUP, sig_handler);
		signal(SIGKILL, sig_handler);
		signal(SIGINT, sig_handler);
		signal(SIGTERM, sig_handler);

    while (1)
    {
        doomgeneric_Tick();
    }

#ifdef USE_EVENTS
		EventDisable();
		SetKBCode(1);
#endif

		stty(fileno(stdin), &orig_term_settings);

    return 0;
}

