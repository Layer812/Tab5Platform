# Tab5 Platform API policy (R2)

### Package identity

Public app identity should be `Tab5 Platform`, not `ArcadeTab5` or `X68KTab5`. The C API prefix is `t5p_` and the ESP-IDF component path is `components/tab5_platform`. `sample/mame/` is an independently removable example, not a platform dependency.

### Stability

R2 is an **experimental source API**: names, backend geometry and task contracts can still change until native build and real hardware passes. The existing R6 X68KTab5 guest/display/audio timings are not declared compatible with generic R2 implementations until a specific app-side adapter has been validated.

### External code boundaries

- Platform C API owns device control, PCM transport, input events, display frame buffers and storage access only.
- A vgmM5 consumer uses vgmM5's real FM/VGM/MDX implementation from its own repository, never a copied or rewritten platform engine.
- A MidMod consumer uses MidMod's real MIDI engine/mapping from its own repository, respecting GPL-3.0 in the combined app.
- An arcade consumer links its own legally acquired MAME subset driver; the platform exposes no copyrighted ROM data or MAME-internal symbols.

### Porting X68KTab5 later

Require `tab5_platform` from an X68KTab5 application-side adapter, translate X68K events to generic output/input types, and avoid migrating its current Screen Manager wholesale until display performance can match R6. Production R6 stays untouched until such an integration is actually verified.
