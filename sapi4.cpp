#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <mmsystem.h>
#include <initguid.h>
#include <objbase.h>
#include <objerror.h>
#include <ole2ver.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <math.h>
#include <time.h>

#include <speech.h>
#include "resource.h"

class CTestNotify : public ITTSNotifySink {
	public:
		CTestNotify (void);
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
};
typedef CTestNotify *PCTestNotify;


HINSTANCE		ghInstance;
PITTSCENTRAL	gpITTSCentral = NULL;
PITTSATTRIBUTES gpITTSAttributes = NULL;
CTestNotify		gNotify;
WORD			gwRealTime = 0xFFFFFFFF;
PIAUDIOFILE		gpIAF = NULL;
BOOL			SAMDone = FALSE;


CTestNotify::CTestNotify()
{
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
	gpIAF->Flush();
	SAMDone = TRUE;

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
	PTTSMODEINFO pTTSInfo)
{
	HRESULT hRes;
	TTSMODEINFO ttsResult;
	PITTSFIND pITTSFind;
	PITTSCENTRAL pITTSCentral;


	hRes = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSFind, (void**)&pITTSFind);
	if (FAILED(hRes))
		return NULL;

	hRes = pITTSFind->Find(pTTSInfo, NULL, &ttsResult);
	if (FAILED(hRes))
		goto failRelease;

	if (strcmp(ttsResult.szModeName, pTTSInfo->szModeName) != 0) {
		PITTSENUM pITTSEnum;
		printf("select voice fallback?\n");

		hRes = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSEnum, (void**)&pITTSEnum);
		if (FAILED(hRes))
			goto failRelease;
	  
		TTSMODEINFO nTTSInfo;
		while (!pITTSEnum->Next(1, &nTTSInfo, NULL)) {
			printf("V %s\n", nTTSInfo.szModeName);
			if (strcmp(nTTSInfo.szModeName, pTTSInfo->szModeName) == 0) {
				ttsResult = nTTSInfo;
				printf("select voice fallback\n");
				break;
			}
		}
	
		pITTSEnum->Release();
	}
	
	hRes = CoCreateInstance(CLSID_AudioDestFile, NULL, CLSCTX_ALL, IID_IAudioFile, (void**)&gpIAF);
	if (FAILED(hRes))
		goto failRelease;

	hRes = pITTSFind->Select(ttsResult.gModeID, &pITTSCentral, gpIAF);
	if (FAILED(hRes))
		goto failRelease;

	printf("selected voice: %s\n", ttsResult.szModeName);
	
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

SOCKET ClientSocket = INVALID_SOCKET;

BOOL InitWinSock(PCSTR Port)
{
	SOCKET ListenSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL;
	WSADATA wsaData;
	struct addrinfo hints;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	
	if (iResult != 0)
		return FALSE;
	
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", Port, &hints, &result);
	if (iResult != 0) {
		WSACleanup();
		return FALSE;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		freeaddrinfo(result);
		WSACleanup();
		return FALSE;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return FALSE;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		closesocket(ListenSocket);
		WSACleanup();
		return FALSE;
	}
	
	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	
	printf("SOCKET OK\n");

	// No longer need server socket
	closesocket(ListenSocket);
	
	return TRUE;
}

int PASCAL WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
	srand(time(NULL));
	char *port;
	char *voice;
	
	int i = 0;
	while (lpszCmdLine[i++] != ',');
	lpszCmdLine[i-1] = '\0';
	port = lpszCmdLine;
	printf("port: %s\n", port);
	
	voice = &(lpszCmdLine[i]);
	printf("voice: %s\n", voice);

AGAIN:

	TTSMODEINFO	ModeInfo;
	SDATA data;
	
	if (!BeginOLE())
		return 1;
	
	if (!InitWinSock(lpszCmdLine))
		return 1;

	// find the right object
	memset (&ModeInfo, 0, sizeof(ModeInfo));
	strcpy (ModeInfo.szModeName, voice);
	
	gpITTSCentral = FindAndSelect(&ModeInfo);
	if (!gpITTSCentral) {
		printf("no\n");
		return 1;
	}
	
	printf("out voice: %s\n", ModeInfo.szModeName);
	
	gpIAF->RealTimeSet(gwRealTime);

	if (FAILED(gpITTSCentral->QueryInterface(IID_ITTSAttributes, (void**)&gpITTSAttributes))) {
		printf("query interface failed\n");
		return 1;
	}
	
	WORD defPitch, minPitch, maxPitch;
	DWORD defSpeed, minSpeed, maxSpeed;
	gpITTSAttributes->PitchGet(&defPitch);
	gpITTSAttributes->SpeedGet(&defSpeed);
	gpITTSAttributes->PitchSet(TTSATTR_MINPITCH);
	gpITTSAttributes->SpeedSet(TTSATTR_MINSPEED);
	gpITTSAttributes->PitchGet(&minPitch);
	gpITTSAttributes->SpeedGet(&minSpeed);
	gpITTSAttributes->PitchSet(TTSATTR_MAXPITCH);
	gpITTSAttributes->SpeedSet(TTSATTR_MAXSPEED);
	gpITTSAttributes->PitchGet(&maxPitch);
	gpITTSAttributes->SpeedGet(&maxSpeed);
	
	WORD toSend[3];
	toSend[0] = defPitch;
	toSend[1] = minPitch;
	toSend[2] = maxPitch;
	
	int iSendResult = send(ClientSocket, (char*)toSend, 6, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	printf("pitch: %d, %d, %d\n", defPitch, minPitch, maxPitch);
	
	DWORD toSend2[3];
	toSend2[0] = defSpeed;
	toSend2[1] = minSpeed;
	toSend2[2] = maxSpeed;
	
	iSendResult = send(ClientSocket, (char*)toSend2, 12, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	printf("speed: %d, %d, %d\n", defSpeed, minSpeed, maxSpeed);
	
	DWORD dwRegKey;
	gpITTSCentral->Register((void*)&gNotify, IID_ITTSNotifySink, &dwRegKey);

	int iResult;
	char recvbuf[4096] { 0 };
	do {
		WCHAR wszFile[17] = L"";
		char rndFile[17] = "";
		for (int x = 0;x < 16;x++) {
			if (rand() % 2 == 0)
				rndFile[x] = 'A' + (rand() % 26);
			else
				rndFile[x] = 'a' + (rand() % 26);
		}
		
		MultiByteToWideChar(CP_ACP, 0, rndFile, -1, wszFile, sizeof(wszFile) / sizeof(WCHAR));
		if (gpIAF->Set(wszFile, 1)) {
			printf("file set failed\n");
			return 1;
		}
		
ping:
		
		/* text len */
		iResult = recv(ClientSocket, recvbuf, 2, 0);
		if (iResult <= 0) {
			printf("refuse0\n");
			break;
		}
		
		WORD len = *(WORD*)recvbuf;
		
		if (len > 4095) {
			printf("refuse1\n");
			break;
		}
		
		if (len == 0) {
			printf("ping\n");
			WORD one = 1;
			iSendResult = send(ClientSocket, (char*)&one, 2, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				break;
			}
			goto ping;
		}
		
		/* pitch */
		iResult = recv(ClientSocket, recvbuf, 2, 0);
		if (iResult <= 0) {
			printf("refuse2\n");
			break;
		}
		
		WORD pitch = *(WORD*)recvbuf;
		
		/* speed */
		iResult = recv(ClientSocket, recvbuf, 4, 0);
		if (iResult <= 0) {
			printf("refuse3\n");
			break;
		}
		
		DWORD speed = *(DWORD*)recvbuf;
		
		/* text */
		iResult = recv(ClientSocket, recvbuf, len, 0);
		if (iResult <= 0) {
			printf("refuse4\n");
			break;
		}
		
		printf("%d,%d\n", pitch, speed);
		
		gpITTSAttributes->PitchSet(pitch);
		gpITTSAttributes->SpeedSet(speed);
		
		data.dwSize = strlen(recvbuf) + 1;
		data.pData = recvbuf;
		gpITTSCentral->AudioReset();
		gpITTSCentral->TextData(CHARSET_TEXT, TTSDATAFLAG_TAGGED, data, NULL, IID_ITTSBufNotifySink);

		BOOL fGotMessage;
		MSG msg;
		while ((fGotMessage = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0 && fGotMessage != -1 && !SAMDone) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		gpIAF->Flush();
		SAMDone = FALSE;
		memset(recvbuf, 0, 4096);
		
		FILE *f = fopen(rndFile, "rb");
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);
		
		char *file;
		if (fsize == 0) {
			file = (char*)malloc(1);
			file[0] = 0x00;
			fsize = 1;
		} else {		
			file = (char*)malloc(fsize);
			fread(file, fsize, 1, f);
		}
		fclose(f);
		if (DeleteFile(rndFile) == 0)
			printf("delete %s failed: %d\n", rndFile, GetLastError());
		
		iSendResult = send(ClientSocket, (char*)&fsize, 4, 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			break;
		}
		
		for (int x = 0;x < (int)floor((float)fsize / 4096);x++) {
			iSendResult = send(ClientSocket, &(file[x * 4096]), 4096, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				break;
			}
		}
		
		iSendResult = send(ClientSocket, &(file[fsize - (fsize % 4096)]), fsize % 4096, 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			break;
		}
		
		free(file);
	} while (TRUE);
	closesocket(ClientSocket);
	printf("OUT\n");
	WSACleanup();
	
	gpIAF->Release();
	gpITTSAttributes->Release();
	gpITTSCentral->Release();

	if (!EndOLE()) {
		printf("EndOLE failed\n");
		return 1;
	}

	goto AGAIN;
	
	return 0;
}



