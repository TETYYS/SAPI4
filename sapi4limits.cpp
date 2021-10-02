#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <excpt.h>

#include "sapi4.hpp"

int main(int argc, char** argv)
{
	if (argc == 2) {
		VOICE_INFO VoiceInfo;
		if (!InitializeForVoice(argv[1], &VoiceInfo)) {
			return 0;
		}

		printf("%s\n%d %d %d\n%d %d %d\n",
			VoiceInfo.ModeInfo.szModeName,
			VoiceInfo.defPitch, VoiceInfo.minPitch, VoiceInfo.maxPitch,
			VoiceInfo.defSpeed, VoiceInfo.minSpeed, VoiceInfo.maxSpeed);
	} else {
		printf("%s", EnumerateVoices());
	}
}