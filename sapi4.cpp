#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <mmsystem.h>
#include <initguid.h>
#include <objbase.h>
#include <objerror.h>
#include <ole2ver.h>

#include <math.h>
#include <time.h>

#include <speech.h>
#include "sapi4.hpp"

CTestNotify::CTestNotify(PVOICE_INFO VoiceInfo)
{
	this->VoiceInfo = VoiceInfo;
}

CTestNotify::~CTestNotify()
{
// this space intentionally left blank
}

STDMETHODIMP CTestNotify::QueryInterface(
	REFIID riid,
	LPVOID *ppv)
{
	*ppv = NULL;

	/* always return our IUnknown for IID_IUnknown */
	if (IsEqualIID (riid, IID_IUnknown) || IsEqualIID(riid,IID_ITTSNotifySink)) {
		*ppv = (LPVOID) this;
		return S_OK;
	}

	// otherwise, cant find
	return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_ (ULONG) CTestNotify::AddRef()
{
	return 1;
}

STDMETHODIMP_(ULONG) CTestNotify::Release()
{
	return 1;
}

STDMETHODIMP CTestNotify::AttribChanged(
	DWORD dwAttribID)
{
	return NOERROR;
}

STDMETHODIMP CTestNotify::AudioStart(
	QWORD qTimeStamp)
{
	return NOERROR;
}

STDMETHODIMP CTestNotify::AudioStop(
	QWORD qTimeStamp)
{
	this->VoiceInfo->pIAF->Flush();
	this->VoiceInfo->VoiceDone = TRUE;

	return NOERROR;
}

STDMETHODIMP CTestNotify::Visual(
	QWORD qTimeStamp,
	CHAR cIPAPhoneme,
	CHAR cEnginePhoneme,
	DWORD dwHints,
	PTTSMOUTH pTTSMouth)
{
	return NOERROR;
}

PITTSCENTRAL FindAndSelect(
	PVOICE_INFO pVoiceInfo)
{
	HRESULT hRes;
	TTSMODEINFO ttsResult;
	PITTSFIND pITTSFind;
	PITTSCENTRAL pITTSCentral;


	hRes = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSFind, (void**)&pITTSFind);
	if (FAILED(hRes))
		return NULL;

	hRes = pITTSFind->Find(&pVoiceInfo->ModeInfo, NULL, &ttsResult);
	if (FAILED(hRes))
		goto failRelease;

	if (strcmp(ttsResult.szModeName, pVoiceInfo->ModeInfo.szModeName) != 0) {
		PITTSENUM pITTSEnum;

		hRes = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSEnum, (void**)&pITTSEnum);
		if (FAILED(hRes))
			goto failRelease;
	  
		TTSMODEINFO nTTSInfo;
		while (!pITTSEnum->Next(1, &nTTSInfo, NULL)) {
			if (strcmp(nTTSInfo.szModeName, pVoiceInfo->ModeInfo.szModeName) == 0) {
				ttsResult = nTTSInfo;
				break;
			}
		}
	
		pITTSEnum->Release();
	}
	
	hRes = CoCreateInstance(CLSID_AudioDestFile, NULL, CLSCTX_ALL, IID_IAudioFile, (void**)&pVoiceInfo->pIAF);
	if (FAILED(hRes))
		goto failRelease;

	hRes = pITTSFind->Select(ttsResult.gModeID, &pITTSCentral, pVoiceInfo->pIAF);
	if (FAILED(hRes))
		goto failRelease;

	pVoiceInfo->ModeInfo = ttsResult;
	
	pITTSFind->Release();

	return pITTSCentral;
	
failRelease:
	pITTSFind->Release();
	return NULL;
}

BOOL BeginOLE()
{
	DWORD dwVer = CoBuildVersion();

	if (rmm != HIWORD(dwVer))
		return FALSE;

	if (FAILED(CoInitialize(NULL)))
		return FALSE;

	return TRUE;
}

BOOL EndOLE()
{
	CoUninitialize();
	return TRUE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
			LARGE_INTEGER ticks;
			QueryPerformanceCounter(&ticks);
			srand(ticks.QuadPart);

			if (!BeginOLE())
				return FALSE;
			break;
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			if (!EndOLE()) {
				return FALSE;
			}
			break;
	}

	return TRUE;
}

extern __declspec(dllexport) VOID TTSCleanup(
	PBYTE TTS
)
{
	free(TTS);
}

extern __declspec(dllexport) LPCSTR EnumerateVoices(
	VOID
)
{
	PITTSENUM pITTSEnum;
	HRESULT hRes;
	LPSTR ret = (LPSTR)malloc(1);
	ret[0] = '\0';
	DWORD curLen = 1;

	hRes = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSEnum, (void**)&pITTSEnum);
	if (FAILED(hRes))
		return NULL;
	
	TTSMODEINFO nTTSInfo;
	while (!pITTSEnum->Next(1, &nTTSInfo, NULL)) {
		DWORD vLen = strlen(nTTSInfo.szModeName);
		ret = (LPSTR)realloc(ret, curLen + vLen + 1);
		strcat(ret, nTTSInfo.szModeName);
		strcat(ret, "\n");
		curLen += vLen + 1;
	}

	pITTSEnum->Release();

	return ret;
}

extern __declspec(dllexport) VOID FreeEnumerateVoices(
	LPCSTR Voices
)
{
	free((void*)Voices);
}

extern __declspec(dllexport) BOOL GetTTS(
	PVOICE_INFO pVoiceInfo,
	WORD Pitch,
	DWORD Speed,
	LPCSTR Text,
	PUINT64 Len,
	LPSTR* OutFile
)
{
	WCHAR wszFile[17] = L"";
	for (int x = 0;x < 16;x++) {
		if (rand() % 2 == 0)
			(*OutFile)[x] = 'A' + (rand() % 26);
		else
			(*OutFile)[x] = 'a' + (rand() % 26);
	}
	
	(*OutFile)[16] = '\0';

	MultiByteToWideChar(CP_ACP, 0, *OutFile, -1, wszFile, sizeof(wszFile) / sizeof(WCHAR));
	if (pVoiceInfo->pIAF->Set(wszFile, 1)) {
		return FALSE;
	}
	
	pVoiceInfo->pITTSAttributes->PitchSet(Pitch);
	pVoiceInfo->pITTSAttributes->SpeedSet(Speed);
	
	SDATA data;
	data.dwSize = strlen(Text) + 1;
	data.pData = (PVOID)Text;

	pVoiceInfo->pITTSCentral->AudioReset();
	pVoiceInfo->pITTSCentral->TextData(CHARSET_TEXT, TTSDATAFLAG_TAGGED, data, NULL, IID_ITTSBufNotifySink);

	BOOL fGotMessage;
	MSG msg;
	while ((fGotMessage = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0 && fGotMessage != -1 && !pVoiceInfo->VoiceDone) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	pVoiceInfo->VoiceDone = FALSE;
	pVoiceInfo->pIAF->Flush();
	
	return TRUE;
}

extern __declspec(dllexport) VOID DeinitializeForVoice(
	PVOICE_INFO pVoiceInfo
)
{
	pVoiceInfo->pIAF->Release();
	pVoiceInfo->pITTSAttributes->Release();
	pVoiceInfo->pITTSCentral->Release();
	delete pVoiceInfo->pNotify;
}

extern __declspec(dllexport) BOOL InitializeForVoice(
	LPCSTR Voice,
	PVOICE_INFO pVoiceInfo
)
{
	// find the right object
	memset (&pVoiceInfo->ModeInfo, 0, sizeof(pVoiceInfo->ModeInfo));
	strcpy (pVoiceInfo->ModeInfo.szModeName, Voice);
	
	pVoiceInfo->pITTSCentral = FindAndSelect(pVoiceInfo);
	if (!pVoiceInfo->pITTSCentral) {
		return FALSE;
	}
	
	pVoiceInfo->pIAF->RealTimeSet(0xFFFF);

	if (FAILED(pVoiceInfo->pITTSCentral->QueryInterface(IID_ITTSAttributes, (void**)&(pVoiceInfo->pITTSAttributes)))) {
		return FALSE;
	}

	pVoiceInfo->pNotify = new CTestNotify(pVoiceInfo);

	DWORD dwRegKey;
	pVoiceInfo->pITTSCentral->Register((void*)pVoiceInfo->pNotify, IID_ITTSNotifySink, &dwRegKey);
	
	pVoiceInfo->pITTSAttributes->PitchGet(&pVoiceInfo->defPitch);
	pVoiceInfo->pITTSAttributes->SpeedGet(&pVoiceInfo->defSpeed);
	pVoiceInfo->pITTSAttributes->PitchSet(TTSATTR_MINPITCH);
	pVoiceInfo->pITTSAttributes->SpeedSet(TTSATTR_MINSPEED);
	pVoiceInfo->pITTSAttributes->PitchGet(&pVoiceInfo->minPitch);
	pVoiceInfo->pITTSAttributes->SpeedGet(&pVoiceInfo->minSpeed);
	pVoiceInfo->pITTSAttributes->PitchSet(TTSATTR_MAXPITCH);
	pVoiceInfo->pITTSAttributes->SpeedSet(TTSATTR_MAXSPEED);
	pVoiceInfo->pITTSAttributes->PitchGet(&pVoiceInfo->maxPitch);
	pVoiceInfo->pITTSAttributes->SpeedGet(&pVoiceInfo->maxSpeed);
	
	return TRUE;
}



