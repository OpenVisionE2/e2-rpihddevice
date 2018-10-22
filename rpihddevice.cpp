/*
 * rpihddevice - Enigma2 output device for Raspberry Pi
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
#include "omxdecoder.h"
#include "rpisetup.h"
#include "rpidisplay.h"

static const char *VERSION        = "1.0.4";
static const char *DESCRIPTION    = "HD output device for Raspberry Pi";

class cRpiHdDevice /*: public cPlugin*/
{
private:

	cOmxDevice *m_device;

public:
	cRpiHdDevice(void);
	virtual ~cRpiHdDevice();
	virtual const char *Version(void) { return VERSION; }
	virtual const char *Description(void) { return DESCRIPTION; }
	virtual const char *CommandLineHelp(void);
	virtual bool ProcessArgs(int argc, char *argv[]);
	virtual bool Initialize(void);
	virtual bool Start(void);
	virtual void Stop(void);
	virtual void Housekeeping(void) {}
	virtual const char *MainMenuEntry(void) { return NULL; }
//	virtual cOsdObject *MainMenuAction(void) { return NULL; }
	virtual cMenuSetupPage *SetupMenu(void);
	virtual bool SetupParse(const char *Name, const char *Value);
};

cRpiHdDevice::cRpiHdDevice(void) : 
	m_device(0)
{
}

cRpiHdDevice::~cRpiHdDevice()
{
	cRpiSetup::DropInstance();
	cRpiDisplay::DropInstance();
}

bool cRpiHdDevice::Initialize(void)
{
	if (!cRpiSetup::HwInit())
		return false;

	// test whether MPEG2 license is available
	if (!cRpiSetup::IsVideoCodecSupported(cVideoCodec::eMPEG2))
		eLog(3, "[cRpiHdDevice] MPEG2 video decoder not enabled!");

	m_device = new cOmxDevice(cRpiDisplay::GetId(), cRpiSetup::VideoLayer());

	if (m_device)
		return !m_device->Init();

	return false;
}

bool cRpiHdDevice::Start(void)
{
	return m_device->Start();
}

void cRpiHdDevice::Stop(void)
{
}

/*	Call to Setup Menu Page TO BE REMOVED on Enigma2
	ToDO List:
	-	Remove setup page
	-	Passed fixed setup value
	-	Remove fixed value and read from Enigma2	*/
cMenuSetupPage* cRpiHdDevice::SetupMenu(void)
{
	return cRpiSetup::GetInstance()->GetSetupPage();
}

bool cRpiHdDevice::SetupParse(const char *Name, const char *Value)
{
	return cRpiSetup::GetInstance()->Parse(Name, Value);
}
/*	Process Argument passed on start TO BE REMOVED or TUNED on Enigma2	*/
bool cRpiHdDevice::ProcessArgs(int argc, char *argv[])
{
	return cRpiSetup::GetInstance()->ProcessArgs(argc, argv);
}
/*	Show command line help TO BE REMOVED on Enigma2	*/
const char *cRpiHdDevice::CommandLineHelp(void)
{
	return cRpiSetup::GetInstance()->CommandLineHelp();
}

