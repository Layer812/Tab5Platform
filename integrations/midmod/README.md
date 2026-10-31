# MidMod consumer integration (planned, code remains upstream)

Origin: https://github.com/Layer812/MidMod
Observed source commit (2026-10-31): `72d6f2f7511eefb47dcd12be62149cd6f451a29e`.

The platform does **not** copy MidMod's `MidiEngine`, GS/SAM2695 mappings, MIDI parser or UART/audio application logic. Applications requiring MIDI processing should reference the upstream implementation through an app-specific bridge/provider. An upstream `MidiEngine` integration can use platform storage/input/timer; MIDI transformations and mapping policy stay with MidMod itself.

MidMod is **GPL-3.0**, so it must not be silently linked into a permissively licensed platform firmware without checking the full combined-work license conditions. No MidMod code is included or compiled by the platform reference app.
