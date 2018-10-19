/*
 * rpihddevice - VDR HD output device for Raspberry Pi
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

#ifndef TOOLS_H
#define TOOLS_H

#include <errno.h>
#include <math.h>
#include <poll.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

//	vdr/tools.h

typedef unsigned char uchar;

#define SECSINDAY  86400

#define KILOBYTE(n) ((n) * 1024)
#define MEGABYTE(n) ((n) * 1024LL * 1024LL)

#define MALLOC(type, size)  (type *)malloc(sizeof(type) * (size))

class cString {
private:
  char *s;
public:
  cString(const char *S = NULL, bool TakePointer = false);
  cString(const char *S, const char *To); ///< Copies S up to To (exclusive). To must be a valid pointer into S. If To is NULL, everything is copied.
  cString(const cString &String);
  virtual ~cString();
  operator const void * () const { return s; } // to catch cases where operator*() should be used
  operator const char * () const { return s; } // for use in (const char *) context
  const char * operator*() const { return s; } // for use in (const void *) context (printf() etc.)
  cString &operator=(const cString &String);
  cString &operator=(const char *String);
  cString &Truncate(int Index); ///< Truncate the string at the given Index (if Index is < 0 it is counted from the end of the string).
  cString &CompactChars(char c); ///< Compact any sequence of characters 'c' to a single character, and strip all of them from the beginning and end of this string.
  static cString sprintf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
  static cString vsprintf(const char *fmt, va_list &ap);
  };

uchar *RgbToJpeg(uchar *Mem, int Width, int Height, int &Size, int Quality = 100);
    ///< Converts the given Memory to a JPEG image and returns a pointer
    ///< to the resulting image. Mem must point to a data block of exactly
    ///< (Width * Height) triplets of RGB image data bytes. Upon return, Size
    ///< will hold the number of bytes of the resulting JPEG data.
    ///< Quality can be in the range 0..100 and controls the quality of the
    ///< resulting image, where 100 is "best". The caller takes ownership of
    ///< the result and has to delete it once it is no longer needed.
    ///< The result may be NULL in case of an error.

class cTimeMs {
private:
  uint64_t begin;
public:
  cTimeMs(int Ms = 0);
      ///< Creates a timer with ms resolution and an initial timeout of Ms.
      ///< If Ms is negative the timer is not initialized with the current
      ///< time.
  static uint64_t Now(void);
  void Set(int Ms = 0);
  bool TimedOut(void) const;
  uint64_t Elapsed(void) const;
  };

class cPoller {
private:
  enum { MaxPollFiles = 16 };
  pollfd pfd[MaxPollFiles];
  int numFileHandles;
public:
  cPoller(int FileHandle = -1, bool Out = false);
  bool Add(int FileHandle, bool Out);
  bool Poll(int TimeoutMs = 0);
  };  

//	vdr/remux.h

#define MAX33BIT  0x00000001FFFFFFFFLL // max. possible value with 33 bit

inline bool PesLongEnough(int Length)
{
  return Length >= 6;
}

inline bool PesHasLength(const uchar *p)
{
  return p[4] | p[5];
}

inline int PesLength(const uchar *p)
{
  return 6 + p[4] * 256 + p[5];
}

inline int PesPayloadOffset(const uchar *p)
{
  return 9 + p[8];
}

inline bool PesHasPts(const uchar *p)
{
  return (p[7] & 0x80) && p[8] >= 5;
}

inline int64_t PesGetPts(const uchar *p)
{
  return ((((int64_t)p[ 9]) & 0x0E) << 29) |
         (( (int64_t)p[10])         << 22) |
         ((((int64_t)p[11]) & 0xFE) << 14) |
         (( (int64_t)p[12])         <<  7) |
         ((((int64_t)p[13]) & 0xFE) >>  1);
}

int64_t PtsDiff(int64_t Pts1, int64_t Pts2);
       ///< Returns the difference between two PTS values. The result of Pts2 - Pts1
       ///< is the actual number of 90kHz time ticks that pass from Pts1 to Pts2,
       ///< properly taking into account the 33bit wrap around. If Pts2 is "before"
       ///< Pts1, the result is negative.

//	vdr/device.h

// Note that VDR itself always uses pmAudioVideo when replaying a recording!
enum ePlayMode { pmNone,           // audio/video from decoder
                 pmAudioVideo,     // audio/video from player
                 pmAudioOnly,      // audio only from player, video from decoder
                 pmAudioOnlyBlack, // audio only from player, no video (black screen)
                 pmVideoOnly,      // video only from player, audio from decoder
                 pmExtern_THIS_SHOULD_BE_AVOIDED
                 // external player (e.g. MPlayer), release the device
                 // WARNING: USE THIS MODE ONLY AS A LAST RESORT, IF YOU
                 // ABSOLUTELY, POSITIVELY CAN'T IMPLEMENT YOUR PLAYER
                 // THE WAY IT IS SUPPOSED TO WORK. FORCING THE DEVICE
                 // TO RELEASE ITS FILES HANDLES (OR WHATEVER RESOURCES
                 // IT MAY USE) TO ALLOW AN EXTERNAL PLAYER TO ACCESS
                 // THEM MEANS THAT SUCH A PLAYER WILL NEED TO HAVE
                 // DETAILED KNOWLEDGE ABOUT THE INTERNALS OF THE DEVICE
                 // IN USE. AS A CONSEQUENCE, YOUR PLAYER MAY NOT WORK
                 // IF A PARTICULAR VDR INSTALLATION USES A DEVICE NOT
                 // KNOWN TO YOUR PLAYER.
               };
			   
//	------------------------------------------------------------------------------------------------

class cVideoResolution
{
public:

	enum eResolution {
		eDontChange = 0,
		eFollowVideo,
		e480,
		e480w,
		e576,
		e576w,
		e720,
		e1080
	};

	static const char* Str(eResolution resolution) {
		return	(resolution == eDontChange)  ? "don't change" :
				(resolution == eFollowVideo) ? "follow video" :
				(resolution == e480)         ? "480"          :
				(resolution == e480w)        ? "480w"         :
				(resolution == e576)         ? "576"          :
				(resolution == e576w)        ? "576w"         :
				(resolution == e720)         ? "720"          :
				(resolution == e1080)        ? "1080"         :	"unknown";
	}
};

class cVideoFrameRate
{
public:

	enum eFrameRate {
		eDontChange = 0,
		eFollowVideo,
		e24p,
		e25p,
		e30p,
		e50i,
		e50p,
		e60i,
		e60p
	};

	static const char* Str(eFrameRate frameRate) {
		return	(frameRate == eDontChange)  ? "don't change" :
				(frameRate == eFollowVideo) ? "follow video" :
				(frameRate == e24p)         ? "p24"          :
				(frameRate == e25p)         ? "p25"          :
				(frameRate == e30p)         ? "p30"          :
				(frameRate == e50i)         ? "i50"          :
				(frameRate == e50p)         ? "p50"          :
				(frameRate == e60i)         ? "i60"          :
				(frameRate == e60p)         ? "p60"          : "unknown";
	}
};

class cVideoFraming
{
public:

	enum eFraming {
		eFrame,
		eCut,
		eStretch
	};

	static const char* Str(eFraming framing) {
		return  (framing == eFrame)   ? "frame"   :
				(framing == eCut)     ? "cut"     :
				(framing == eStretch) ? "stretch" : "unknown";
	}
};

class cAudioCodec
{
public:

	enum eCodec {
		ePCM,
		eMPG,
		eAC3,
		eEAC3,
		eAAC,
		eAAC_LATM,
		eDTS,
		eNumCodecs,
		eInvalid
	};

	static const char* Str(eCodec codec) {
		return  (codec == ePCM)      ? "PCM"      :
				(codec == eMPG)      ? "MPEG"     :
				(codec == eAC3)      ? "AC3"      :
				(codec == eEAC3)     ? "E-AC3"    :
				(codec == eAAC)      ? "AAC"      :
				(codec == eAAC_LATM) ? "AAC-LATM" :
				(codec == eDTS)      ? "DTS"      : "unknown";
	}
};

class cAudioFormat
{
public:

	enum eFormat {
		ePassThrough,
		eMultiChannelPCM,
		eStereoPCM
	};

	static const char* Str(eFormat format) {
		return  (format == ePassThrough)     ? "pass through"      :
				(format == eMultiChannelPCM) ? "multi channel PCM" :
				(format == eStereoPCM)       ? "stereo PCM"        : "unknown";
	}
};

class cVideoCodec
{
public:

	enum eCodec {
		eMPEG2,
		eH264,
		eNumCodecs,
		eInvalid
	};

	static const char* Str(eCodec codec) {
		return  (codec == eMPEG2) ? "MPEG2" :
				(codec == eH264)  ? "H264"  : "unknown";
	}
};

class cRpiAudioPort
{
public:

	enum ePort {
		eLocal,
		eHDMI
	};

	static const char* Str(ePort port) {
		return 	(port == eLocal) ? "local" :
				(port == eHDMI)  ? "HDMI"  : "unknown";
	}
};

class cScanMode
{
public:

	enum eMode {
		eProgressive,
		eTopFieldFirst,
		eBottomFieldFirst
	};

	static const char* Str(eMode mode) {
		return 	(mode == eProgressive)      ? "progressive"      :
				(mode == eTopFieldFirst)    ? "interlaced (tff)" :
				(mode == eBottomFieldFirst) ? "interlaced (bff)" : "unknown";
	}

	static const bool Interlaced(eMode mode) {
		return mode != eProgressive;
	}
};

class cVideoFrameFormat
{
public:

	cVideoFrameFormat() : width(0), height(0), frameRate(0),
		scanMode(cScanMode::eProgressive), pixelWidth(0), pixelHeight(0) { };

	int width;
	int height;
	int frameRate;
	cScanMode::eMode scanMode;
	int pixelWidth;
	int pixelHeight;

	bool Interlaced(void) const {
		return cScanMode::Interlaced(scanMode);
	}
};

class cRational
{
public:

	cRational(double d);
	cRational(int _num, int _den) : num(_num), den(_den) { }

	bool Reduce(int max);

	int num;
	int den;

private:

	cRational();
	static int Gcd(int u, int v);
};

#endif
