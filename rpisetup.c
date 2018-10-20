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

#include "rpisetup.h"
#include "rpidisplay.h"

#include <lib/base/eerror.h>

#include <getopt.h>

#include <bcm_host.h>
#include "interface/vchiq_arm/vchiq_if.h"
#include "interface/vmcs_host/vc_tvservice.h"

/* ------------------------------------------------------------------------- */

class cRpiSetupPage
{

public:

	cRpiSetupPage(
			cRpiSetup::AudioParameters audio,
			cRpiSetup::VideoParameters video) :

		m_audio(audio),
		m_video(video)
	{
		m_audioPort[0] = "analog";
		m_audioPort[1] = "HDMI";

		m_audioFormat[0] = "pass through";
		m_audioFormat[1] = "multi channel PCM";
		m_audioFormat[2] = "stereo PCM";

		m_videoFraming[0] = "box";
		m_videoFraming[1] = "crop";
		m_videoFraming[2] = "stretch";

		m_videoResolution[0] = "default";
		m_videoResolution[1] = "follow video";
		m_videoResolution[2] = "720x480 (4:3)";
		m_videoResolution[3] = "720x480 (16:9)";
		m_videoResolution[4] = "720x576 (4:3)";
		m_videoResolution[5] = "720x576 (16:9)";
		m_videoResolution[6] = "1280x720";
		m_videoResolution[7] = "1920x1080";

		m_videoFrameRate[0] = "default";
		m_videoFrameRate[1] = "follow video";
		m_videoFrameRate[2] = "24p";
		m_videoFrameRate[3] = "25p";
		m_videoFrameRate[4] = "30p";
		m_videoFrameRate[5] = "50i";
		m_videoFrameRate[6] = "50p";
		m_videoFrameRate[7] = "60i";
		m_videoFrameRate[8] = "60p";

		m_useAdvancedDeinterlacer[0] = "no";
		m_useAdvancedDeinterlacer[1] = "for SD video only";
		m_useAdvancedDeinterlacer[2] = "always";

//		Setup();
	}
/*
	eOSState ProcessKey(eKeys Key)
	{
		int newAudioPort = m_audio.port;
		eOSState state = cMenuSetupPage::ProcessKey(Key);

		if (Key != kNone)
		{
			if (newAudioPort != m_audio.port)
				Setup();
		}

		return state;
	}

protected:

	virtual void Store(void)
	{
		SetupStore("AudioPort", m_audio.port);
		SetupStore("AudioFormat", m_audio.format);

		SetupStore("VideoFraming", m_video.framing);
		SetupStore("Resolution", m_video.resolution);
		SetupStore("FrameRate", m_video.frameRate);
		SetupStore("AdvancedDeinterlacer", m_video.advancedDeinterlacer);

		cRpiSetup::GetInstance()->Set(m_audio, m_video);
}

private:

	void Setup(void)
	{
		int current = Current();
		Clear();

		if (!cRpiDisplay::IsFixedMode())
		{
			Add(new cMenuEditStraItem(
				"Resolution", &m_video.resolution, 8, m_videoResolution));

			Add(new cMenuEditStraItem(
				"Frame Rate", &m_video.frameRate, 9, m_videoFrameRate));
		}
		if (cRpiDisplay::IsProgressive())
			Add(new cMenuEditStraItem(
					"Use Advanced Deinterlacer",
					&m_video.advancedDeinterlacer, 3,
					m_useAdvancedDeinterlacer));

		Add(new cMenuEditStraItem(
				"Video Framing", &m_video.framing, 3, m_videoFraming));

		Add(new cMenuEditStraItem(
				"Audio Port", &m_audio.port, 2, m_audioPort));

		if (m_audio.port == 1)
		{
			Add(new cMenuEditStraItem("Digital Audio Format"),
					&m_audio.format, 3, m_audioFormat));
		}

		SetCurrent(Get(current));
		Display();
	}
*/
	cRpiSetup::AudioParameters m_audio;
	cRpiSetup::VideoParameters m_video;

	const char *m_audioPort[2];
	const char *m_audioFormat[3];
	const char *m_videoFraming[3];
	const char *m_videoResolution[8];
	const char *m_videoFrameRate[9];
	const char *m_useAdvancedDeinterlacer[3];
};

/* ------------------------------------------------------------------------- */

cRpiSetup* cRpiSetup::s_instance = 0;

cRpiSetup* cRpiSetup::GetInstance(void)
{
	if (!s_instance)
		s_instance = new cRpiSetup();

	return s_instance;
}

void cRpiSetup::DropInstance(void)
{
	delete s_instance;
	s_instance = 0;

	bcm_host_deinit();
}

bool cRpiSetup::HwInit(void)
{
	cRpiSetup* instance = GetInstance();
	if (!instance)
		return false;

	bcm_host_init();

	if (!vc_gencmd_send("codec_enabled MPG2"))
	{
		char buffer[1024];
		if (!vc_gencmd_read_response(buffer, sizeof(buffer)))
		{
			if (!strcasecmp(buffer,"MPG2=enabled"))
				GetInstance()->m_mpeg2Enabled = true;
		}
	}

	int width, height;
	if (!cRpiDisplay::GetSize(width, height))
	{
		eLog(2, "[cRpiSetup] HwInit() done, display size is %dx%d", width, height);
	}
	else
		eLog(1, "[cRpiSetup] failed to get video port information!");

	return true;
}

void cRpiSetup::SetAudioSetupChangedCallback(void (*callback)(void*), void* data)
{
	GetInstance()->m_onAudioSetupChanged = callback;
	GetInstance()->m_onAudioSetupChangedData = data;
}

void cRpiSetup::SetVideoSetupChangedCallback(void (*callback)(void*), void* data)
{
	GetInstance()->m_onVideoSetupChanged = callback;
	GetInstance()->m_onVideoSetupChangedData = data;
}

bool cRpiSetup::IsAudioFormatSupported(cAudioCodec::eCodec codec,
		int channels, int samplingRate)
{
	// MPEG-1 layer 2 audio pass-through not supported by audio render
	// and AAC audio pass-through not yet working
	if (codec == cAudioCodec::eMPG || codec == cAudioCodec::eAAC)
		return false;

	if (channels < 2 || channels > 6)
		return false;

	switch (GetAudioFormat())
	{
	case cAudioFormat::ePassThrough:
		return (vc_tv_hdmi_audio_supported(
					codec == cAudioCodec::eMPG  ? EDID_AudioFormat_eMPEG1 :
					codec == cAudioCodec::eAC3  ? EDID_AudioFormat_eAC3   :
					codec == cAudioCodec::eEAC3 ? EDID_AudioFormat_eEAC3  :
					codec == cAudioCodec::eAAC  ? EDID_AudioFormat_eAAC   :
					codec == cAudioCodec::eDTS  ? EDID_AudioFormat_eDTS   :
							EDID_AudioFormat_ePCM, channels,
					samplingRate ==  32000 ? EDID_AudioSampleRate_e32KHz  :
					samplingRate ==  44100 ? EDID_AudioSampleRate_e44KHz  :
					samplingRate ==  88200 ? EDID_AudioSampleRate_e88KHz  :
					samplingRate ==  96000 ? EDID_AudioSampleRate_e96KHz  :
					samplingRate == 176000 ? EDID_AudioSampleRate_e176KHz :
					samplingRate == 192000 ? EDID_AudioSampleRate_e192KHz :
							EDID_AudioSampleRate_e48KHz,
							EDID_AudioSampleSize_16bit) == 0);

	case cAudioFormat::eMultiChannelPCM:
		return codec == cAudioCodec::ePCM;

	default:
	case cAudioFormat::eStereoPCM:
		return codec == cAudioCodec::ePCM && channels == 2;
	}
}

void cRpiSetup::SetHDMIChannelMapping(bool passthrough, int channels)
{
	char command[80], response[80];

	sprintf(command, "hdmi_stream_channels %d", passthrough ? 1 : 0);
	vc_gencmd(response, sizeof(response), command);

	uint32_t channel_map = 0;

	if (!passthrough && channels > 0 && channels <= 6)
	{
		const unsigned char ch_mapping[6][8] =
		{
			{ 0, 0, 0, 0, 0, 0, 0, 0 }, // not supported
			{ 1, 2, 0, 0, 0, 0, 0, 0 }, // 2.0
			{ 1, 2, 4, 0, 0, 0, 0, 0 }, // 2.1
			{ 0, 0, 0, 0, 0, 0, 0, 0 }, // not supported
			{ 0, 0, 0, 0, 0, 0, 0, 0 }, // not supported
			{ 1, 2, 4, 3, 5, 6, 0, 0 }, // 5.1
		};

		// speaker layout according CEA 861, Table 28: Audio InfoFrame, byte 4
		const unsigned char cea_map[] =
		{
			0xff,	// not supported
			0x00,	// 2.0
			0x01,	// 2.1
			0xff,	// not supported
			0xff,	// not supported
			0x0b	// 5.1
		};

		for (int ch = 0; ch < channels; ch++)
			if (ch_mapping[channels - 1][ch])
				channel_map |= (ch_mapping[channels - 1][ch] - 1) << (3 * ch);

		channel_map |= cea_map[channels - 1] << 24;
	}

	sprintf(command, "hdmi_channel_map 0x%08x", channel_map);
	vc_gencmd(response, sizeof(response), command);
}
/*
cMenuSetupPage* cRpiSetup::GetSetupPage(void)
{
	return new cRpiSetupPage(m_audio, m_video);
}
*/
bool cRpiSetup::Parse(const char *name, const char *value)
{
	if (!strcasecmp(name, "AudioPort"))
		m_audio.port = atoi(value);
	else if (!strcasecmp(name, "AudioFormat"))
		m_audio.format = atoi(value);
	else if (!strcasecmp(name, "VideoFraming"))
		m_video.framing = atoi(value);
	else if (!strcasecmp(name, "Resolution"))
		m_video.resolution = atoi(value);
	else if (!strcasecmp(name, "FrameRate"))
		m_video.frameRate = atoi(value);
	else if (!strcasecmp(name, "AdvancedDeinterlacer"))
		m_video.advancedDeinterlacer = atoi(value);
	else return false;

	return true;
}

void cRpiSetup::Set(AudioParameters audio, VideoParameters video)
{
	if (audio != m_audio)
	{
		m_audio = audio;
		if (m_onAudioSetupChanged)
			m_onAudioSetupChanged(m_onAudioSetupChangedData);
	}

	if (video != m_video)
	{
		m_video = video;
		if (m_onVideoSetupChanged)
			m_onVideoSetupChanged(m_onVideoSetupChangedData);
	}

}

bool cRpiSetup::ProcessArgs(int argc, char *argv[])
{
	const int cDisplayOpt = 0x100;
	static struct option long_options[] = {		
			{ "display",     required_argument, NULL, cDisplayOpt },
			{ "video-layer", required_argument, NULL, 'v'         },
			{ 0, 0, 0, 0 }
	};
	int c;
	while ((c = getopt_long(argc, argv, "do:v:", long_options, NULL)) != -1)
	{
		switch (c)
		{
		case 'v':
			m_plugin.videoLayer = atoi(optarg);
			break;
		case cDisplayOpt:
		{
			int d = atoi(optarg);
			switch (d)
			{
			case 0:
			case 4:
			case 5:
			case 6:
				m_plugin.display = d;
				break;
			default:
				eLog(1, "[cRpiSetup] invalid device id (%d), using default display!", d);
				break;
			}
		}
			break;
		default:
			return false;
		}
	}
	eDebug("[cRpiSetup] dispmanx layers: video=%d, display=%d",
			m_plugin.videoLayer, m_plugin.display);

	return true;
}

const char *cRpiSetup::CommandLineHelp(void)
{
	return	
			"  -v,       --video-layer  dispmanx layer for video (default 0)\n"
			"            --display      display used for output:\n"
			"                           0: default display (default)\n"
			"                           4: LCD\n"
			"                           5: TV/HDMI\n"
			"                           6: non-default display\n";
}
