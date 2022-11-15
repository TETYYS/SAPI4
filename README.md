# SAPI4

Web interface for Microsoft Sam &amp; friends written in C & D (vibe-d), runnable on headless linux.

Demo: https://tetyys.com/SAPI4

## Setup

### SAPI4 server compilation on local Windows machine

1. Install Microsoft Speech SDK 4.0 (`SAPI4SDK.exe`)
1. Run `build.bat`. If you have Visual Studio older than 2022, change vcvars32 path
1. Install [`ldc2`](https://github.com/ldc-developers/ldc/releases) and [`dub`](https://github.com/dlang/dub/releases)
1. Go to SAPI4_web and compile the web server: `dub --compiler=ldc2 --arch=x86 --build=release`

### Web server compilation & SAPI4 setup on remote Linux machine

1. Install `wine` (`sudo apt install wine`), 1.8.7 is fine. If wine doesn't work on your system, you must stop here
1. In a VNC/RDP session or X11-forwarded SSH connection:
	* Install Microsoft Speech 4.0 API in the wine environment: `wine spchapi.exe`
	* Install Lernout & Hauspie TruVoice Amer. Eng. TTS Engine in the wine environment: `wine tv_enua.exe`
1. Move:	
	* `public` (static web assets)
	* `sapi4.exe` (web server)
	* `sapi4.dll` (SAPI4 voice audio generation library)
	* `sapi4limits.exe` (SAPI4 voice enumerator)
	* `sapi4out.exe` (SAPI4 voice audio generation program)
to a new empty folder
1. Install `xvfb` (`apt install xvfb`)
1. Run web server: `while true; do; xvfb-run -a wine sapi4.exe; sleep 1; done;`
1. Pass the web server through nginx - add this to nginx config: `location ^~ /SAPI4/ { proxy_pass http://127.0.0.1:23451/; }`. Note that the web server will work only on `/SAPI4/` location, if you want to change that, change references to scripts and other assets in `SAPI4_web/views/layout.dt`, `SAPI4_web/public/scripts/tts.js`.
1. Go to `http(s)://localhost/SAPI4/`, put `soi soi soi soi soi soi soi soi soi soi soi soi soi soi soi soi soi soi soi soi soi` as text, set speed to 450 and enjoy.

You might be familiar with Speakonia. As CFS-Technologies have released an unlimited license (http://www.cfs-technologies.com/home/) for Speakonia, you can get .wavs Microsoft Sam & other voices genereated text with Speakonia too, however web interface is more convenient and generates text much faster. Speakonia is set to generate text at real-time of speaking speed and SAPI4 server is set to generate text at x16777215 of real-time speaking speed. You can download .wavs from web interface too (right click the player and press `Save audio as...`, at least on Chrome).

You can generate text from an API too, endpoints are `/SAPI4/VoiceLimitations?voice=(voice)` and `/SAPI4/SAPI4?text=(text)[&voice=(voice)][&pitch=(pitch)][&speed=(speed)]`. `()` - required parameters, `[]` - optional parameters.
