var SAPI4 =
{
	SayTTS: async function(voice, text, pitch, speed)
	{
		pitch = parseInt(pitch);
		speed = parseInt(speed);

		let wav = await fetch("/SAPI4/SAPI4?text=" + encodeURIComponent(text) + "&voice=" + encodeURIComponent(voice) + "&pitch=" + pitch + "&speed=" + speed);
		let blobUrl = URL.createObjectURL(await wav.blob());
		document.getElementById("source").setAttribute("src", blobUrl);
		let audio = document.getElementById("audio");
		audio.pause();
		audio.load();
		audio.play();
	},

	VoiceLimitations: async function(voice)
	{
		let wav = await fetch("/SAPI4/VoiceLimitations?voice=" + voice);
		return wav.json();
	}
}