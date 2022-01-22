#pragma once

#include <objbase.h>
#include <speech.h>

struct _VOICE_INFO;

class CTestNotify : public ITTSNotifySink {
	public:
		CTestNotify (struct _VOICE_INFO *VoiceInfo);
		~CTestNotify (void);

		// IUnkown members that delegate to m_punkOuter
		// Non-delegating object IUnknown
		STDMETHODIMP		   QueryInterface (REFIID, LPVOID FAR *);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);

		// ITTSNotifySink
		STDMETHOD (AttribChanged)  (DWORD);
		STDMETHOD (AudioStart)	   (QWORD);
		STDMETHOD (AudioStop)	   (QWORD);
		STDMETHOD (Visual)		   (QWORD, CHAR, CHAR, DWORD, PTTSMOUTH);
	private:
		struct _VOICE_INFO *VoiceInfo;
};
typedef CTestNotify *PCTestNotify;


typedef struct _VOICE_INFO {
	PITTSCENTRAL	pITTSCentral = NULL;
	TTSMODEINFO		ModeInfo;
	PIAUDIOFILE		pIAF = NULL;
	PITTSATTRIBUTES	pITTSAttributes = NULL;

	WORD defPitch, minPitch, maxPitch;
	DWORD defSpeed, minSpeed, maxSpeed;

	PCTestNotify pNotify;
	BOOL VoiceDone = FALSE;
} VOICE_INFO, *PVOICE_INFO;

extern __declspec(dllexport) VOID TTSCleanup(
	PBYTE TTS
);

extern __declspec(dllexport) BOOL GetTTS(
	PVOICE_INFO pVoiceInfo,
	WORD Pitch,
	DWORD Speed,
	LPCSTR Text,
	PUINT64 Len
);

extern __declspec(dllexport) VOID DeinitializeForVoice(
	PVOICE_INFO pVoiceInfo
);

extern __declspec(dllexport) LPCSTR EnumerateVoices(
	VOID
);

extern __declspec(dllexport) VOID FreeEnumerateVoices(
	LPCSTR Voices
);

extern __declspec(dllexport) BOOL InitializeForVoice(
	LPCSTR Voice,
	PVOICE_INFO pVoiceInfo
);