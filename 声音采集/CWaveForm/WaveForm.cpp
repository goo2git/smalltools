/*
+===========================================================================+
|				Copyright (C) Direct Line Corp. 1999-2000.					|
+---------------------------------------------------------------------------+
| File Name:																|
|																			|
|	WaveForm.cpp															|
|																			|
+---------------------------------------------------------------------------+
| Descriptions:																|
|																			|
|	Win32 wave form API object oriented implementation.						|
|																			|
+---------------------------------------------------------------------------+
| Developer(s):																|
|																			|
|	Xu Wen Bin.																|
|																			|
+===========================================================================+
|						C H A N GE		L O G								|
+---------------------------------------------------------------------------+
|																			|
|	07-20-01	1.00	Created.											|
|	07-27-01	1.01	Modified.											|
|	09-01-01	1.02	Modified.											|
|	02-22-06	1.03	Modified.											|
|																			|
+---------------------------------------------------------------------------+
| Notes:																	|
|																			|
|	1. Wave device open sequence : open()->start()->stop()->close().		|
|	2. Support seperate operation on wave in and out object.				|
|	3. Use standard stream device interface.								|
|																			|
+===========================================================================+
*/

#include "StdAfx.h"
#include "WaveForm.h"

/****************************************************************************
 * CWaveForm::CWaveForm()
 ****************************************************************************
 * CWaveForm constructor.
 */
CWaveForm::CWaveForm
(
	UINT           uDirection,
	UINT           uWaveInDeviceID,
	UINT           uWaveOutDeviceID,
	WAVEFORMATEX * lpWaveInFormatEx,
	WAVEFORMATEX * lpWaveOutFormatEx,
	DWORD          dwWaveInVolume,
	DWORD          dwWaveOutVolume
)
{
	/////////////////////////////////////////////////////////////////////////
	// Init wave format related variables.
	/////////////////////////////////////////////////////////////////////////
	m_RefCount = 0;

	/////////////////////////////////////////////////////////////////////////
	// Preset wave in/out device info.
	/////////////////////////////////////////////////////////////////////////
	m_uDirection      = uDirection;
	m_waveInDeviceID  = uWaveInDeviceID;
	m_waveOutDeviceID = uWaveOutDeviceID;

	if ((m_uDirection & WAVE_FORM_FLOW_IN) && (lpWaveInFormatEx != NULL))
	{
		memcpy(&m_waveInFormatEx, lpWaveInFormatEx, sizeof(WAVEFORMATEX));
	}

	if ((m_uDirection & WAVE_FORM_FLOW_OUT) && (lpWaveOutFormatEx != NULL))
	{
		memcpy(&m_waveOutFormatEx, lpWaveOutFormatEx, sizeof(WAVEFORMATEX));
	}

	m_hWaveIn  = NULL;
	m_hWaveOut = NULL;

	m_waveInState  = WAVE_STATE_CLOSE;
	m_waveOutState = WAVE_STATE_CLOSE;

	/////////////////////////////////////////////////////////////////////////
	// Initialize wave in critical section locks & free list.
	/////////////////////////////////////////////////////////////////////////
	if (m_uDirection & WAVE_FORM_FLOW_IN)
	{
        InitializeCriticalSection(&m_waveInFrameFreeListLock);
		InitializeCriticalSection(&m_waveInFrameWaitingListLock);
        InitializeCriticalSection(&m_waveInFrameProcessListLock);

        m_bWaveInProcessFlag = FALSE;
        m_hWaveInProcessEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hWaveInProcessThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProcIn, (LPVOID)this, 0, 0);
	}

	/////////////////////////////////////////////////////////////////////////
	// Initialize wave out critical section locks & free list.
	/////////////////////////////////////////////////////////////////////////
	if (m_uDirection & WAVE_FORM_FLOW_OUT)
	{
		InitializeCriticalSection(&m_waveOutFrameFreeListLock);
		InitializeCriticalSection(&m_waveOutFrameWaitingListLock);
        InitializeCriticalSection(&m_waveOutFrameProcessListLock);

        m_bWaveOutProcessFlag = FALSE;
        m_hWaveOutProcessEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hWaveOutProcessThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProcOut, (LPVOID)this, 0, 0);
	}

	/////////////////////////////////////////////////////////////////////////
	// Save wave in/out volumes.
	/////////////////////////////////////////////////////////////////////////
	m_waveInVolume = dwWaveInVolume;
	m_waveOutVolume = dwWaveOutVolume;

	/////////////////////////////////////////////////////////////////////////
	// Setup wave in/out outstanding monitor.
	/////////////////////////////////////////////////////////////////////////
	m_dwWaveInByteInDevice = 0;
	m_dwWaveOutByteInDevice = 0;
}

/****************************************************************************
 * CWaveForm::~CWaveForm()
 ****************************************************************************
 * CWaveForm destructor.
 */
CWaveForm::~CWaveForm()
{
	/////////////////////////////////////////////////////////////////////////
	// Delete wave in list lock & free free/waiting/process list.
	/////////////////////////////////////////////////////////////////////////
	if (m_uDirection & WAVE_FORM_FLOW_IN)
	{
        m_bWaveInProcessFlag = TRUE;
        SetEvent(m_hWaveInProcessEvent);
        WaitForSingleObject(m_hWaveInProcessThread, INFINITE);

		CloseHandle(m_hWaveInProcessEvent);
		CloseHandle(m_hWaveInProcessThread);

		DeleteCriticalSection(&m_waveInFrameFreeListLock);
		DeleteCriticalSection(&m_waveInFrameWaitingListLock);
        DeleteCriticalSection(&m_waveInFrameProcessListLock);
	}

	/////////////////////////////////////////////////////////////////////////
	// Delete wave out list lock & free free/waiting/process list.
	/////////////////////////////////////////////////////////////////////////
	if (m_uDirection & WAVE_FORM_FLOW_OUT)
	{
        m_bWaveOutProcessFlag = TRUE;
        SetEvent(m_hWaveOutProcessEvent);
        WaitForSingleObject(m_hWaveOutProcessThread, INFINITE);

        CloseHandle(m_hWaveOutProcessEvent);
		CloseHandle(m_hWaveOutProcessThread);

		DeleteCriticalSection(&m_waveOutFrameFreeListLock);
		DeleteCriticalSection(&m_waveOutFrameWaitingListLock);
        DeleteCriticalSection(&m_waveOutFrameProcessListLock);
	}
}

/****************************************************************************
 * CWaveForm::AddRef()
 ****************************************************************************
 * Increase reference count.
 */
long
CWaveForm::AddRef()
{
	InterlockedIncrement(&m_RefCount);
	return m_RefCount;
}

/****************************************************************************
 * CWaveForm::Release()
 ****************************************************************************
 * Decrease reference count, if 0, delete object.
 */
long
CWaveForm::Release()
{
	if (InterlockedDecrement(&m_RefCount) == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

/****************************************************************************
 * CWaveForm::Open()
 ****************************************************************************
 * Open wave in/out device.
 */
BOOL
CWaveForm::Open
(
	UINT	uDirection
)
{
	MMRESULT ret = MMSYSERR_INVALPARAM;

	/////////////////////////////////////////////////////////////////////////
	// Try to open wave in device, which support requested wave format.
	/////////////////////////////////////////////////////////////////////////
	if ((uDirection & WAVE_FORM_FLOW_IN) && (m_uDirection & WAVE_FORM_FLOW_IN))
	{
		if (m_waveInState == WAVE_STATE_CLOSE)
		{
			ret = ::waveInOpen
					(
						&m_hWaveIn,
						m_waveInDeviceID,
						&m_waveInFormatEx,
						DWORD(WaveInCallBackRoutine),
						DWORD(this),
						CALLBACK_FUNCTION
					);

			if (ret == MMSYSERR_NOERROR)
			{
				m_waveInState = WAVE_STATE_OPEN;
				SetVolume(WAVE_FORM_FLOW_IN, m_waveInVolume);
			}
		}
		else
		{
			::MessageBox
			(
				NULL,
				_T("Fail To Open Wave In Device!!!"),
				_T("Direct Line Service"),
				MB_OK | MB_ICONWARNING
			);

			return FALSE;
		}
	}
    else
    {
		::MessageBox
		(
			NULL,
			_T("Wave In Object Is Not Initialized!!!"),
			_T("Direct Line Service"),
			MB_OK | MB_ICONWARNING
		);

        return FALSE;
    }

	/////////////////////////////////////////////////////////////////////////
	// Try to open wave out device, which support requested wave format.
	/////////////////////////////////////////////////////////////////////////
	if ((uDirection & WAVE_FORM_FLOW_OUT) && (m_uDirection & WAVE_FORM_FLOW_OUT))
	{
		if (m_waveOutState == WAVE_STATE_CLOSE)
		{
			ret = ::waveOutOpen
					(
						&m_hWaveOut,
						m_waveOutDeviceID,
						&m_waveOutFormatEx,
						DWORD(WaveOutCallBackRoutine),
						DWORD(this),
						CALLBACK_FUNCTION
					);

			if (ret == MMSYSERR_NOERROR)
			{
				m_waveOutState = WAVE_STATE_OPEN;
				SetVolume(WAVE_FORM_FLOW_OUT, m_waveOutVolume);
			}
		}
		else
		{
			::MessageBox
			(
				NULL,
				_T("Fail To Open Wave Out Device!!!"),
				_T("Direct Line Service"),
				MB_OK | MB_ICONWARNING
			);

			return FALSE;
		}
	}
    else
    {
		::MessageBox
		(
			NULL,
			_T("Wave Out Object Is Not Initialized!!!"),
			_T("Direct Line Service"),
			MB_OK | MB_ICONWARNING
		);

        return FALSE;
    }

	return TRUE;
}

/****************************************************************************
 * CWaveForm::Close()
 ****************************************************************************
 * Close wave in/out device.
 */
BOOL
CWaveForm::Close
(
	UINT	uDirection
)
{
	MMRESULT ret = MMSYSERR_INVALPARAM;

	/////////////////////////////////////////////////////////////////////////
	// Try to close wave in device.
	/////////////////////////////////////////////////////////////////////////
	if ((uDirection & WAVE_FORM_FLOW_IN) && (m_uDirection & WAVE_FORM_FLOW_IN))
	{
		if (m_waveInState == WAVE_STATE_STOP)
		{
			if (m_hWaveIn)
			{
		        while (!m_waveInFrameFreeList.IsEmpty())
        		{
		        	PWAVEFRAME waveFrame = m_waveInFrameFreeList.RemoveHead();
			        ::waveInUnprepareHeader
        			(
		        		m_hWaveIn,
				        (LPWAVEHDR)waveFrame,
        				sizeof(WAVEHDR)
		        	);
			        delete waveFrame;
		        }
		        while (!m_waveInFrameWaitingList.IsEmpty())
		        {
			        PWAVEFRAME waveFrame = m_waveInFrameWaitingList.RemoveHead();
			        ::waveInUnprepareHeader
			        (
				        m_hWaveIn,
				        (LPWAVEHDR)waveFrame,
				        sizeof(WAVEHDR)
			        );
			        delete waveFrame;
		        }
		        while (!m_waveInFrameProcessList.IsEmpty())
                {
			        PWAVEFRAME waveFrame = m_waveInFrameProcessList.RemoveHead();
			        ::waveInUnprepareHeader
			        (
				        m_hWaveIn,
				        (LPWAVEHDR)waveFrame,
				        sizeof(WAVEHDR)
			        );
			        delete waveFrame;
		        }

                ::waveInClose(m_hWaveIn);
				m_hWaveIn = NULL;
				m_waveInState = WAVE_STATE_CLOSE;
			}
		}
		else
		{
			::MessageBox
			(
				NULL,
				_T("Wave In Device Must Be Stopped First!!!"),
				_T("Direct Line Service"),
				MB_OK | MB_ICONWARNING
			);

			return FALSE;
		}
	}
    else
    {
		::MessageBox
		(
			NULL,
			_T("No Matching Wave In Stream!!!"),
			_T("Direct Line Service"),
			MB_OK | MB_ICONWARNING
		);

        return FALSE;
    }

	/////////////////////////////////////////////////////////////////////////
	// Try to close wave out device.
	/////////////////////////////////////////////////////////////////////////
	if ((uDirection & WAVE_FORM_FLOW_OUT) && (m_uDirection & WAVE_FORM_FLOW_OUT))
	{
		if (m_waveOutState == WAVE_STATE_STOP)
		{
			if (m_hWaveOut)
			{
        		while (!m_waveOutFrameFreeList.IsEmpty())
        		{
        			PWAVEFRAME waveFrame = m_waveOutFrameFreeList.RemoveHead();
        			::waveOutUnprepareHeader
        			(
        				m_hWaveOut,
        				(LPWAVEHDR)waveFrame,
        				sizeof(WAVEHDR)
        			);
        			delete waveFrame;
        		}
        		while (!m_waveOutFrameWaitingList.IsEmpty())
        		{
        			PWAVEFRAME waveFrame = m_waveOutFrameWaitingList.RemoveHead();
        			::waveOutUnprepareHeader
        			(
        				m_hWaveOut,
        				(LPWAVEHDR)waveFrame,
        				sizeof(WAVEHDR)
        			);
        			delete waveFrame;
        		}
        		while (!m_waveOutFrameProcessList.IsEmpty())
        		{
        			PWAVEFRAME waveFrame = m_waveOutFrameProcessList.RemoveHead();
        			::waveOutUnprepareHeader
        			(
        				m_hWaveOut,
        				(LPWAVEHDR)waveFrame,
        				sizeof(WAVEHDR)
        			);
        			delete waveFrame;
                }

				::waveOutClose(m_hWaveOut);
				m_hWaveOut = NULL;
				m_waveOutState = WAVE_STATE_CLOSE;
			}
		}
		else
		{
			::MessageBox
			(
				NULL,
				_T("Wave Out Device Must Be Stopped First!!!"),
				_T("Direct Line Service"),
				MB_OK | MB_ICONWARNING
			);

			return FALSE;
		}
	}
    else
    {
		::MessageBox
		(
			NULL,
			_T("No Matching Wave Out Stream!!!"),
			_T("Direct Line Service"),
			MB_OK | MB_ICONWARNING
		);

        return FALSE;
    }

	return TRUE;
}

/****************************************************************************
 * CWaveForm::Start()
 ****************************************************************************
 * Start wave device.
 */
BOOL
CWaveForm::Start
(
	UINT	uDirection
)
{
	MMRESULT ret = MMSYSERR_INVALPARAM;

	/////////////////////////////////////////////////////////////////////////
	// Start wave in stream.
	/////////////////////////////////////////////////////////////////////////
	if ((uDirection & WAVE_FORM_FLOW_IN) && (m_uDirection & WAVE_FORM_FLOW_IN))
	{
		if (m_waveInState == WAVE_STATE_OPEN)
		{
			if (m_hWaveIn)
			{
                for (int i = 0; i < MIN_WAVE_NUM; i++)
                {
					PWAVEFRAME waveFrame = new WAVEFRAME;

					if (waveFrame)
					{
						waveFrame->waveHdr.lpData          = waveFrame->Data;
						waveFrame->waveHdr.dwBufferLength  = MAX_WAVE_BUFFER_LEN;
						waveFrame->waveHdr.dwBytesRecorded = 0;
						waveFrame->waveHdr.dwFlags         = 0;
						waveFrame->waveHdr.dwLoops         = 0;
						waveFrame->waveHdr.dwUser          = 0;
						waveFrame->waveHdr.lpNext          = 0;

						ret = ::waveInPrepareHeader
								(
									m_hWaveIn,
									(LPWAVEHDR)waveFrame,
									sizeof(WAVEHDR)
								);

						ret = ::waveInAddBuffer
								(
									m_hWaveIn,
									(LPWAVEHDR)waveFrame,
									sizeof(WAVEHDR)
								);

						m_dwWaveInByteInDevice += waveFrame->waveHdr.dwBufferLength;
					}
				}

				ret = ::waveInStart(m_hWaveIn);
				m_waveInState = WAVE_STATE_START;
			}
		}
		else
		{
			::MessageBox
			(
				NULL,
				_T("Fail To Start Wave In Device!!!"),
				_T("Direct Line Service"),
				MB_OK | MB_ICONWARNING
			);

			return FALSE;
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// Start wave out stream.
	/////////////////////////////////////////////////////////////////////////
	if ((uDirection & WAVE_FORM_FLOW_OUT) && (m_uDirection & WAVE_FORM_FLOW_OUT))
	{
		if (m_waveOutState == WAVE_STATE_OPEN)
		{
			if (m_hWaveOut)
			{
				while (!m_waveOutFrameWaitingList.IsEmpty())
				{
					PWAVEFRAME waveFrame = m_waveOutFrameWaitingList.RemoveHead();

					if (waveFrame)
					{
						ret = ::waveOutWrite
								(
									m_hWaveOut,
									(LPWAVEHDR)waveFrame,
									sizeof(WAVEHDR)
								);

						m_dwWaveOutByteInDevice += waveFrame->waveHdr.dwBufferLength;
					}
				}

				m_waveOutState = WAVE_STATE_START;
			}
		}
		else
		{
			::MessageBox
			(
				NULL,
				_T("Fail To Start Wave Out Device!!!"),
				_T("Direct Line Service"),
				MB_OK | MB_ICONWARNING
			);

			return FALSE;
		}
	}

	return TRUE;
}

/****************************************************************************
 * CWaveForm::Stop()
 ****************************************************************************
 * Stop wave device.
 */
BOOL
CWaveForm::Stop
(
	UINT	uDirection
)
{
	MMRESULT ret = MMSYSERR_INVALPARAM;

	/////////////////////////////////////////////////////////////////////////
	// Stop wave in stream.
	/////////////////////////////////////////////////////////////////////////
	if ((uDirection & WAVE_FORM_FLOW_IN) && (m_uDirection & WAVE_FORM_FLOW_IN))
	{
		if (m_waveInState == WAVE_STATE_START)
		{
			if (m_hWaveIn)
			{
				m_waveInState = WAVE_STATE_RESET;
				ret = ::waveInReset(m_hWaveIn);
                while(m_dwWaveInByteInDevice > 0)
                {
                    Sleep(500);
                }
                SetEvent(m_hWaveInProcessEvent);
				m_waveInState = WAVE_STATE_STOP;
				ret = ::waveInStop(m_hWaveIn);
			}
		}
		else
		{
			::MessageBox
			(
				NULL,
				_T("Fail To Stop Wave In Device!!!"),
				_T("Direct Line Service"),
				MB_OK | MB_ICONWARNING
			);

			return FALSE;
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// Stop wave out stream.
	/////////////////////////////////////////////////////////////////////////
	if ((uDirection & WAVE_FORM_FLOW_OUT) && (m_uDirection & WAVE_FORM_FLOW_OUT))
	{
		if (m_waveOutState == WAVE_STATE_START)
		{
			if (m_hWaveOut)
			{
				m_waveOutState = WAVE_STATE_RESET;
				ret = ::waveOutReset(m_hWaveOut);
                while(m_dwWaveOutByteInDevice > 0)
                {
                    Sleep(500);
                }
                SetEvent(m_hWaveOutProcessEvent);
				m_waveOutState = WAVE_STATE_STOP;
			}
		}
		else
		{
			::MessageBox
			(
				NULL,
				_T("Fail To Stop Wave Out Device!!!"),
				_T("Direct Line Service"),
				MB_OK | MB_ICONWARNING
			);

			return FALSE;
		}
	}

	return TRUE;
}

/****************************************************************************
 * CWaveForm::Pause()
 ****************************************************************************
 * Pause wave device.
 */
BOOL
CWaveForm::Pause
(
	UINT uDirection
)
{
	if ((uDirection & WAVE_FORM_FLOW_OUT) && (m_uDirection & WAVE_FORM_FLOW_OUT))
	{
		if (m_hWaveOut)
		{
			m_waveOutState = WAVE_STATE_PAUSE;
			::waveOutPause(m_hWaveOut);
			return TRUE;
		}
	}
	return FALSE;
}

/****************************************************************************
 * CWaveForm::Restart()
 ****************************************************************************
 * Restart wave device.
 */
BOOL
CWaveForm::Restart
(
	UINT uDirection
)
{
	if ((uDirection & WAVE_FORM_FLOW_OUT) && (m_uDirection & WAVE_FORM_FLOW_OUT))
	{
		if (m_hWaveOut)
		{
			m_waveOutState = WAVE_STATE_START;
			::waveOutRestart(m_hWaveOut);
			return TRUE;
		}
	}
	return FALSE;
}

/****************************************************************************
 * CWaveForm::Reset()
 ****************************************************************************
 * Reset wave device.
 */
BOOL
CWaveForm::Reset
(
	UINT uDirection
)
{
	if ((uDirection & WAVE_FORM_FLOW_IN) && (m_uDirection & WAVE_FORM_FLOW_IN))
	{
		if (m_hWaveIn)
		{
			m_waveInState = WAVE_STATE_RESET;
			::waveInReset(m_hWaveIn);
			m_waveInState = WAVE_STATE_STOP;
			return TRUE;
		}
	}

	if ((uDirection & WAVE_FORM_FLOW_OUT) && (m_uDirection & WAVE_FORM_FLOW_OUT))
	{
		if (m_hWaveOut)
		{
			m_waveOutState = WAVE_STATE_RESET;
			::waveOutReset(m_hWaveOut);
			m_waveOutState = WAVE_STATE_STOP;
			return TRUE;
		}
	}

	return FALSE;
}

/****************************************************************************
 * CWaveForm::GetDirection()
 ****************************************************************************
 * Get wave form flow direction.
 */
UINT
CWaveForm::GetDirection()
{
	return m_uDirection;
}

/****************************************************************************
 * CWaveForm::GetDirection()
 ****************************************************************************
 * Set wave form flow direction.
 * Please note: this function is not fully implemented.
 */
void
CWaveForm::SetDirection(UINT uDirection)
{
    ASSERT(0);
}

/****************************************************************************
 * CWaveForm::GetFormat()
 ****************************************************************************
 * Get wave in/out format.
 */
BOOL
CWaveForm::GetFormat
(
	UINT			uDirection,
	LPWAVEFORMATEX	lpWaveFormatEx
)
{
	if ((uDirection & WAVE_FORM_FLOW_IN) && (lpWaveFormatEx != NULL))
	{
		memcpy(lpWaveFormatEx, &m_waveInFormatEx, sizeof(WAVEFORMATEX));
		return TRUE;
	}

	if ((uDirection & WAVE_FORM_FLOW_OUT) && (lpWaveFormatEx != NULL))
	{
		memcpy(lpWaveFormatEx, &m_waveOutFormatEx, sizeof(WAVEFORMATEX));
		return TRUE;
	}

	return FALSE;
}

/****************************************************************************
 * CWaveForm::SetFormat()
 ****************************************************************************
 * Set wave in/out format.
 * Please note: this function is not fully implemented.
 */
BOOL
CWaveForm::SetFormat
(
	UINT			uDirection,
	LPWAVEFORMATEX	lpWaveFormatEx
)
{
	if ((uDirection & WAVE_FORM_FLOW_IN) && (lpWaveFormatEx != NULL))
	{
		memcpy(&m_waveInFormatEx, lpWaveFormatEx, sizeof(WAVEFORMATEX));
		return TRUE;
	}

	if ((uDirection & WAVE_FORM_FLOW_OUT) && (lpWaveFormatEx != NULL))
	{
		memcpy(&m_waveOutFormatEx, lpWaveFormatEx, sizeof(WAVEFORMATEX));
		return TRUE;
	}

	return FALSE;
}

/****************************************************************************
 * CWaveForm::UpdateFormat()
 ****************************************************************************
 * Update wave in/out format.
 * Please note: this function is not fully implemented.
 */
BOOL
CWaveForm::UpdateFormat
(
	UINT			uDirection,
	LPWAVEFORMATEX	lpWaveFormatEx
)
{
	if ((uDirection & WAVE_FORM_FLOW_IN) && (lpWaveFormatEx != NULL))
	{
		VALIDATE(lpWaveFormatEx->nChannels, m_waveInFormatEx.nChannels);
		VALIDATE(lpWaveFormatEx->nSamplesPerSec, m_waveInFormatEx.nSamplesPerSec);
		VALIDATE(lpWaveFormatEx->wBitsPerSample, m_waveInFormatEx.wBitsPerSample);

		lpWaveFormatEx->nAvgBytesPerSec = lpWaveFormatEx->nSamplesPerSec
											* lpWaveFormatEx->nChannels
											* (lpWaveFormatEx->wBitsPerSample / 8);
		lpWaveFormatEx->nBlockAlign = lpWaveFormatEx->nChannels
											* (lpWaveFormatEx->wBitsPerSample / 8);

		m_waveInFormatEx.nBlockAlign = lpWaveFormatEx->nBlockAlign;
		m_waveInFormatEx.nAvgBytesPerSec = lpWaveFormatEx->nAvgBytesPerSec;

		return TRUE;
	}

	if ((uDirection & WAVE_FORM_FLOW_OUT) && (lpWaveFormatEx != NULL))
	{
		VALIDATE(lpWaveFormatEx->nChannels, m_waveOutFormatEx.nChannels);
		VALIDATE(lpWaveFormatEx->nSamplesPerSec, m_waveOutFormatEx.nSamplesPerSec);
		VALIDATE(lpWaveFormatEx->wBitsPerSample, m_waveOutFormatEx.wBitsPerSample);

		lpWaveFormatEx->nAvgBytesPerSec = lpWaveFormatEx->nSamplesPerSec
											* lpWaveFormatEx->nChannels
											* (lpWaveFormatEx->wBitsPerSample / 8);
		lpWaveFormatEx->nBlockAlign = lpWaveFormatEx->nChannels
											* (lpWaveFormatEx->wBitsPerSample / 8);

		m_waveOutFormatEx.nBlockAlign = lpWaveFormatEx->nBlockAlign;
		m_waveOutFormatEx.nAvgBytesPerSec = lpWaveFormatEx->nAvgBytesPerSec;

		return TRUE;
	}

	return FALSE;
}

/****************************************************************************
 * CWaveForm::GetVolume()
 ****************************************************************************
 * Get wave form volume.
 */
BOOL
CWaveForm::GetVolume
(
	UINT	uDirection,
	PDWORD	pVolume
)
{
	MMRESULT ret = MMSYSERR_INVALPARAM;

	if (uDirection & WAVE_FORM_FLOW_IN)
	{
		if (m_hWaveIn)
		{
			HMIXER hMixer = NULL;
            ret = ::mixerOpen(&hMixer, (UINT)m_hWaveIn, NULL, NULL, MIXER_OBJECTF_HWAVEIN);

			if (ret == MMSYSERR_NOERROR)
			{
				// Get mixer recording dest line.
				MIXERLINE mixerDestLine;
				memset(&mixerDestLine, 0, sizeof(MIXERLINE));
				mixerDestLine.cbStruct = sizeof(MIXERLINE);
				mixerDestLine.dwDestination = 1; // 0 - Playback , 1 - Recording.

				ret = ::mixerGetLineInfo
				(
					(HMIXEROBJ)hMixer,
					&mixerDestLine,
					MIXER_GETLINEINFOF_DESTINATION
				);

				if (ret == MMSYSERR_NOERROR)
				{
					// Enumerate mixer recording source lines.
					UINT srcConnection = 0;
					MIXERLINE mixerSrcLine;

					for (; srcConnection < mixerDestLine.cConnections; srcConnection++)
					{
						memset(&mixerSrcLine, 0, sizeof(MIXERLINE));
						mixerSrcLine.cbStruct = sizeof(MIXERLINE);
						mixerSrcLine.dwDestination = 1; // 0 - Playback , 1 - Recording.
						mixerSrcLine.dwSource = srcConnection;

						ret = ::mixerGetLineInfo
						(
							(HMIXEROBJ)hMixer,
							&mixerSrcLine,
							MIXER_GETLINEINFOF_SOURCE
						);

						if (mixerSrcLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
						{
							MIXERCONTROL mixerMicVolumeControl;
							memset(&mixerMicVolumeControl, 0, sizeof(MIXERCONTROL));
							mixerMicVolumeControl.cbStruct = sizeof(MIXERCONTROL);

							MIXERLINECONTROLS mixerMicLineControls;
							memset(&mixerMicLineControls, 0, sizeof(MIXERLINECONTROLS));
							mixerMicLineControls.cbStruct  = sizeof(MIXERLINECONTROLS);
							mixerMicLineControls.dwLineID  = mixerSrcLine.dwLineID;
							mixerMicLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
							mixerMicLineControls.cControls = 1;
							mixerMicLineControls.pamxctrl  = &mixerMicVolumeControl;
							mixerMicLineControls.cbmxctrl  = sizeof(MIXERCONTROL);

							ret = ::mixerGetLineControls
							(
								(HMIXEROBJ)hMixer,
								&mixerMicLineControls,
								MIXER_GETLINECONTROLSF_ONEBYTYPE
							);

							if (ret == MMSYSERR_NOERROR)
							{
								MIXERCONTROLDETAILS_UNSIGNED volumeLevel[2];
								volumeLevel[0].dwValue = 0;
								volumeLevel[1].dwValue = 0;

								MIXERCONTROLDETAILS mixerControlDetails;
								mixerControlDetails.cbStruct    = sizeof(MIXERCONTROLDETAILS);
								mixerControlDetails.dwControlID = mixerMicLineControls.pamxctrl->dwControlID;
								mixerControlDetails.cMultipleItems = 0;
								mixerControlDetails.cChannels = mixerSrcLine.cChannels;
								mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED) * mixerSrcLine.cChannels;
								mixerControlDetails.paDetails = volumeLevel;

								ret = ::mixerGetControlDetails
								(
									(HMIXEROBJ)hMixer,
									&mixerControlDetails,
									MIXER_GETCONTROLDETAILSF_VALUE
								);

								*pVolume = volumeLevel[0].dwValue;
								*pVolume |= volumeLevel[1].dwValue << 16;
							}

							break;
						}
					}
				}

				ret = ::mixerClose(hMixer);
			}
		}
	}
	
	if (uDirection & WAVE_FORM_FLOW_OUT)
	{
		if (m_hWaveOut)
		{
			ret = ::waveOutGetVolume(m_hWaveOut, pVolume);
		}
	}

	if (ret == MMSYSERR_NOERROR)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/****************************************************************************
 * CWaveForm::SetVolume()
 ****************************************************************************
 * Set wave form volume.
 */
BOOL
CWaveForm::SetVolume
(
	UINT	uDirection,
	DWORD	dwVolume
)
{
	MMRESULT ret = MMSYSERR_INVALPARAM;

	if (uDirection & WAVE_FORM_FLOW_IN)
	{
		if (m_hWaveIn)
		{
			HMIXER hMixer = NULL;
			ret = ::mixerOpen(&hMixer, (UINT)m_hWaveIn, NULL, NULL, MIXER_OBJECTF_HWAVEIN);

			if (ret == MMSYSERR_NOERROR)
			{
				// Get mixer recording dest line.
				MIXERLINE mixerDestLine;
				memset(&mixerDestLine, 0, sizeof(MIXERLINE));
				mixerDestLine.cbStruct = sizeof(MIXERLINE);
				mixerDestLine.dwDestination = 1; // 0 - Playback , 1 - Recording.

				ret = ::mixerGetLineInfo
				(
					(HMIXEROBJ)hMixer,
					&mixerDestLine,
					MIXER_GETLINEINFOF_DESTINATION
				);

				if (ret == MMSYSERR_NOERROR)
				{
					// Enumerate mixer recording source lines.
					UINT srcConnection = 0;
					MIXERLINE mixerSrcLine;

					for (; srcConnection < mixerDestLine.cConnections; srcConnection++)
					{
						memset(&mixerSrcLine, 0, sizeof(MIXERLINE));
						mixerSrcLine.cbStruct = sizeof(MIXERLINE);
						mixerSrcLine.dwDestination = 1; // 0 - Playback , 1 - Recording.
						mixerSrcLine.dwSource = srcConnection;

						ret = ::mixerGetLineInfo
						(
							(HMIXEROBJ)hMixer,
							&mixerSrcLine,
							MIXER_GETLINEINFOF_SOURCE
						);

						if (mixerSrcLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
						{
							MIXERCONTROL mixerMicVolumeControl;
							memset(&mixerMicVolumeControl, 0, sizeof(MIXERCONTROL));
							mixerMicVolumeControl.cbStruct = sizeof(MIXERCONTROL);

							MIXERLINECONTROLS mixerMicLineControls;
							memset(&mixerMicLineControls, 0, sizeof(MIXERLINECONTROLS));
							mixerMicLineControls.cbStruct  = sizeof(MIXERLINECONTROLS);
							mixerMicLineControls.dwLineID  = mixerSrcLine.dwLineID;
							mixerMicLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
							mixerMicLineControls.cControls = 1;
							mixerMicLineControls.pamxctrl  = &mixerMicVolumeControl;
							mixerMicLineControls.cbmxctrl  = sizeof(MIXERCONTROL);

							ret = ::mixerGetLineControls
							(
								(HMIXEROBJ)hMixer,
								&mixerMicLineControls,
								MIXER_GETLINECONTROLSF_ONEBYTYPE
							);

							if (ret == MMSYSERR_NOERROR)
							{
								MIXERCONTROLDETAILS_UNSIGNED volumeLevel[2];
								volumeLevel[0].dwValue = dwVolume & 0x0ffff;
								volumeLevel[1].dwValue = volumeLevel[0].dwValue;

								MIXERCONTROLDETAILS mixerControlDetails;
								mixerControlDetails.cbStruct    = sizeof(MIXERCONTROLDETAILS);
								mixerControlDetails.dwControlID = mixerMicLineControls.pamxctrl->dwControlID;
								mixerControlDetails.cMultipleItems = 0;
								mixerControlDetails.cChannels = mixerSrcLine.cChannels;
								mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED) * mixerSrcLine.cChannels;
								mixerControlDetails.paDetails = volumeLevel;

								ret = ::mixerSetControlDetails
								(
									(HMIXEROBJ)hMixer,
									&mixerControlDetails,
									MIXER_SETCONTROLDETAILSF_VALUE
								);

								m_waveInVolume = dwVolume;
							}

							break;
						}
					}

					// Select microphone as the recording source.
					MIXERCONTROL mixerRecControl;
					memset(&mixerRecControl, 0, sizeof(MIXERCONTROL));
					mixerRecControl.cbStruct = sizeof(MIXERCONTROL);

					MIXERLINECONTROLS mixerRecLineControls;
					memset(&mixerRecLineControls, 0, sizeof(MIXERLINECONTROLS));
					mixerRecLineControls.cbStruct  = sizeof(MIXERLINECONTROLS);
					mixerRecLineControls.dwLineID  = mixerDestLine.dwLineID;
					mixerRecLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
					mixerRecLineControls.cControls = 1;
					mixerRecLineControls.pamxctrl  = &mixerRecControl;
					mixerRecLineControls.cbmxctrl  = sizeof(MIXERCONTROL);

					ret = ::mixerGetLineControls
					(
						(HMIXEROBJ)hMixer,
						&mixerRecLineControls,
						MIXER_GETLINECONTROLSF_ONEBYTYPE
					);

					if (ret == MMSYSERR_NOERROR)
					{
						MIXERCONTROLDETAILS mixerControlDetails;
						mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
						mixerControlDetails.dwControlID = mixerRecLineControls.pamxctrl->dwControlID;
						mixerControlDetails.cMultipleItems = mixerDestLine.cConnections;
						mixerControlDetails.cChannels = mixerDestLine.cChannels;

						UINT BooleanArraySize = mixerControlDetails.cMultipleItems * sizeof(MIXERCONTROLDETAILS_BOOLEAN);
						LPMIXERCONTROLDETAILS_BOOLEAN BooleanArray = new MIXERCONTROLDETAILS_BOOLEAN[mixerControlDetails.cMultipleItems];

						if (BooleanArray)
						{
							ret = ::mixerGetControlDetails
							(
								(HMIXEROBJ)hMixer,
								&mixerControlDetails,
								MIXER_GETCONTROLDETAILSF_VALUE
							);

                            BooleanArray[srcConnection].fValue = 1; /* This "srcConnection" comes from above. */
							mixerControlDetails.cbDetails = BooleanArraySize;
							mixerControlDetails.paDetails = BooleanArray;

							ret = ::mixerSetControlDetails
							(
								(HMIXEROBJ)hMixer,
								&mixerControlDetails,
								MIXER_SETCONTROLDETAILSF_VALUE
							);

							delete BooleanArray;
						}
					}
				}

				ret = ::mixerClose(hMixer);
			}
		}
	}
	
	if (uDirection & WAVE_FORM_FLOW_OUT)
	{
		if (m_hWaveOut)
		{
			ret = ::waveOutSetVolume(m_hWaveOut, dwVolume);
			m_waveOutVolume = dwVolume;
		}
	}

	if (ret == MMSYSERR_NOERROR)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/****************************************************************************
 * CWaveForm::Read()
 ****************************************************************************
 * Read wave frames in waiting list.
 */
BOOL
CWaveForm::Read
(
	PPWAVEFRAME	ppwaveFrame
)
{
	if (m_waveInState == WAVE_STATE_START)
	{
		if ((m_uDirection & WAVE_FORM_FLOW_IN) && m_hWaveIn)
		{
			EnterCriticalSection(&m_waveInFrameWaitingListLock);

			if (!m_waveInFrameWaitingList.IsEmpty())
			{
				*ppwaveFrame = m_waveInFrameWaitingList.RemoveHead();
			}
			else
			{
				*ppwaveFrame = NULL;
			}

			LeaveCriticalSection(&m_waveInFrameWaitingListLock);

			return TRUE;
		}
	}
	else
	{
		*ppwaveFrame = NULL;
	}

	return FALSE;
}

/****************************************************************************
 * CWaveForm::ReadDone()
 ****************************************************************************
 * Return used wave frames.
 */
BOOL
CWaveForm::ReadDone
(
	PWAVEFRAME	waveFrame
)
{
	if (waveFrame)
	{
		if ((m_uDirection & WAVE_FORM_FLOW_IN) && m_hWaveIn)
		{
			// Clear used frame.
			waveFrame->waveHdr.dwBytesRecorded = 0;

			// Reuse it in free list.
			EnterCriticalSection(&m_waveInFrameFreeListLock);

			m_waveInFrameFreeList.AddTail(waveFrame);

			LeaveCriticalSection(&m_waveInFrameFreeListLock);

			return TRUE;
		}
	}

	return FALSE;
}

/****************************************************************************
 * CWaveForm::Write()
 ****************************************************************************
 * Ask to allocate wave frame.
 */
BOOL
CWaveForm::Write
(
	PPWAVEFRAME	ppwaveFrame
)
{
	if ((m_uDirection & WAVE_FORM_FLOW_OUT) && m_hWaveOut)
	{
		EnterCriticalSection(&m_waveOutFrameFreeListLock);

		if (!m_waveOutFrameFreeList.IsEmpty())
		{
			*ppwaveFrame = m_waveOutFrameFreeList.RemoveHead();
		}
		else
		{
			PWAVEFRAME waveFrame = NULL;
			waveFrame = new WAVEFRAME;

			if (waveFrame)
			{
				waveFrame->waveHdr.lpData          = waveFrame->Data;
				waveFrame->waveHdr.dwBufferLength  = MAX_WAVE_BUFFER_LEN;
				waveFrame->waveHdr.dwBytesRecorded = 0;
				waveFrame->waveHdr.dwFlags         = 0;
				waveFrame->waveHdr.dwLoops         = 0;
				waveFrame->waveHdr.dwUser          = 0;
				waveFrame->waveHdr.lpNext          = 0;

				MMRESULT ret = ::waveOutPrepareHeader
								(
									m_hWaveOut,
									(LPWAVEHDR)waveFrame,
									sizeof(WAVEHDR)
								);

				*ppwaveFrame = waveFrame;
			}
		}

		LeaveCriticalSection(&m_waveOutFrameFreeListLock);

		return TRUE;
	}

	return FALSE;
}

/****************************************************************************
 * CWaveForm::Write()
 ****************************************************************************
 * Write data to preallocated wave frame.
 */
BOOL
CWaveForm::Write
(
	PWAVEFRAME	waveFrame
)
{
	if (waveFrame)
	{
		if (m_waveOutState == WAVE_STATE_START)
		{
			if ((m_uDirection & WAVE_FORM_FLOW_OUT) && m_hWaveOut)
			{
				EnterCriticalSection(&m_waveOutFrameWaitingListLock);

				// Queue this coming frame.
				if (m_waveOutFrameWaitingList.GetCount() < MAX_WAVE_NUM)
				{
					m_waveOutFrameWaitingList.AddTail(waveFrame);
				}
				else // Waiting list is full.
				{
					EnterCriticalSection(&m_waveOutFrameFreeListLock);

					if (m_waveOutFrameFreeList.GetCount() < MAX_WAVE_NUM)
					{
						m_waveOutFrameFreeList.AddTail(waveFrame);
					}
					else // Free list is also full.
					{
						MMRESULT ret = ::waveOutUnprepareHeader
										(
											m_hWaveOut,
											(LPWAVEHDR)waveFrame,
											sizeof(WAVEHDR)
										);

						delete waveFrame;
					}

					LeaveCriticalSection(&m_waveOutFrameFreeListLock);
				}

				// Fetch new frame from waiting list and send to device.
				if (!m_waveOutFrameWaitingList.IsEmpty() &&
					(m_dwWaveOutByteInDevice < MAX_BUFFER_IN_QUEUE))
				{
					PWAVEFRAME newWaveFrame = NULL;
					newWaveFrame = m_waveOutFrameWaitingList.RemoveHead();

					if (newWaveFrame)
					{
						MMRESULT ret = ::waveOutWrite
										(
											m_hWaveOut,
											(LPWAVEHDR)newWaveFrame,
											sizeof(WAVEHDR)
										);

						m_dwWaveOutByteInDevice += newWaveFrame->waveHdr.dwBufferLength;
					}
				}

				LeaveCriticalSection(&m_waveOutFrameWaitingListLock);

				return TRUE;
			}
		}
		else
		{
			EnterCriticalSection(&m_waveOutFrameFreeListLock);

			if (m_waveOutFrameFreeList.GetCount() < MAX_WAVE_NUM)
			{
				m_waveOutFrameFreeList.AddTail(waveFrame);
			}
			else // Free list is full.
			{
				MMRESULT ret = ::waveOutUnprepareHeader
								(
									m_hWaveOut,
									(LPWAVEHDR)waveFrame,
									sizeof(WAVEHDR)
								);

				delete waveFrame;
			}

			LeaveCriticalSection(&m_waveOutFrameFreeListLock);
		}
	}

	return FALSE;
}

/****************************************************************************
 * CWaveForm::WaveInProcess()
 ****************************************************************************
 * Process wave in buffers.
 */
void
CWaveForm::WaveInProcess()
{
    while (TRUE)
    {
        WaitForSingleObject(m_hWaveInProcessEvent, INFINITE);

        if (m_bWaveInProcessFlag == FALSE) break;

        while (m_waveInFrameProcessList.GetCount() > 0)
        {
            PWAVEFRAME newWaveFrame = NULL;
            EnterCriticalSection(&m_waveInFrameProcessListLock);
            newWaveFrame = m_waveInFrameProcessList.RemoveHead();
            LeaveCriticalSection(&m_waveInFrameProcessListLock);        
            OnWaveInData(newWaveFrame);
        }
    }
}

/****************************************************************************
 * CWaveForm::WaveOutProcess()
 ****************************************************************************
 * Process wave out buffers.
 */
void
CWaveForm::WaveOutProcess()
{
    while (TRUE)
    {
        WaitForSingleObject(m_hWaveOutProcessEvent, INFINITE);

        if (m_bWaveOutProcessFlag == FALSE) break;

        while (m_waveOutFrameProcessList.GetCount() > 0)
        {
            PWAVEFRAME newWaveFrame = NULL;
            EnterCriticalSection(&m_waveOutFrameProcessListLock);
            newWaveFrame = m_waveOutFrameProcessList.RemoveHead();
            LeaveCriticalSection(&m_waveOutFrameProcessListLock);        
            OnWaveOutDone(newWaveFrame);
        }
    }
}

/****************************************************************************
 * CWaveForm::OnWaveInData()
 ****************************************************************************
 * Wave in data handler.
 */
void
CWaveForm::OnWaveInData
(
    PWAVEFRAME waveFrame
)
{
	if (m_dwWaveInByteInDevice != 0)
	{
		m_dwWaveInByteInDevice -= waveFrame->waveHdr.dwBufferLength;
	}

	/////////////////////////////////////////////////////////////////////////
	// Process incoming wave in data when device is ON.
	/////////////////////////////////////////////////////////////////////////
    if (waveFrame->waveHdr.dwBytesRecorded != 0)
	{
		EnterCriticalSection(&m_waveInFrameWaitingListLock);

		if (m_waveInFrameWaitingList.GetCount() < MAX_WAVE_NUM)
		{
			m_waveInFrameWaitingList.AddTail(waveFrame);
		}
		else // Waiting list is full.
		{
			EnterCriticalSection(&m_waveInFrameFreeListLock);

			if (m_waveInFrameFreeList.GetCount() < MAX_WAVE_NUM)
			{
				waveFrame->waveHdr.dwBytesRecorded = 0;
				m_waveInFrameFreeList.AddTail(waveFrame);
			}
			else // Free list is also full.
			{
				MMRESULT ret = ::waveInUnprepareHeader
								(
									m_hWaveIn,
									(LPWAVEHDR)waveFrame,
									sizeof(WAVEHDR)
								);

				delete waveFrame;
			}

			LeaveCriticalSection(&m_waveInFrameFreeListLock);
		}

		LeaveCriticalSection(&m_waveInFrameWaitingListLock);

		// Notify client.
		Invoke(WM_STREAM_NOTIFY, SUB_EVENT_WAVE_IN, 0);
	}
	else // (waveFrame->waveHdr.dwBytesRecorded == 0)
	{
		EnterCriticalSection(&m_waveInFrameFreeListLock);

		if (m_waveInFrameFreeList.GetCount() < MAX_WAVE_NUM)
		{
			waveFrame->waveHdr.dwBytesRecorded = 0;
			m_waveInFrameFreeList.AddTail(waveFrame);
		}
		else // Free list is full.
		{
			MMRESULT ret = ::waveInUnprepareHeader
							(
								m_hWaveIn,
								(LPWAVEHDR)waveFrame,
								sizeof(WAVEHDR)
							);

			delete waveFrame;
		}

		LeaveCriticalSection(&m_waveInFrameFreeListLock);
	}

	/////////////////////////////////////////////////////////////////////////
	// Find or allocate a free wave in frame and add it into device.
	/////////////////////////////////////////////////////////////////////////
	if ((m_waveInState == WAVE_STATE_START) && 
        (m_dwWaveInByteInDevice < MAX_BUFFER_IN_QUEUE))
	{
		PWAVEFRAME newWaveFrame = NULL;
        MMRESULT ret = MMSYSERR_NOERROR;

		EnterCriticalSection(&m_waveInFrameFreeListLock);

		if (!m_waveInFrameFreeList.IsEmpty())
		{
			newWaveFrame = m_waveInFrameFreeList.RemoveHead();
		}
		else
		{
			newWaveFrame = new WAVEFRAME;

			if (newWaveFrame)
			{
				newWaveFrame->waveHdr.lpData          = newWaveFrame->Data;
				newWaveFrame->waveHdr.dwBufferLength  = MAX_WAVE_BUFFER_LEN;
				newWaveFrame->waveHdr.dwBytesRecorded = 0;
				newWaveFrame->waveHdr.dwFlags         = 0;
				newWaveFrame->waveHdr.dwLoops         = 0;
				newWaveFrame->waveHdr.dwUser          = 0;
				newWaveFrame->waveHdr.lpNext          = 0;

				ret = ::waveInPrepareHeader
						(
							m_hWaveIn,
							(LPWAVEHDR)newWaveFrame,
							sizeof(WAVEHDR)
						);
			}
		}

		LeaveCriticalSection(&m_waveInFrameFreeListLock);

		if (newWaveFrame)
		{
			ret = ::waveInAddBuffer
					(
						m_hWaveIn,
						(LPWAVEHDR)newWaveFrame,
						sizeof(WAVEHDR)
					);

			m_dwWaveInByteInDevice += newWaveFrame->waveHdr.dwBufferLength;
		}
	}
}

/****************************************************************************
 * CWaveForm::OnWaveOutDone()
 ****************************************************************************
 * Wave out done handler.
 */
void
CWaveForm::OnWaveOutDone
(
    PWAVEFRAME waveFrame
)
{
	if (m_dwWaveOutByteInDevice != 0)
	{
		m_dwWaveOutByteInDevice -= waveFrame->waveHdr.dwBufferLength;
	}

	/////////////////////////////////////////////////////////////////////////
	// Processing used wave out frame.
	/////////////////////////////////////////////////////////////////////////
	EnterCriticalSection(&m_waveOutFrameFreeListLock);

	if (m_waveOutFrameFreeList.GetCount() < MAX_WAVE_NUM)
	{
		m_waveOutFrameFreeList.AddTail(waveFrame);
	}
	else // Free list is full.
	{
		MMRESULT ret = ::waveOutUnprepareHeader
							(
								m_hWaveOut,
								(LPWAVEHDR)waveFrame,
								sizeof(WAVEHDR)
							);

		delete waveFrame;
	}

	LeaveCriticalSection(&m_waveOutFrameFreeListLock);

	/////////////////////////////////////////////////////////////////////////
	// If device is running, fetch new data from waiting list.
	/////////////////////////////////////////////////////////////////////////
	if ((m_waveOutState == WAVE_STATE_START) &&
		(m_dwWaveOutByteInDevice < MAX_BUFFER_IN_QUEUE))
	{
		PWAVEFRAME newWaveFrame = NULL;
        MMRESULT ret = MMSYSERR_NOERROR;

		EnterCriticalSection(&m_waveOutFrameWaitingListLock);

		if (!m_waveOutFrameWaitingList.IsEmpty())
		{
			newWaveFrame = m_waveOutFrameWaitingList.RemoveHead();
		}

		LeaveCriticalSection(&m_waveOutFrameWaitingListLock);

		if (newWaveFrame)
		{
			ret = ::waveOutWrite
					(
						m_hWaveOut,
						(LPWAVEHDR)newWaveFrame,
						sizeof(WAVEHDR)
					);

			m_dwWaveOutByteInDevice += newWaveFrame->waveHdr.dwBufferLength;
		}
	}
}

/****************************************************************************
 * CWaveForm::Compress()
 ****************************************************************************
 * Compress data.
 */
BOOL
CWaveForm::Compress
(
	PVOID	inData,
	ULONG	inDataLength,
	PVOID	outData,
	PULONG	outDataLength
)
{
	memcpy(outData, inData, inDataLength);
	*outDataLength = inDataLength;
	return TRUE;
}

/****************************************************************************
 * CWaveForm::Uncompress()
 ****************************************************************************
 * Uncompress data.
 */
BOOL
CWaveForm::Uncompress
(
	PVOID	inData,
	ULONG	inDataLength,
	PVOID	outData,
	PULONG	outDataLength
)
{
	memcpy(outData, inData, inDataLength);
	*outDataLength = inDataLength;
	return TRUE;
}

/****************************************************************************
 * WaveInCallBackRoutine()
 ****************************************************************************
 * Wave in callback routine.
 */
void
CALLBACK
WaveInCallBackRoutine
(
	HWAVEIN  hwi,
	UINT     uMsg,
	DWORD    dwInstance,
	DWORD    dwParam1,
	DWORD    dwParam2
)
{
	CWaveForm * that = (CWaveForm *)dwInstance;
    PWAVEFRAME waveFrame = (PWAVEFRAME)dwParam1;

	if (hwi == that->m_hWaveIn)
	{
		switch (uMsg)
		{
			case WIM_OPEN:
				break;

			case WIM_DATA:
                EnterCriticalSection(&that->m_waveInFrameProcessListLock);
                that->m_waveInFrameProcessList.AddTail(waveFrame);
                LeaveCriticalSection(&that->m_waveInFrameProcessListLock);
                SetEvent(that->m_hWaveInProcessEvent);
				break;

			case WIM_CLOSE:
				break;
		}
	}
}

/****************************************************************************
 * WaveOutCallBackRoutine()
 ****************************************************************************
 * Wave out call back routine.
 */
void
CALLBACK
WaveOutCallBackRoutine
(
	HWAVEOUT  hwo,
	UINT      uMsg,
	DWORD     dwInstance,
	DWORD     dwParam1,
	DWORD     dwParam2
)
{
	CWaveForm * that = (CWaveForm *)dwInstance;
    PWAVEFRAME waveFrame = (PWAVEFRAME)dwParam1;

	if (hwo == that->m_hWaveOut)
	{
		switch (uMsg)
		{
			case WOM_OPEN:
				break;

			case WOM_DONE:
                EnterCriticalSection(&that->m_waveOutFrameProcessListLock);
                that->m_waveOutFrameProcessList.AddTail(waveFrame);
                LeaveCriticalSection(&that->m_waveOutFrameProcessListLock);
                SetEvent(that->m_hWaveOutProcessEvent);
				break;

			case WOM_CLOSE:
				break;
		}
	}
}

/****************************************************************************
 * ThreadProcIn()
 ****************************************************************************
 * Wave in data process thread.
 */
DWORD
ThreadProcIn
(
    LPVOID lpData
)
{
    CWaveForm * pi = (CWaveForm *)lpData;
    pi->WaveInProcess();
    return 0;
}

/****************************************************************************
 * ThreadProcOut()
 ****************************************************************************
 * Wave out data process thread.
 */
DWORD
ThreadProcOut
(
    LPVOID lpData
)
{
    CWaveForm * po = (CWaveForm *)lpData;
    po->WaveOutProcess();
    return 0;
}
