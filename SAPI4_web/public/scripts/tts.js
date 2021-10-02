var SAPI4 =
{
	SayTTS: async function(voice, text, pitch, speed)
	{
		pitch = parseInt(pitch);
		speed = parseInt(speed);

		let url = "/SAPI4/SAPI4?text=" + encodeURIComponent(text) + "&voice=" + encodeURIComponent(voice) + "&pitch=" + pitch + "&speed=" + speed;

		if (url.length > 4088) {
			alert("Text too long");
			return;
		}

		let wav = await fetch(url);
		if (wav.status != 200) {
			alert(await wav.text());
			return;
		}
		let blobUrl = URL.createObjectURL(await wav.blob());
		document.getElementById("source").setAttribute("src", blobUrl);
		let audio = document.getElementById("audio");
		audio.pause();
		audio.load();
		audio.play();
	},

	VoiceLimitations: async function(voice)
	{
		let wav = await fetch("/SAPI4/VoiceLimitations?voice=" + encodeURIComponent(voice));
		return wav.json();
	}
}