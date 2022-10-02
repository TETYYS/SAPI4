import vibe.vibe;
import std.algorithm;
import std.conv;
import std.encoding;
import std.string;
import std.array;
import std.process;
import core.stdc.stdlib;
import std.datetime;
import std.parallelism;
import std.file;

struct SAM {
	string voice;
	ushort defPitch, minPitch, maxPitch;
	uint defSpeed, minSpeed, maxSpeed;

	this(string voice)
	{
		auto proc = execute(["sapi4limits.exe", voice]);
		if (proc.status != 0) {
			logError("sapi4limits fail on %s", voice);
			exit(1);
		}
		auto spl = proc.output.replace("err:xrandr:xrandr12_init_modes Failed to get primary CRTC info.", "").strip.split("\r\n");
		logInfo("voice: %s", spl[0 .. $]);
		voice = spl[0];
		auto pitches = spl[1].split(' ');
		defPitch = to!ushort(pitches[0]);
		minPitch = to!ushort(pitches[1]);
		maxPitch = to!ushort(pitches[2]);
		auto speeds = spl[2].split(' ');
		defSpeed = to!ushort(speeds[0]);
		minSpeed = to!ushort(speeds[1]);
		maxSpeed = to!ushort(speeds[2]);
	}
}

SAM[string] SAMS;

void main(string[] args)
{
	auto router = new URLRouter;
	router.registerWebInterface(new SAMService);
	router.get("/*", serveStaticFiles("public/"));

	auto proc = execute("sapi4limits.exe");
	if (proc.status != 0) {
		logError("sapi4limits fail");
		exit(1);
	}
	auto voices = proc.output.replace("err:xrandr:xrandr12_init_modes Failed to get primary CRTC info.", "").strip.split("\r\n");
	foreach (voice; voices) {
		SAMS[voice] = SAM(voice);
	}

	auto settings = new HTTPServerSettings;
	settings.port = 23451;
	settings.bindAddresses = [ "127.0.0.1" ];
	listenHTTP(settings, router);

	if (false) {
		// LEAVE IN THIS BLOCK!!!! OPTLINK SUCKS
		string voice;
		readOption("voices", &voice, "Voices");
	}

	runApplication();
}

class SAMService
{
	@path("/")
	void getIndex(HTTPServerRequest req, HTTPServerResponse res)
	{
		auto voices = SAMS.keys.sort;
		render!("index.dt", voices);
	}

	@path("/VoiceLimitations")
	void getVoiceLimitations(HTTPServerRequest req, HTTPServerResponse res)
	{
		string voice;
		if ((voice = req.query.get("voice", "")) == "" || !SAMS.keys.canFind(voice)) {
			res.writeBody("Invalid voice", HTTPStatus.badRequest);
			return;
		}

		auto s = SAMS[voice];
		res.writeJsonBody([
			"defPitch": s.defPitch,
			"minPitch": s.minPitch,
			"maxPitch": s.maxPitch,
			"defSpeed": s.defSpeed,
			"minSpeed": s.minSpeed,
			"maxSpeed": s.maxSpeed
		]);
	}

	@path("/SAPI4")
	void getSAPI4(HTTPServerRequest req, HTTPServerResponse res)
	{
		try {
			string textRaw = req.query.get("text", "");
			AsciiString text;
			transcode(textRaw, text);

			if (text == "" || text.length > 4095) {
				res.writeBody("Invalid text", 400);
				return;
			}

			int pitch;
			long speed;
			try {
				pitch = to!int(req.query.get("pitch", "-1"));
				speed = to!long(req.query.get("speed", "-1"));
			} catch (Exception) {
				res.writeBody("Invalid pitch/speed", 400);
				return;
			}

			string voice = req.query.get("voice", "INVALID");
			if (!SAMS.keys.canFind(voice)) {
				res.writeBody("Invalid voice", 400);
				return;
			}
			auto sam = SAMS[voice];

			if (pitch == -1)
				pitch = sam.defPitch;
			if (speed == -1)
				speed = sam.defSpeed;

			if (pitch > sam.maxPitch || pitch < sam.minPitch) {
				res.writeBody("Available pitch: [" ~ to!string(sam.minPitch) ~ "; " ~ to!string(sam.maxPitch) ~ "], got " ~ to!string(pitch), 400);
				return;
			}

			if (speed > sam.maxSpeed || speed < sam.minSpeed) {
				res.writeBody("Available speed: [" ~ to!string(sam.minPitch) ~ "; " ~ to!string(sam.maxPitch) ~ "], got " ~ to!string(speed), 400);
				return;
			}

			auto proc = pipeProcess(["sapi4out.exe", voice, to!string(pitch), to!string(speed), cast(string)text], Redirect.all);

			auto executedAt = Clock.currTime;
			auto wait = tryWait(proc.pid);

			while (!wait.terminated && Clock.currTime - executedAt < dur!"seconds"(10)) {
				wait = tryWait(proc.pid);
				sleep(dur!"msecs"(100));
			}

			if (!wait.terminated) {
				kill(proc.pid);
				res.writeBody("Please reformat your text", 400);
				return;
			} else if (wait.status != 0) {
				res.writeBody("Please reformat your text", 400);
				return;
			}
			
			string outputErr;
			foreach (line; proc.stderr.byLine)
				outputErr ~= line.idup;

			string output;
			foreach (line; proc.stdout.byLine)
				output ~= line.idup;
			
			if (outputErr != "")
				logInfo("got error output \"" ~ outputErr ~ "\"");

			auto file = output.replace("err:xrandr:xrandr12_init_modes Failed to get primary CRTC info.", "").strip();

			if (file == "") {
				core.stdc.stdlib.exit(1);
			}

			auto fs = openFile(file, FileMode.read);

			res.contentType = "audio/wav";
			pipe(fs, res.bodyWriter);
			fs.close();
			removeFile(file);
		} catch (Exception ex) {
			logInfo("exception " ~ ex.toString());
		}
	}
}