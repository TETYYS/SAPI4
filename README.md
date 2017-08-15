# SAPI4
Web interface for Microsoft Sam &amp; friends written in C & D (vibe-d), runnable on headless linux.

Demo: https://tetyys.com/SAPI4

## Setup
### SAPI4 server compilation on local Windows machine

1. Install Microsoft Speech SDK 4.0 (SAPI4SDK.exe).
1. Run `vcvars32.bat` of your oldest installation of Visual Studio, SAPI4 is a wonder of '90s, but it should compile with Visual Studio 2017 too.
1. Compile: `cl sapi4.cpp ole32.lib user32.lib Ws2_32.lib /MT -I"C:\Program Files (x86)\Microsoft Speech SDK\Include"`.

### Web server compilation & SAPI4 server setup on remote Linux machine

1. Clone SAPI4_web & move `sapi4.exe` (SAPI4 server), `spchapi.exe` (Microsoft Speech 4.0 API) and `tv_enua.exe` (Lernout & Hauspie TruVoice Amer. Eng. TTS Engine) to the linux server. The voice of Bonzi Buddy lives in `tv_enua.exe`.
1. Install wine (`sudo apt-get install wine`), 1.8.7 is fine. If wine doesn't work on your system, you must stop here.
1. Install Microsoft Speech 4.0 API in the wine environment: `wine spchapi.exe`.
1. Install Lernout & Hauspie TruVoice Amer. Eng. TTS Engine in the wine environment: `wine tv_enua.exe`.
1. Run SAPI4 server(s) first: `wine sapi4.exe [Local listen port],[Voice name]`, for example `wine demo.exe 23453,Adult Male \#2, American English \(TruVoice\)` or `wine demo.exe 23452,Sam`.
1. Modify `SAPI4_web/views/index.dt` lines 41-42 to match your target voices (sorry, it can be done automatically).
1. Install `dub` and `dmd` from http://d-apt.sourceforge.net/ .
1. Go to SAPI4_web and compile the web server: `dub build --build=release`.
1. Run web server with currently running SAPI4 server(s): `./sapi4 --ports=[port1,[name1]:[port2],[name2]:...`, for example `./sapi4 --ports=60002,Sam:60003,Bonzi`.
1. Pass the web server through nginx - add this to nginx config: `location ^~ /SAPI4/ { proxy_pass http://127.0.0.1:23451/; }`. Note that the web server will work only on `/SAPI4/` location, if you want to change that, change references to scripts and other assets in `SAPI4_web/views/layout.dt`, `SAPI4_web/public/scripts/tts.js`.
1. Go to `http(s)://localhost/SAPI4/` and enjoy.

You might be familiar with Speakonia. As CFS-Technologies have released an unlimited license (http://www.cfs-technologies.com/home/) for Speakonia, you can get .wavs Microsoft Sam & other voice genereated text with Speakonia too, however web interface is more convenient and generates text much faster. Speakonia is set to generate text at real-time of speaking speed and SAPI4 server is set to generate text at x16777215 of real-time speaking speed. You can download .wavs from web interface too (right click the player and press `Save audio as...`, at least on Chrome).

You can generate text from an API too, endpoints are `/VoiceLimitations?voice=(voice)` and `/SAPI4?text=(text)[&voice=(voice)][&pitch=(pitch)][&speed=(speed)]`. `()` - required parameters, `[]` - optional parameters.

If the Microsoft Sam is very busy, you can scale SAPI4 by running it multiple times with different ports but same voices. Run the single instance of web server with different ports but same names too.

It's possible to run a combination of SAPI4 server on Windows Server and web server on Linux server, without need of `wine`. You will need to modify `sapi4.cpp` to listen on `0.0.0.0` and firewall it to accept connections only from linux server.
