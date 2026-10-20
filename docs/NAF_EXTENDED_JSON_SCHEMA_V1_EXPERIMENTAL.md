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
    "id": "UniqueAnimationID",
    "actors": [
      { "skeleton": "Human", "gender": "Any", "file": "author\\path\\clip.hkx" }
    ]
  },
  "position": {
    "id": "UniquePositionID",
    "hidden": false,
    "tags": ["tag-one", "tag-two"]
  },
  "extensions": {
    "author.namespace": { "arbitrary-key": "arbitrary valid JSON" }
  }
}
```

`gender` accepts `Any`, `Male`, or `Female`. Actor-array order is slot order.
Each `file` is the same mesh-relative HKX path accepted by legacy NAF animation
XML. `position.id` defaults to `animation.id` when omitted.

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

Deferred from this provisional version: staged groups, trees, conditions,
equipment, morph directives, actions, face timelines, and a finalized Passion
semantic vocabulary. Legacy XML and JSON both feed the same runtime; this schema
does not introduce a second playback engine.
