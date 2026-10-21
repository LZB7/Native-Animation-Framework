# NAF Extended dynamic expressions (Phase 3C)

Phase 3C adds reusable actor-level expression presets without requiring a face
timeline tied to the current body animation. It reuses NAF's managed FaceGen
hook, actor handles, Set3D rebinding, request generations, and task dispatch.
It is not a second facial-animation engine.

## Presets

Preset files live anywhere below `Data/NAF` and end in `.nafexpr.json`:

```json
{
  "format": "NAFExtendedExpressionPreset",
  "schemaVersion": 1,
  "id": "example.smile",
  "displayName": "Example Smile",
  "channels": [
    { "morph": 17, "value": 0.55 },
    { "morph": 40, "value": 0.55 }
  ]
}
```

Morph indices are the same validated 0-53 Face Creator expression channels.
Values must be finite and in `[0,1]`. Duplicate channel indices, duplicate
preset IDs, unknown schemas, empty channel lists, and malformed files are
rejected; the first lexically loaded duplicate ID wins. Presets are parsed once
during NAF data initialization and cached in memory. This format is deliberately
separate from `.naf.json` animation packs.

The checked-in `tests/fixtures/expressions` files are development fixtures, not
mandatory framework content. Their conservative smile/frown channel identities
come directly from Face Creator (`17/40` and `5/28`) and still require visual
acceptance in game.

## Runtime semantics

- A request is actor-level and independent of scenes or body animation format.
- Intensity `0.0` contributes zero on the preset's channels; `1.0` applies the
  authored values. Non-finite or out-of-range values are rejected.
- Replacement is immediate on NAF's safe F4SE task path. Smooth transitions are
  deferred; v2 `options` must be zero.
- A dynamic expression and an authored face animation do not compose. The most
  recent valid request wins. Authored playback reports the dynamic expression as
  canceled/replaced through the v2 query flag.
- Dynamic expressions run after the vanilla face update and alter only their
  declared expression channels. Unlike authored NAF face timelines, they do not
  suppress vanilla lip updates. Eye coordinates are not part of the v1 preset
  schema, preserving blink/eye ownership.
- `ReleaseExpression` removes only the dynamic contribution. It does not clear
  engine-owned base/modifier state or an unrelated eye override.
- `ResetFaceExpression` remains the stronger operation: it invalidates pending
  work, releases all NAF face ownership, and neutralizes composed output.
- Dynamic requests are transient. Save/load, new game, or serialization revert
  clears them. No serialization record was added and `FRST` remains retired.
- Set3D rebinding preserves an active transient request while its actor remains
  valid; no FaceGen pointer crosses the public ABI.

## Native API

Existing `NAFExtFaceAPIv1` and `NAFExt_GetFaceAPI` are unchanged. Consumers opt
into the additive `NAFExtFaceAPIv2` through `NAFExt_GetFaceAPIv2` and validate
version, table size, capabilities, and function pointers.

New capabilities are `NAFEXT_CAP_EXPRESSION_PRESETS`,
`NAFEXT_CAP_EXPRESSION_INTENSITY`, `NAFEXT_CAP_EXPRESSION_RELEASE`, and
`NAFEXT_CAP_EXPRESSION_QUERY`. The new operations are:

```c
request_expression(actor_form_id, preset_id, intensity, options)
release_expression(actor_form_id, options)
get_expression_state(actor_form_id, state, preset_buffer, capacity, required)
```

Actor IDs are current-session load-order-resolved FormIDs. `NAFEXT_OK` means the
request was accepted, not yet visually applied. State queries expose status,
generation, intensity, preset ID, FaceGen availability, and whether authored
playback replaced the expression. Strings remain caller-owned/caller-copied and
no CommonLib, STL, engine pointer, or cross-CRT ownership crosses the ABI.

## Known limitations

The initial implementation intentionally has immediate replacement/release and
no timed duration or cross-source blending. Presets address expression morphs
only; eye-coordinate animation remains the existing managed-face facility.
Actual appearance and intensity response now have initial production coverage;
uncommon lip/eye combinations and full-exit save/load cleanup remain on the
runtime coverage list.

## Runtime coverage

The October 21 harness run loaded three presets and exercised them visibly both
outside NAF scenes and during the JSON-only `J1` HKX scene. Requests returned
`NAFEXT_OK`; queries reported `Active` for `dev.smile` at both `1.00` and `0.35`,
and release reported `Completed` with no preset ID. Scene playback, snapshots,
and cleanup remained healthy. The familiar MFG-like results are expected because
NAF Face Creator and MFG expose the same engine FaceGen morph channels; NAF does
not call or depend on MFG. Save followed by full exit/reload was not demonstrated
by that log and remains pending before the transient lifecycle is runtime-closed.
