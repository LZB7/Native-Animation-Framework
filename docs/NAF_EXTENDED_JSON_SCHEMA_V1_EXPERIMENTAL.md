# NAF Extended experimental JSON animation schema v1

Files are discovered recursively below `Data/NAF` when their filename ends in
`.naf.json`. Parsing happens during normal data initialization and configuration
reload; JSON is never reparsed during scene updates.

This schema is experimental. Version 1 intentionally proves only ordinary HKX
registration through NAF's existing `Animation` and `Position` registries.

```json
{
  "format": "NAFExtendedAnimation",
  "schemaVersion": 1,
  "loadPriority": 0,
  "animation": {
    "id": "author.package.animation-id",
    "actors": [
      {
        "file": "author\\path\\clip.hkx",
        "requirements": { "skeleton": "Human", "legacyGameSex": "Any" },
        "expectedBodyType": "Masc",
        "role": "giver",
        "erection": true
      }
    ]
  },
  "position": {
    "id": "author.package.position-id",
    "displayName": "Human-readable position name",
    "hidden": false,
    "tags": ["tag-one", "tag-two"]
  },
  "extensions": {
    "author.namespace": { "arbitrary-key": "arbitrary valid JSON" }
  }
}
```

Actor-array order is slot order. `requirements.skeleton` defaults to `Human`.
`requirements.legacyGameSex` is optional and accepts `Any`, `Male`, or
`Female`; omission is unrestricted (`Any`). For compatibility with the first
experimental files, top-level actor `skeleton` and `gender` remain accepted.
This field represents Fallout's game-record sex restriction only. It does not
express gender identity, presentation, anatomy, sexuality, or relationship
roles. Future versions can add capability requirements without redefining this
legacy constraint.

`expectedBodyType` is optional and accepts `Masc`, `Femme`, or `Any`; omission
means `Any`. It records the body presentation anticipated by the animator and
does not constrain Fallout's Male/Female game-record sex. The deprecated JSON
`gender` spelling retains its earlier hard game-sex meaning. When both fields
are present they remain independent: `gender` controls legacy eligibility and
`expectedBodyType` remains descriptive metadata.

`role` is optional and accepts `giver`, `receiver`, or `solo`. It describes the
authored action, not sex, identity, anatomy, or eligibility. `erection` is an optional
Boolean instruction. Its three states are significant: `true` is an explicit
erect request, `false` is an explicit non-erect request, and omission supplies
no instruction. NAF publishes these values but does not apply morphs, equip
items, infer anatomy, or call an external gameplay plugin.

Each `file` is the same mesh-relative HKX path accepted by legacy NAF animation
XML. `position.id` defaults to `animation.id` when omitted.

IDs are non-empty, case-insensitive machine identifiers. No `NAFExtJSON_`
prefix is required; reverse-domain or `author.package.name` namespacing is
recommended only to avoid collisions. `position.displayName` is optional,
accepts ordinary UTF-8 display text, and falls back to the position ID. It is
used by the normal scene selector and exposed as `naf.extended` metadata without
changing the underlying identity. Multiple distinct animation IDs may reference
the same HKX path; an HKX resource path is not a definition key.

Legacy XML loads first. JSON files then load in lexical path order. A definition
whose animation or position ID already exists is rejected as a unit, so existing
XML wins and the first JSON definition wins among JSON conflicts. Malformed
documents, unsupported schema versions, missing slots, invalid genders, and
missing path strings are rejected with one actionable warning. Resource opening
remains the responsibility of the normal dynamic-idle playback path, which will
report a missing HKX when selected.

Unknown `extensions` are optional and do not affect registration. NAF preserves
each namespace/key value as compact UTF-8 JSON in a parallel registry and exposes
it through API snapshot metadata. No JSON-library type crosses the ABI.

Actor metadata uses namespace `naf.extended.actor` with slot-associated keys:

- `slot.N.requirements.skeleton`
- `slot.N.expectedBodyType` (always present, including the default `"Any"`)
- `slot.N.role` (present only when authored)
- `slot.N.erection` (present only when authored; JSON `false` remains distinct
  from an absent key)

These records use the existing ABI-v1 metadata array, so no ABI layout or
version change is required.

Deferred from this provisional version: staged groups, trees, conditions,
equipment, morph directives, actions, face timelines, and a finalized Passion
semantic vocabulary. Legacy XML and JSON both feed the same runtime; this schema
does not introduce a second playback engine.

## Configuration cache interaction

JSON uses the separate `Data/NAF/NAFPackCache.bin`. Cache format version 2 contains
validated normalized definitions in CBOR plus a deterministic manifest of
relative path, byte size, modification time, and a 64-bit source fingerprint.
The complete manifest must match, so additions, removals, same-size edits, and
metadata-only changes rebuild the cache. A bounded header, payload length,
version/schema checks, and payload checksum reject truncated or corrupt data.
Writes use a temporary file followed by atomic replacement; write failure does
not discard definitions already parsed from source.

`_XMLCache.bin` remains independent and unchanged. It stores complete XML source
text and Face Animation cache metadata. XML still loads first; cached JSON is
re-registered afterward, so XML-first precedence and normal collision rules are
identical on cold and warm loads. Configuration reload clears runtime JSON
registries and repeats the same manifest validation and registration process.
