#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <excpt.h>

#include "sapi4.hpp"

int main(int argc, char** argv)
{
	VOICE_INFO VoiceInfo;
	if (!InitializeForVoice(argv[1], &VoiceInfo)) {
		return 0;
	}

	UINT64 Len;
	GetTTS(&VoiceInfo, atoi(argv[2]), atoi(argv[3]), argv[4], &Len);
}