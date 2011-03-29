#ifndef __CMIXER_H__
#define __CMIXER_H__

#include <windows.h>
#include <mmsystem.h>

class CMixer 
{
public:
	CMixer();
	CMixer(const int VolRange);
	virtual ~CMixer();
	enum MixerDeice
	{
		SPEAKERS=0,
		WAVEOUT,
		SYNTHESIZER,
		MICROPHONE
	};

	bool GetMute(MixerDeice dev);   //检查设备是否静音
	bool SetMute(MixerDeice dev,bool vol);     //设置设备静音
	bool SetVolume(MixerDeice dev,long vol);   //设置设备的音量
	unsigned GetVolume(MixerDeice dev);    //得到设备的音量dev=0主音量，1WAVE ,2MIDI ,3 LINE IN
	bool SetGetDevVolume(bool bIsSet,DWORD & dwVolume);

private:
	bool GetVolumeControl(HMIXER hmixer ,long componentType,long ctrlType,MIXERCONTROL* mxc);
	bool SetVolumeValue(HMIXER hmixer ,MIXERCONTROL *mxc, long volume);
	bool SetMuteValue(HMIXER hmixer ,MIXERCONTROL *mxc, bool mute);
	unsigned GetVolumeValue(HMIXER hmixer ,MIXERCONTROL *mxc);
	long GetMuteValue(HMIXER hmixer ,MIXERCONTROL *mxc);
	

private:
	long m_VolRange ;
};

#endif