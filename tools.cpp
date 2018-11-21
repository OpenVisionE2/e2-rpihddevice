/*
 * rpihddevice - Enigma2 rpihddevice library for Raspberry Pi
 * Copyright (C) 2014, 2015, 2016 Thomas Reufer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <limits.h>
#include <sys/time.h>
#include <time.h>
#include <utime.h>
#include <syslog.h>
#include "tools.h"
#include <algorithm>

extern "C" {
#ifdef boolean
#define HAVE_BOOLEAN
#endif
#include <jpeglib.h>
#undef boolean
}

/*
 * ffmpeg's implementation for rational numbers:
 * https://github.com/FFmpeg/FFmpeg/blob/master/libavutil/rational.c
 */

// --- cTimeMs ---------------------------------------------------------------

cTimeMs::cTimeMs(int Ms)
{
  if (Ms >= 0)
     Set(Ms);
  else
     begin = 0;
}

uint64_t cTimeMs::Now(void)
{
#if _POSIX_TIMERS > 0 && defined(_POSIX_MONOTONIC_CLOCK)
#define MIN_RESOLUTION 5 // ms
  static bool initialized = false;
  static bool monotonic = false;
  struct timespec tp;
  if (!initialized) {
     // check if monotonic timer is available and provides enough accurate resolution:
     if (clock_getres(CLOCK_MONOTONIC, &tp) == 0) {
        long Resolution = tp.tv_nsec;
        // require a minimum resolution:
        if (tp.tv_sec == 0 && tp.tv_nsec <= MIN_RESOLUTION * 1000000) {
           if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
              syslog(LOG_DEBUG, "[cTimeMs] using monotonic clock (resolution is %ld ns)", Resolution);
              monotonic = true;
              }
           else
              syslog(LOG_ERR, "[cTimeMs] clock_gettime(CLOCK_MONOTONIC) failed");
           }
        else
           syslog(LOG_DEBUG, "[cTimeMs] not using monotonic clock - resolution is too bad (%ld s %ld ns)", tp.tv_sec, tp.tv_nsec);
        }
     else
        syslog(LOG_ERR, "[cTimeMs] clock_getres(CLOCK_MONOTONIC) failed");
     initialized = true;
     }
  if (monotonic) {
     if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
        return (uint64_t(tp.tv_sec)) * 1000 + tp.tv_nsec / 1000000;
     syslog(LOG_ERR, "[cTimeMs] clock_gettime(CLOCK_MONOTONIC) failed");
     monotonic = false;
     // fall back to gettimeofday()
     }
#else
#  warning Posix monotonic clock not available
#endif
  struct timeval t;
  if (gettimeofday(&t, NULL) == 0)
     return (uint64_t(t.tv_sec)) * 1000 + t.tv_usec / 1000;
  return 0;
}

void cTimeMs::Set(int Ms)
{
  begin = Now() + Ms;
}

bool cTimeMs::TimedOut(void) const
{
  return Now() >= begin;
}

uint64_t cTimeMs::Elapsed(void) const
{
  return Now() - begin;
}


// --- cString ---------------------------------------------------------------

cString::cString(const char *S, bool TakePointer)
{
  s = TakePointer ? (char *)S : S ? strdup(S) : NULL;
}

cString::~cString()
{
  free(s);
}

cString cString::vsprintf(const char *fmt, va_list &ap)
{
  char *buffer;
  if (!fmt || vasprintf(&buffer, fmt, ap) < 0) {
     syslog(LOG_ERR, "[cString:] error in vasprintf('%s', ...)", fmt);
     buffer = strdup("???");
     }
  return cString(buffer, true);
}

// --- RgbToJpeg -------------------------------------------------------------

#define JPEGCOMPRESSMEM 500000

struct tJpegCompressData {
  int size;
  uchar *mem;
  };

static void JpegCompressInitDestination(j_compress_ptr cinfo)
{
  tJpegCompressData *jcd = (tJpegCompressData *)cinfo->client_data;
  if (jcd) {
     cinfo->dest->free_in_buffer = jcd->size = JPEGCOMPRESSMEM;
     cinfo->dest->next_output_byte = jcd->mem = MALLOC(uchar, jcd->size);
     }
}

static boolean JpegCompressEmptyOutputBuffer(j_compress_ptr cinfo)
{
  tJpegCompressData *jcd = (tJpegCompressData *)cinfo->client_data;
  if (jcd) {
     int Used = jcd->size;
     int NewSize = jcd->size + JPEGCOMPRESSMEM;
     if (uchar *NewBuffer = (uchar *)realloc(jcd->mem, NewSize)) {
        jcd->size = NewSize;
        jcd->mem = NewBuffer;
        }
     else {
        syslog(LOG_ERR, "ERROR: out of memory");
        return false;
        }
     if (jcd->mem) {
        cinfo->dest->next_output_byte = jcd->mem + Used;
        cinfo->dest->free_in_buffer = jcd->size - Used;
        return true;
        }
     }
  return false;
}

static void JpegCompressTermDestination(j_compress_ptr cinfo)
{
  tJpegCompressData *jcd = (tJpegCompressData *)cinfo->client_data;
  if (jcd) {
     int Used = cinfo->dest->next_output_byte - jcd->mem;
     if (Used < jcd->size) {
        if (uchar *NewBuffer = (uchar *)realloc(jcd->mem, Used)) {
           jcd->size = Used;
           jcd->mem = NewBuffer;
           }
        else
           syslog(LOG_ERR, "ERROR: out of memory");
        }
     }
}

uchar *RgbToJpeg(uchar *Mem, int Width, int Height, int &Size, int Quality)
{
  if (Quality < 0)
     Quality = 0;
  else if (Quality > 100)
     Quality = 100;

  jpeg_destination_mgr jdm;

  jdm.init_destination = JpegCompressInitDestination;
  jdm.empty_output_buffer = JpegCompressEmptyOutputBuffer;
  jdm.term_destination = JpegCompressTermDestination;

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  cinfo.dest = &jdm;
  tJpegCompressData jcd;
  cinfo.client_data = &jcd;
  cinfo.image_width = Width;
  cinfo.image_height = Height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, Quality, true);
  jpeg_start_compress(&cinfo, true);

  int rs = Width * 3;
  JSAMPROW rp[Height];
  for (int k = 0; k < Height; k++)
      rp[k] = &Mem[rs * k];
  jpeg_write_scanlines(&cinfo, rp, Height);
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  Size = jcd.size;
  return jcd.mem;
}

// --- cRational -------------------------------------------------------------

cRational::cRational(double d) :
	num(0), den(0)
{
	int exp;
	frexp(d, &exp);					/*	call to	vdr/tools.h		*/

	den = 1LL << (29 - std::max(exp - 1, 0));
	num = floor(d * den + 0.5);		/*	call to	vdr/tools.h		*/

	Reduce(INT_MAX);
}

bool cRational::Reduce(int max)
{
	cRational a0 = cRational(0, 1), a1 = cRational(1, 0);
	int sign = (num < 0) ^ (den < 0);
	if (int div = Gcd(abs(num), abs(den)))
	{
		num = abs(num) / div;
		den = abs(den) / div;
	}
	if (num <= max && den <= max)
	{
		a1 = cRational(num, den);
		den = 0;
	}
	while (den)
	{
		int x = num / den;
		int nextDen = num - den * x;
		cRational a2 = cRational(x * a1.num + a0.num, x * a1.den + a0.den);
		if (a2.num > max || a2.den > max)
		{
			if (a1.num)
				x = (max - a0.num) / a1.num;
			if (a1.den)
				x = std::min(x, (max - a0.den) / a1.den);
			if (den * (2 * x * a1.den + a0.den) > num * a1.den)
				a1 = cRational(x * a1.num + a0.num, x * a1.den + a0.den);
			break;
		}
		a0 = a1;
		a1 = a2;
		num = den;
		den = nextDen;
	}
	num = sign ? -a1.num : a1.num;
	den = a1.den;
	return den == 0;
}

/*
 * Stein's binary GCD algorithm:
 * https://en.wikipedia.org/wiki/Binary_GCD_algorithm
 */

int cRational::Gcd(int u, int v)
{
    if (u == v || v == 0)
        return u;

    if (u == 0)
        return v;

    // look for factors of 2
    if (~u & 1) // u is even
    {
        if (v & 1) // v is odd
            return Gcd(u >> 1, v);
        else // both u and v are even
            return Gcd(u >> 1, v >> 1) << 1;
    }

    if (~v & 1) // u is odd, v is even
        return Gcd(u, v >> 1);

    // reduce larger argument
    if (u > v)
        return Gcd((u - v) >> 1, v);

    return Gcd((v - u) >> 1, u);
}
