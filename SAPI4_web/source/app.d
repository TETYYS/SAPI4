import vibe.vibe;
import std.algorithm;
import std.conv;
import std.encoding;
import std.bitmanip;
import std.array;

class SAM {
	TCPConnection conn;
	bool busy;
	string voice;
	ushort defPitch, minPitch, maxPitch;
	uint defSpeed, minSpeed, maxSpeed;
	Task pingTask;

	this(TCPConnection conn, string voice, ushort defPitch, ushort minPitch, ushort maxPitch, uint defSpeed, uint minSpeed, uint maxSpeed) {
		this.conn = conn;
		this.voice = voice;
		this.defPitch = defPitch;
		this.minPitch = minPitch;
		this.maxPitch = maxPitch;
		this.defSpeed = defSpeed;
		this.minSpeed = minSpeed;
		this.maxSpeed = maxSpeed;
		this.pingTask = runTask({
			for (;;) {
				conn.write([ cast(ubyte)0, cast(ubyte)0 ]);
				ubyte[2] p;
				conn.read(p);
				sleep(60.seconds);
			}
		});
	}
}

SAM[] SAMPool;
string[] existingVoices;

void main(string[] args)
{
	auto router = new URLRouter;
	router.registerWebInterface(new SAMService);
	router.get("/*", serveStaticFiles("public/"));

	auto settings = new HTTPServerSettings;
	settings.port = 23451;
	settings.bindAddresses = [ "127.0.0.1" ];
	listenHTTP(settings, router);

	// 55486
	string ports;
	readOption("ports", &ports, "SAM PORTS");
	foreach (param; ports.split(':')) {
		auto p = param.split(',');

		if (p.length > 2)
			p[1] = join(p[1 .. $]);

		auto conn = connectTCP("127.0.0.1", to!ushort(p[0]));

		ubyte[] buff;
		buff.length = 18;
		conn.read(buff);
		logInfo("%s", to!string(buff[0 .. $]));
		const defPitch = buff.read!(ushort, Endian.littleEndian)();
		const minPitch = buff.read!(ushort, Endian.littleEndian)();
		const maxPitch = buff.read!(ushort, Endian.littleEndian)();
		const defSpeed = buff.read!(uint, Endian.littleEndian)();
		const minSpeed = buff.read!(uint, Endian.littleEndian)();
		const maxSpeed = buff.read!(uint, Endian.littleEndian)();

		SAMPool ~= new SAM(conn, p[1], defPitch, minPitch, maxPitch, defSpeed, minSpeed, maxSpeed);
		if (!existingVoices.canFind(p[1]))
			existingVoices ~= p[1];
	}

	runApplication();
}

class SAMService
{
	@path("/")
	void getIndex(HTTPServerRequest req, HTTPServerResponse res)
	{
		render!("index.dt");
	}

	@path("/VoiceLimitations")
	void getVoiceLimitations(HTTPServerRequest req, HTTPServerResponse res)
	{
		string voice;
		if ((voice = req.query.get("voice", "")) == "" || !existingVoices.canFind(voice)) {
			res.writeBody("Invalid voice", HTTPStatus.badRequest);
			return;
		}

		foreach (s; SAMPool) {
			if (s.voice == voice) {
				res.writeJsonBody([
					"defPitch": s.defPitch,
					"minPitch": s.minPitch,
					"maxPitch": s.maxPitch,
					"defSpeed": s.defSpeed,
					"minSpeed": s.minSpeed,
					"maxSpeed": s.maxSpeed
				]);
				return;
			}
		}
	}

	@path("/SAPI4")
	void getSAPI4(HTTPServerRequest req, HTTPServerResponse res)
	{
		string textRaw = req.query.get("text", "");
		AsciiString text;
		transcode(textRaw, text);

		if (text == "" || text.length > 4095) {
			res.writeBody("Microsoft Sam is simply confused.");
			return;
		}

		int pitch;
		long speed;
		try {
			pitch = to!int(req.query.get("pitch", "-1"));
			speed = to!long(req.query.get("speed", "-1"));
		} catch (Exception) {
			res.writeBody("Microsoft Sam is very confused.");
			return;
		}

		string voice = req.query.get("voice", "Sam");
		if (!existingVoices.canFind(voice)) {
			res.writeBody("Microsoft Sam is somewhat confused.");
			return;
		}

		SAM sam;
		bool foundSam = false;
		synchronized {
			foreach (s; SAMPool) {
				if (!s.busy && s.voice == voice) {
					s.busy = true;
					foundSam = true;
					sam = s;
					break;
				}
			}
		}

		scope (exit) {
			sam.busy = false;
		}

		if (!foundSam) {
			res.writeBody("Microsoft Sam is busy.");
			return;
		}

		if (pitch == -1)
			pitch = sam.defPitch;
		if (speed == -1)
			speed = sam.defSpeed;

		if (pitch > sam.maxPitch || pitch < sam.minPitch) {
			res.writeBody("Microsoft Sam writes: pitch: [" ~ to!string(sam.minPitch) ~ "; " ~ to!string(sam.maxPitch) ~ "]");
			return;
		}

		if (speed > sam.maxSpeed || speed < sam.minSpeed) {
			res.writeBody("Microsoft Sam writes: speed: [" ~ to!string(sam.minPitch) ~ "; " ~ to!string(sam.maxPitch) ~ "]");
			return;
		}

		logInfo("req: %d, %d, %s, %s", pitch, speed, voice, cast(string)text);

		ubyte[] buff;
		buff.length = 8;
		buff.write!(ushort, Endian.littleEndian)(cast(ushort)text.length, 0);
		buff.write!(ushort, Endian.littleEndian)(cast(ushort)pitch, 2);
		buff.write!(uint, Endian.littleEndian)(cast(uint)speed, 4);
		buff ~= cast(ubyte[])text;
		sam.conn.write(buff);

		buff.length = 0;
		buff.length = 4;
		sam.conn.read(buff);
		auto len = buff.read!(uint, Endian.littleEndian)();

		logInfo("BUF >%d", len);

		buff.length = 0;
		buff.length = len;

		res.contentType = "audio/wav";
		pipe(sam.conn, res.bodyWriter, len);
	}
}