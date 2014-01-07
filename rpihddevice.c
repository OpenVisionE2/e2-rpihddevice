/*
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/config.h>

#include "ovgosd.h"
#include "omxdevice.h"
#include "setup.h"

static const char *VERSION        = "0.0.4";
static const char *DESCRIPTION    = "HD output device for Raspberry Pi";

class cDummyDevice : cDevice
{

public:

	cDummyDevice() { }
	virtual ~cDummyDevice() { }
    virtual bool HasDecoder(void) const { return true; }
    virtual bool SetPlayMode(ePlayMode PlayMode) { return true; }
    virtual int  PlayVideo(const uchar *Data, int Length) { return Length; }
    virtual int  PlayAudio(const uchar *Data, int Length, uchar Id) { return Length; }
    virtual bool Poll(cPoller &Poller, int TimeoutMs = 0) { return true; }
    virtual bool Flush(int TimeoutMs = 0) { return true; }
	virtual void GetOsdSize(int &Width, int &Height, double &PixelAspect)
		{ cRpiSetup::GetDisplaySize(Width, Height, PixelAspect); }

    bool Start(void) {return true;}

protected:
	virtual void MakePrimaryDevice(bool On) { if (On) new cRpiOsdProvider(); }

};

class cPluginRpiHdDevice : public cPlugin
{
private:

	cOmxDevice *m_device;

	static void OnPrimaryDevice(void) { new cRpiOsdProvider(); }

public:
	cPluginRpiHdDevice(void);
	virtual ~cPluginRpiHdDevice();
	virtual const char *Version(void) { return VERSION; }
	virtual const char *Description(void) { return DESCRIPTION; }
	virtual const char *CommandLineHelp(void) { return NULL; }
	virtual bool ProcessArgs(int argc, char *argv[]) { return true; }
	virtual bool Initialize(void);
	virtual bool Start(void);
	virtual void Stop(void);
	virtual void Housekeeping(void) {}
	virtual const char *MainMenuEntry(void) { return NULL; }
	virtual cOsdObject *MainMenuAction(void) { return NULL; }
	virtual cMenuSetupPage *SetupMenu(void);
	virtual bool SetupParse(const char *Name, const char *Value);
};

cPluginRpiHdDevice::cPluginRpiHdDevice(void) : 
	m_device(0)
{
	//new cDummyDevice();
}

cPluginRpiHdDevice::~cPluginRpiHdDevice()
{
	delete m_device;
	cRpiSetup::DropInstance();
}

bool cPluginRpiHdDevice::Initialize(void)
{
	if (!cRpiSetup::HwInit())
		return false;

	if (!cRpiSetup::IsVideoCodecSupported(cOmxDevice::eMPEG2))
		esyslog("rpihddevice: WARNING: MPEG2 video decoder not enabled!");

	m_device = new cOmxDevice(&OnPrimaryDevice);

	if (m_device)
		return (m_device->Init() == 0);

	return false;
}

bool cPluginRpiHdDevice::Start(void)
{
	return true;
}

void cPluginRpiHdDevice::Stop(void)
{
	if (m_device)
		m_device->DeInit();
}

cMenuSetupPage* cPluginRpiHdDevice::SetupMenu(void)
{
	return cRpiSetup::GetInstance()->GetSetupPage();
}

bool cPluginRpiHdDevice::SetupParse(const char *Name, const char *Value)
{
	return cRpiSetup::GetInstance()->Parse(Name, Value);
}

VDRPLUGINCREATOR(cPluginRpiHdDevice); // Don't touch this!
