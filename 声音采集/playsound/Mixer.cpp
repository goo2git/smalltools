// Mixer.cpp: implementation of the CMixer class.
//
//#include "stdwx.h"
#include "Mixer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMixer::CMixer()
{
	m_VolRange = 100;
}

CMixer::~CMixer()
{

}

CMixer::CMixer(const int VolRange)
{
	m_VolRange = VolRange;
}
//----------------------------设定音量---------------------------------------
bool CMixer::GetVolumeControl( HMIXER hmixer ,long componentType,long ctrlType,MIXERCONTROL* mxc )
{
	MIXERLINECONTROLS mxlc;
	MIXERLINE mxl;
	bool exist = false;
	mxl.cbStruct = sizeof(mxl);
	mxl.dwComponentType = componentType;

	if (componentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
	{//获取录音麦克风设备
		mxl.dwComponentType   =   MIXERLINE_COMPONENTTYPE_DST_WAVEIN;   
		//得到录制总线中的连接数   
		mixerGetLineInfo((HMIXEROBJ)hmixer, &mxl, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
		//将连接数保存   
		DWORD dwConnections  = mxl.cConnections;   
		//   准备获取麦克风设备的ID   
		DWORD   dwLineID   =   0;   
		for(DWORD i = 0; i < dwConnections; ++i)   
		{   
			//枚举每一个设备，当Source的ID等于当前的迭代记数   
			mxl.dwSource = i;   
			//根据SourceID获得连接的信息   
			MMRESULT mr = mixerGetLineInfo((HMIXEROBJ)hmixer, &mxl, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_SOURCE);   
			//判断函数执行错误   
			if(mr != 0)   
			{   
				break;   
			}   
			//如果当前设备类型是麦克风，则跳出循环。   
			if(mxl.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)   
			{   
				exist = true;
				break;   
			}   
		} 
	}
	else if(!mixerGetLineInfo((HMIXEROBJ)hmixer, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE))
	{
		exist = true;
	}

	if (exist)
	{
		mxlc.cbStruct = sizeof(mxlc);
		mxlc.dwLineID = mxl.dwLineID;
		mxlc.dwControlType = ctrlType;
		mxlc.cControls = 1;
		mxlc.cbmxctrl = sizeof(MIXERCONTROL);
		mxlc.pamxctrl = mxc;
		if(mixerGetLineControls((HMIXEROBJ)hmixer,&mxlc,MIXER_GETLINECONTROLSF_ONEBYTYPE))
			return 0;
		else
			return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
long CMixer::GetMuteValue(HMIXER hmixer ,MIXERCONTROL *mxc)
{
	MIXERCONTROLDETAILS mxcd;
	MIXERCONTROLDETAILS_BOOLEAN mxcdMute;
	mxcd.hwndOwner = 0;
	mxcd.cbStruct = sizeof(mxcd);
	mxcd.dwControlID = mxc->dwControlID;
	mxcd.cbDetails = sizeof(mxcdMute);
	mxcd.paDetails = &mxcdMute;
	mxcd.cChannels = 1;
	mxcd.cMultipleItems = 0;
	if (mixerGetControlDetails((HMIXEROBJ)hmixer, &mxcd,MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE))
	{
		return -1;
	}
	return mxcdMute.fValue;
}

//---------------------------------------------------------------------------
unsigned CMixer::GetVolumeValue(HMIXER hmixer ,MIXERCONTROL *mxc)
{
	MIXERCONTROLDETAILS mxcd;
	MIXERCONTROLDETAILS_UNSIGNED vol; 
	vol.dwValue=0;
	mxcd.hwndOwner = 0;
	mxcd.cbStruct = sizeof(mxcd);
	mxcd.dwControlID = mxc->dwControlID;
	mxcd.cbDetails = sizeof(vol);
	mxcd.paDetails = &vol;
	mxcd.cChannels = 1;
	if(mixerGetControlDetails((HMIXEROBJ)hmixer, &mxcd, MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE))
	{
		return -1;
	}
	return vol.dwValue;
}

//---------------------------------------------------------------------------
bool CMixer::SetMuteValue(HMIXER hmixer ,MIXERCONTROL *mxc, bool mute)
{
	MIXERCONTROLDETAILS mxcd;
	MIXERCONTROLDETAILS_BOOLEAN mxcdMute;
	mxcdMute.fValue=mute;
	mxcd.hwndOwner = 0;
	mxcd.dwControlID = mxc->dwControlID;
	mxcd.cbStruct = sizeof(mxcd);
	mxcd.cbDetails = sizeof(mxcdMute);
	mxcd.paDetails = &mxcdMute;
	mxcd.cChannels = 1;
	mxcd.cMultipleItems = 0;
	if (mixerSetControlDetails((HMIXEROBJ)hmixer, &mxcd, MIXER_OBJECTF_HMIXER|MIXER_SETCONTROLDETAILSF_VALUE))
	{
		return 0;
	}
	return 1;
}

//---------------------------------------------------------------------------

bool CMixer::SetVolumeValue(HMIXER hmixer ,MIXERCONTROL *mxc, long volume)
{
	MIXERCONTROLDETAILS mxcd;
	MIXERCONTROLDETAILS_UNSIGNED vol;
	vol.dwValue = volume;
	mxcd.hwndOwner = 0;
	mxcd.dwControlID = mxc->dwControlID;
	mxcd.cbStruct = sizeof(mxcd);
	mxcd.cbDetails = sizeof(vol);
	mxcd.paDetails = &vol;
	mxcd.cChannels = 1;
	if(mixerSetControlDetails((HMIXEROBJ)hmixer, &mxcd, MIXER_OBJECTF_HMIXER|MIXER_SETCONTROLDETAILSF_VALUE))
	{
		return 0;
	}
	return 1;
}


//---------------------------------------------------------------------------
unsigned /*WINAPI */CMixer::GetVolume(MixerDeice dev)//得到设备的音量dev=0主音量，1WAVE ,2MIDI ,3 LINE IN
{
	long device;
	unsigned rt=0;
	MIXERCONTROL volCtrl;
	HMIXER hmixer;
	switch (dev)
	{
	case WAVEOUT:
		device=MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT; break;
	case SYNTHESIZER:
		device=MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER; break;
	case MICROPHONE:
		// device=MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC; break;  // cd 音量
		device=MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE; break;      //麦克风音量
		// device=MIXERLINE_COMPONENTTYPE_SRC_LINE; break;         //PC 扬声器音量
		//device=MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC; break;
	default:
		device=MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	}

	if(mixerOpen(&hmixer, 0, 0, 0, 0)) return 0;
	if(!GetVolumeControl(hmixer,device,MIXERCONTROL_CONTROLTYPE_VOLUME,&volCtrl))
	{
		mixerClose(hmixer);
		return 0;
	}
	rt = GetVolumeValue(hmixer,&volCtrl)*m_VolRange /volCtrl.Bounds.lMaximum;
	mixerClose(hmixer);
	return rt;

}

//---------------------------------------------------------------------------
bool /*WINAPI*/ CMixer::SetVolume(MixerDeice dev,long vol)//设置设备的音量
{
	// dev =0,1,2   分别表示主音量,波形,MIDI ,LINE IN
	// vol=0-m_VolRange      表示音量的大小 , 设置与返回音量的值用的是百分比，即音量从0 - m_VolRange ，而不是设备的绝对值
	// retrun false 表示设置音量的大小的操作不成功
	// retrun true 表示设置音量的大小的操作成功

	long device;
	bool rc=false;
	MIXERCONTROL volCtrl;
	HMIXER hmixer;
	switch (dev)
	{
	case WAVEOUT:
		device=MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT; break;
	case SYNTHESIZER:
		device=MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER; break;
	case MICROPHONE:

		// device=MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC; break; // cd 音量
		device=MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE; break;   //麦克风音量
		// device=MIXERLINE_COMPONENTTYPE_SRC_LINE; break;         //PC 扬声器音量
		//device=MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC; break;
	default:
		device=MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	}

	if(mixerOpen(&hmixer, 0, 0, 0, 0)) return 0;

	if(GetVolumeControl(hmixer,device,MIXERCONTROL_CONTROLTYPE_VOLUME,&volCtrl))
	{
		vol=vol*volCtrl.Bounds.lMaximum/m_VolRange ;
		if(SetVolumeValue(hmixer,&volCtrl,vol))
		{
			rc=true;
		}
	}
	mixerClose(hmixer);
	return rc;
}

//---------------------------------------------------------------------------

bool /*WINAPI*/ CMixer::SetMute(MixerDeice dev,/*long*/bool vol)//设置设备静音
{
	// dev =0,1,2   分别表示主音量,波形,MIDI ,LINE IN
	// vol=0,1      分别表示取消静音,设置静音
	// retrun false 表示取消或设置静音操作不成功
	// retrun true 表示取消或设置静音操作成功

	long device;
	bool rc=false;
	MIXERCONTROL volCtrl;
	HMIXER hmixer;
	switch (dev)
	{
	case WAVEOUT:
		device=MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT; break;
	case SYNTHESIZER:
		device=MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER; break;
	case MICROPHONE:
		// device=MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC; break; // cd 音量
		device=MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE; break;   //麦克风音量
		// device=MIXERLINE_COMPONENTTYPE_SRC_LINE; break;         //PC 扬声器音量
		//device=MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC; break;

	default:
		device=MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	}

	if(mixerOpen(&hmixer, 0, 0, 0, 0)) return 0;
	if(GetVolumeControl(hmixer,device,MIXERCONTROL_CONTROLTYPE_MUTE,&volCtrl))
	{
		if(SetMuteValue(hmixer,&volCtrl,(bool)vol))
		{
			rc=true;
		}
	}
	mixerClose(hmixer);
	return rc;
}

//---------------------------------------------------------------------------

bool /*WINAPI */CMixer::GetMute(MixerDeice dev)//检查设备是否静音
{
	//dev =0,1,2 分别表示主音量，波形,MIDI ,LINE IN
	// retrun false 表示没有静音
	// retrun true 表示静音
	long device;
	bool rc=false;
	MIXERCONTROL volCtrl;
	HMIXER hmixer;
	switch (dev)
	{
	case WAVEOUT:
		device=MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT; break;
	case SYNTHESIZER:
		device=MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER; break;
	case MICROPHONE:  
		// device=MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC; break; // cd 音量
		//device=MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE; break;   //麦克风音量
		// device=MIXERLINE_COMPONENTTYPE_SRC_LINE; break;         //PC 扬声器音量
		//device=MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC; break;
		device=MIXERLINE_COMPONENTTYPE_DST_WAVEIN; break;
	default:
		device=MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	}

	if(mixerOpen(&hmixer, 0, 0, 0, 0)) return 0;

	if(GetVolumeControl(hmixer,device,MIXERCONTROL_CONTROLTYPE_MUTE,&volCtrl))
	{
		rc=GetMuteValue(hmixer,&volCtrl);
	}
	mixerClose(hmixer);
	return rc;

}

//如果第一个参数是true,那么就是设置，否则就是获得  
bool CMixer::SetGetDevVolume(bool bIsSet,DWORD & dwVolume)  
{  
	MMRESULT          rc;     // 多媒体函数返回结果变量  
	HMIXER            hMixer; // 混合器设备句柄  
	MIXERLINE         mxl;    // 音频线路标准状态信息结构体  
	MIXERLINECONTROLS mxlc;   // 音频线路控制器集合信息结构体  
	MIXERCONTROL      mxc;    // 单个音频线路控制器信息结构体  

	// 打开混合器设备  
	rc = mixerOpen(&hMixer, // 返回的设备句柄  
		0,       // 单声卡的设备ID为零  
		0,       // 不使用回调机制  
		0,       // 回调函数参数  
		0);      // MIXER_OBJECTF_MIXER宏的值，表示任选有效设备ID  

	// 打开混合器设备无错的话，则  
	if (MMSYSERR_NOERROR == rc)  
	{  
		// MIXERLINE 结构体变量清零  
		ZeroMemory(&mxl, sizeof(MIXERLINE));  

		mxl.cbStruct = sizeof(MIXERLINE); // 微软用此办法判断版本  
		// 指出需要获取的通道，声卡的音频输出用MIXERLINE_COMPONENTTYPE_DST_SPEAKERS  
		mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;  

		// 取得混合器设备的指定线路信息  
		rc = mixerGetLineInfo((HMIXEROBJ)hMixer,  
			&mxl,  
			// 取得MIXERLINE::dwComponentType指定类型的第一个音频线路信息  
			MIXER_GETLINEINFOF_COMPONENTTYPE);  

		// 取得混合器设备的指定线路信息成功的话，则  
		if (MMSYSERR_NOERROR == rc)  
		{  
			// MIXERCONTROL 结构体变量清零  
			ZeroMemory(&mxc, sizeof(MIXERCONTROL));  

			mxc.cbStruct = sizeof(mxc); // 微软用此办法判断版本  

			// MIXERLINECONTROLS 结构体变量清零  
			ZeroMemory(&mxlc, sizeof(MIXERLINECONTROLS));  

			mxlc.cbStruct = sizeof(mxlc); // 微软用此办法判断版本  

			mxlc.dwLineID = mxl.dwLineID; // 上面取得的声卡音频输出线路标识  
			// 控制类型为控制音量  
			mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;  
			mxlc.cControls = 1;          // 使用 MIXERCONTROL 结构体变量个数  
			mxlc.pamxctrl = &mxc;        // MIXERCONTROL 结构体变量指针  
			mxlc.cbmxctrl = sizeof(mxc); // MIXERCONTROL 结构体变量字节大小  

			// 取得控制器信息  
			rc = mixerGetLineControls((HMIXEROBJ)hMixer,  
				&mxlc,  
				MIXER_GETLINECONTROLSF_ONEBYTYPE);  

			// 取得控制器信息成功的话，则  
			if (MMSYSERR_NOERROR == rc)  
			{  
				// 获取控制器中的值的音量范围：mxc.Bounds.lMinimum到mxc.Bounds.lMaximum.  
				MIXERCONTROLDETAILS        mxcd;      // 控制器的状态信息  
				MIXERCONTROLDETAILS_SIGNED volStruct; // 音量结构体变量(就一个成员量)  

				// MIXERCONTROLDETAILS 结构体变量清零  
				ZeroMemory(&mxcd, sizeof(mxcd));  

				mxcd.cbStruct = sizeof(mxcd);       // 微软用此办法判断版本  
				mxcd.dwControlID = mxc.dwControlID; // 上面取得的控制器标识  
				mxcd.paDetails = &volStruct;        // 音量结构体变量指针  
				mxcd.cbDetails = sizeof(volStruct); // 音量结构体变量字节大小  
				mxcd.cChannels = 1;                 // 取得或设置全部通道  


				// 设置音量  
				if (bIsSet)  
				{  
					volStruct.lValue = dwVolume; // 想要设置的值  
					rc = mixerSetControlDetails((HMIXEROBJ)hMixer,  
						&mxcd,  
						MIXER_SETCONTROLDETAILSF_VALUE);  

					if (MMSYSERR_NOERROR == rc)  
					{  
						return  true;  
					}  
					else  
					{  
						return  false;  
					}  
				}   
				// 获得音量值  
				else  
				{  
					rc = mixerGetControlDetails((HMIXEROBJ)hMixer,  
						&mxcd,  
						MIXER_GETCONTROLDETAILSF_VALUE);  

					if (MMSYSERR_NOERROR == rc)  
					{  
						dwVolume=volStruct.lValue;  
					}  
					else  
					{  
						return  false;  
					}  
				}  
			}  
			else  
			{  
				return  false;  
			}  
		}  
		else  
		{  
			return  false;  
		}  
	}  
	else  
	{  
		return  false;  
	}  

	return  true;  
}  