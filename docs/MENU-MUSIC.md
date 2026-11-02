# Custom main-menu music

The supplied `VRsta Point.mp3` is converted to Ogg Vorbis, stereo, 48 kHz,
quality 7. This matches the codec, channel count and sample rate of the native
title music. The source MP3 is retained unchanged. No cover artwork or video
stream is included in the converted resource.

The payload contains a single loose override:

`Data/Music/fragments/myrtana_title_day_01.ogg`

Copy that resource below the game root with its directory structure intact.
The existing `Data/Music=Data/Music` mount loads it in place of the same entry
in `Music.pak`. This adds no playback hooks, extra player process or runtime
conversion. The native music volume, menu transitions and playback system
remain in charge. Restart the game to reload an already cached title song.

The local native `Library.pak/g3.mul` maps the `title` situation to
`myrtana_title_day_01.wav`; the shipped compressed resource has the `.ogg`
extension above. No library or archive needs repacking. The verified native
`gCMusicStream::SetDescriptor` calls `UpdateDuration`, which reads the loaded
sound's actual length and resets its end point, rather than requiring the
replacement to be padded to the old song's duration.

Preparation is reproducible with `tools/PrepareMenuMusic.py --input <mp3>
--payload <staging-folder> --ffmpeg <ffmpeg.exe>`. The script selects the audio
stream explicitly, validates the Ogg container, decodes the entire result and
writes a SHA-256 manifest. Keep the manifest outside the game directory.

To restore the original title music, remove this exact loose override from
the game directory. Do not modify or remove `Music.pak` or `Music.p00`.

The audited Union+ cleanup removes `.m*`/`.n*` archives and files tracked in its
own installed-mod list. This standalone loose `.ogg` is not registered in that
list; it does not require changes to the Union+ protection executable.

Native evidence: supported Game.dll SHA-256
`4728A47B244ADCB0EA2EC3501DB04994909DCFCFF338C7B8075F2B68DD8D6E00`;
`gCMusicStream::SetDescriptor` RVA `0xF6270`, `UpdateDuration` RVA `0xF62E0`.
In-game acceptance still requires hearing the replacement in the main menu
after a fresh launch and confirming the ordinary world music on loading a save.
