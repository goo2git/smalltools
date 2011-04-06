/*
+===========================================================================+
|				Copyright (C) Direct Line Corp. 1999-2000.					|
+---------------------------------------------------------------------------+
| File Name:																|
|																			|
|	WaveForm.h																|
|																			|
+---------------------------------------------------------------------------+
| Descriptions:																|
|																			|
|	Win32 wave form API object oriented definition.							|
|																			|
+---------------------------------------------------------------------------+
| Developer(s):																|
|																			|
|	Xu Wen Bin.																|
|																			|
+===========================================================================+
|                         C H A N G E     L O G                             |
+---------------------------------------------------------------------------+
|																			|
|	07-20-01	1.00	Created.											|
|	08-08-01	1.01	Modified.											|
|	02-22-06	1.02	Modified.											|
|																			|
+---------------------------------------------------------------------------+
| Notes:	 																|
|																			|
|	1. Wave device sequence : open()->start()->stop()->close().				|
|	2. Support seperate operation on wave in and out object.				|
|																			|
+===========================================================================+
*/

#ifndef _WAVEFORM_H_
#define _WAVEFORM_H_

#include <Mmsystem.h>
#include <Mmreg.h>
#include <Msacm.h>
#include "Interface.h"

/****************************************************************************
 * Limitation for number of wave buffer can be queued in list.
 ****************************************************************************
 */
#define MIN_WAVE_NUM		4
#define MAX_WAVE_NUM		16
#define MAX_WAVE_BUFFER_LEN	8192
#define MAX_BUFFER_IN_QUEUE	MAX_WAVE_BUFFER_LEN * (MAX_WAVE_NUM / 2)

#define WAVE_FORM_FLOW_IN	0x00000001
#define WAVE_FORM_FLOW_OUT	0x00000002
#define WAVE_FORM_FLOW_BOTH	0x00000003

#define VALIDATE(x,y) x > y ? (x = y) : (y = x)

/****************************************************************************
 * External messages borrowed from [Thread.h].
 ****************************************************************************
 */
#define WM_STREAM_NOTIFY		WM_USER + 1
#define SUB_EVENT_WAVE_IN		0x00000001
#define SUB_EVENT_SOCKET_IN		0x00000002

#define WM_STREAM_CONTROL		WM_USER + 2
#define SUB_EVENT_WAVE_QUIT		0x00000001
#define SUB_EVENT_SOCKET_QUIT	0x00000002

/****************************************************************************
 * Internal wave frame structure.
 ****************************************************************************
 */
typedef struct stWaveFrame
{
	WAVEHDR waveHdr;
	CHAR    Data[MAX_WAVE_BUFFER_LEN];
} WAVEFRAME, *PWAVEFRAME;

typedef PWAVEFRAME * PPWAVEFRAME;

/****************************************************************************
 * Wave device state.
 ****************************************************************************
 */
#define WAVE_STATE_OPEN     0x0001
#define WAVE_STATE_CLOSE    0x0002
#define WAVE_STATE_START	0x0003
#define WAVE_STATE_STOP		0x0004
#define WAVE_STATE_RESET	0x0005
#define WAVE_STATE_PAUSE	0x0006
#define WAVE_STATE_RESTART	0x0007

/****************************************************************************
 * Wave device callback routines.
 ****************************************************************************
 */
static void CALLBACK WaveInCallBackRoutine
(
	HWAVEIN   hwi,
	UINT      uMsg,
	DWORD     dwInstance,
	DWORD     dwParam1,
	DWORD     dwParam2
);

static void CALLBACK WaveOutCallBackRoutine
(
	HWAVEOUT  hwo,
	UINT      uMsg,
	DWORD     dwInstance,
	DWORD     dwParam1,
	DWORD     dwParam2
);

static DWORD ThreadProcIn
(
    PVOID pData
);

static DWORD ThreadProcOut
(
    PVOID pData
);

/****************************************************************************
 * CWaveForm class definition.
 ****************************************************************************
 *
 */
class CWaveForm : public IIUnknown,
                  public IIStreamDevice,
				  public IIDataSource<PWAVEFRAME>,
				  public IIDataSink<PWAVEFRAME>,
                  public CAdvise
{
public:
    static BOOL SoundCardExists()
    {
        WAVEINCAPS waveInCaps;

        if (::waveInGetNumDevs() == 0)
        {
            ::MessageBox(0, _T("No sound card installed"), _T("WaveForm"), MB_OK);
            return FALSE;
        }

        if (waveInGetDevCaps(0, &waveInCaps, sizeof(WAVEINCAPS)) != MMSYSERR_NOERROR)
        {
            ::MessageBox(0, _T("Cannot determine sound card capabilities"), _T("WaveForm"), MB_OK);
            return FALSE;
        }

        return TRUE;
    }

public:
	CWaveForm
	(
		UINT           uDirection,
		UINT           uWaveInDeviceID,
		UINT           uWaveOutDeviceID,
		WAVEFORMATEX * lpWaveInFormatEx,
		WAVEFORMATEX * lpWaveOutFormatEx,
		DWORD          dwWaveInVolume,
		DWORD          dwWaveOutVolume
	);

	~CWaveForm();

	/////////////////////////////////////////////////////////////////////////
	// Standard interface implementation.
	/////////////////////////////////////////////////////////////////////////
	IMP_IIUNKNOWN();
	IMP_IISTREAMDEVICE();
	IMP_IIDATASOURCE(PWAVEFRAME);
	IMP_IIDATASINK(PWAVEFRAME);

	/////////////////////////////////////////////////////////////////////////
	// WaveForm spefific APIs.
	/////////////////////////////////////////////////////////////////////////
    BOOL GetFormat(UINT uDirection, LPWAVEFORMATEX lpWaveFormatEx);
	BOOL SetFormat(UINT uDirection, LPWAVEFORMATEX lpWaveFormatEx);
	BOOL UpdateFormat(UINT uDirection, LPWAVEFORMATEX lpWaveFormatEx);

	BOOL GetVolume(UINT uDirection, PDWORD pVolume);
	BOOL SetVolume(UINT uDirection, DWORD dwVolume);

	BOOL Compress(PVOID inData, ULONG inDataLength, PVOID outData, PULONG outDataLength);
	BOOL Uncompress(PVOID inData, ULONG inDataLength, PVOID outData, PULONG outDataLength);

private:
	LONG m_RefCount;

	UINT m_uDirection;
	UINT m_waveInState;
	UINT m_waveOutState;
	UINT m_waveInDeviceID;
	UINT m_waveOutDeviceID;

	WAVEFORMATEX m_waveInFormatEx;
	WAVEFORMATEX m_waveOutFormatEx;

	HWAVEIN  m_hWaveIn;
	HWAVEOUT m_hWaveOut;

	DWORD m_waveInVolume;
	DWORD m_waveOutVolume;

	DWORD m_dwWaveInByteInDevice;
	DWORD m_dwWaveOutByteInDevice;

	CRITICAL_SECTION m_waveInFrameFreeListLock;
	CRITICAL_SECTION m_waveInFrameWaitingListLock;
    CRITICAL_SECTION m_waveInFrameProcessListLock;
	CList<PWAVEFRAME,PWAVEFRAME&> m_waveInFrameFreeList;
	CList<PWAVEFRAME,PWAVEFRAME&> m_waveInFrameWaitingList;
    CList<PWAVEFRAME,PWAVEFRAME&> m_waveInFrameProcessList;

	CRITICAL_SECTION m_waveOutFrameFreeListLock;
	CRITICAL_SECTION m_waveOutFrameWaitingListLock;
    CRITICAL_SECTION m_waveOutFrameProcessListLock;
	CList<PWAVEFRAME,PWAVEFRAME&> m_waveOutFrameFreeList;
	CList<PWAVEFRAME,PWAVEFRAME&> m_waveOutFrameWaitingList;
    CList<PWAVEFRAME,PWAVEFRAME&> m_waveOutFrameProcessList;

    BOOL m_bWaveInProcessFlag;
    HANDLE m_hWaveInProcessEvent;
    HANDLE m_hWaveInProcessThread;

    BOOL m_bWaveOutProcessFlag;
    HANDLE m_hWaveOutProcessEvent;
    HANDLE m_hWaveOutProcessThread;

    void WaveInProcess();
    void WaveOutProcess();

    void OnWaveInData(PWAVEFRAME waveFrame);
	void OnWaveOutDone(PWAVEFRAME waveFrame);

    friend static void CALLBACK WaveInCallBackRoutine
	(
		HWAVEIN  hwi,
		UINT     uMsg,
		DWORD    dwInstance,
		DWORD    dwParam1,
		DWORD    dwParam2
	);
	friend static void CALLBACK WaveOutCallBackRoutine
	(
		HWAVEOUT  hwo,
		UINT      uMsg,
		DWORD     dwInstance,
		DWORD     dwParam1,
		DWORD     dwParam2
	);
    friend static DWORD ThreadProcIn
    (
        LPVOID lpData
    );
	friend static DWORD ThreadProcOut
    (
        LPVOID lpData
    );
};

#endif // _WAVEFORM_H_
