# vgmM5 consumer integration (planned, code remains upstream)

Origin: https://github.com/Layer812/vgmM5
Observed source commit (2026-10-31): `0f311eb3a2a2fa690a9665cb2f8025c61c622970`.

The platform does **not** implement or vendor YM2151, SN76489, OPLL, ymfm, VGM/VGZ parsers, MDX, audio synthesis or player UI. For an application using these functions, retrieve the exact source from vgmM5, preserve the original MIT and per-component notices, and connect generated PCM to `t5p_audio_submit()`. Clocking, timer/IRQ and synthesis remain on the vgmM5 side. Do not mix different emulator branches by renaming copies into the platform.

No integration build has yet been performed in this release candidate.
