# 0.22.16 — FMOD wrapper compatibility and 30-degree free look

The previous hero-voice change queried Channel::getPan. PUP exports that symbol
but implements it as an unsupported-feature warning dialog. An executable export
and the prior mock API checks were insufficient to detect that incompatibility.

- Remove the pan getter from the voice adapter and compiled plugin entirely.
  The remaining getMode / setMode / setPan exports forward directly to FMOD Core
  in the installed wrapper. No wrapper preference or warning suppression is
  written, and no warning function is patched.
- Center only originally-3D hero speech and restore its complete original mode
  flags on release. Existing 2D channels remain untouched, including their pan.
  Returning to 3D restores spatialization; the obsolete 2D pan is not queried.
  Failure while centering rolls back the native mode. NPC audio is unchanged.
- Increase the physical head/body free-look cone from 12 to 30 degrees, as
  requested. Both native facing and stabilized body rendering use the same
  constant; diagnostic output derives the value from that constant.
- Keep the existing native movement/knockout/camera fixes and accepted hands,
  weapon tracking, settings and GPU pipeline.

Validation: all 83 CTests, including a voice adapter that has no GetPan method,
  originally-2D preservation, API failure rollback, reused/ended-channel lifetime,
  native hook ABI checks, and small head turns through +/-29.9 degrees.
An additional offline PE audit verifies the three actual wrapper forwards and
backend exports and confirms that the compiled DLL contains no getPan resolver.
Headset playback remains a live-test requirement.
