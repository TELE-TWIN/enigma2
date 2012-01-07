#include <lib/gdi/lcd.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if defined(HAVE_DBOX_FP_H) && defined(HAVE_DBOX_LCD_KS0713_H)
#include <dbox/fp.h>
#include <dbox/lcd-ks0713.h>
#else
#define NO_LCD 1
#endif

#include <lib/gdi/esize.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>
#ifdef HAVE_TEXTLCD
	#include <lib/base/estring.h>
#endif
#include <lib/gdi/glcddc.h>

eDBoxLCD *eDBoxLCD::instance;

eLCD::eLCD()
{
	lcdfd = -1;
	locked=0;
}

void eLCD::setSize(int xres, int yres, int bpp)
{
	res = eSize(xres, yres);
	_buffer=new unsigned char[xres * yres * bpp/8];
	memset(_buffer, 0, res.height()*res.width()*bpp/8);
	_stride=res.width()*bpp/8;
	eDebug("lcd buffer %p %d bytes, stride %d", _buffer, xres*yres*bpp/8, _stride);
}

eLCD::~eLCD()
{
	delete [] _buffer;
}

int eLCD::lock()
{
	if (locked)
		return -1;

	locked=1;
	return lcdfd;
}

void eLCD::unlock()
{
	locked=0;
}

#ifdef HAVE_TEXTLCD
void eLCD::renderText(ePoint start, const char *text)
{
	if (lcdfd >= 0 && start.y() < 5)
	{
		std::string message = text;
		message = replace_all(message, "\n", " ");
		::write(lcdfd, message.c_str(), message.size());
	}
}
#endif

eDBoxLCD::eDBoxLCD()
{
	int xres=132, yres=64, bpp=8;
	is_oled = 0;
#ifndef NO_LCD
	lcdfd = open("/proc/vfd", O_RDWR);
#endif
	instance=this;

	setSize(xres, yres, bpp);
}

void eDBoxLCD::setInverted(unsigned char inv)
{
	inverted=inv;
	update();
}

int eDBoxLCD::setLCDContrast(int contrast)
{
	return(0);
}

int eDBoxLCD::setLCDBrightness(int brightness)
{
	return(0);
}

eDBoxLCD::~eDBoxLCD()
{
	if (lcdfd>=0)
	{
		close(lcdfd);
		lcdfd=-1;
	}
}

eDBoxLCD *eDBoxLCD::getInstance()
{
	return instance;
}

void eDBoxLCD::update()
{
#ifndef HAVE_TEXTLCD
	if (lcdfd >= 0)
	{
		if (is_oled == 0 || is_oled == 2)
		{
			unsigned char raw[132*8];
			int x, y, yy;
			for (y=0; y<8; y++)
			{
				for (x=0; x<132; x++)
				{
					int pix=0;
					for (yy=0; yy<8; yy++)
					{
						pix|=(_buffer[(y*8+yy)*132+x]>=108)<<yy;
					}
					raw[y*132+x]=(pix^inverted);
				}
			}
			write(lcdfd, raw, 132*8);
		}
		else if (is_oled == 3)
			write(lcdfd, _buffer, _stride * res.height());
		else /* is_oled == 1 */
		{
			unsigned char raw[64*64];
			int x, y;
			memset(raw, 0, 64*64);
			for (y=0; y<64; y++)
			{
				int pix=0;
				for (x=0; x<128 / 2; x++)
				{
					pix = (_buffer[y*132 + x * 2 + 2] & 0xF0) |(_buffer[y*132 + x * 2 + 1 + 2] >> 4);
					if (inverted)
						pix = 0xFF - pix;
					raw[y*64+x] = pix;
				}
			}
			write(lcdfd, raw, 64*64);
		}
	}
#endif
}
