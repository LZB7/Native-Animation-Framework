# NAF Extended JSON animation-pack schema v2

Files end in `.naf.json` and may appear anywhere below `Data/NAF`. Schema v2
represents one logical package containing multiple independently registered NAF
definitions. It normalizes into the same `Animation`, `Position`,
`AnimationGroup`, and `PositionTree` registries used by XML; it is not a second
playback engine.

## Complete example

```json
{
  "format": "NAFExtendedAnimationPack",
  "schemaVersion": 2,
  "pack": {
    "id": "author.dances",
    "displayName": "Author Dance Collection",
    "extensions": { "author.package": { "edition": 1 } }
  },
  "defaults": {
    "requirements": { "skeleton": "Human", "legacyGameSex": "Any" },
    "expectedBodyType": "Any",
    "tags": ["dance"]
  },
  "animations": [
    {
      "id": "author.dances.belly",
      "actors": [
        {
          "file": "author\\Dances\\belly.hkx",
          "expectedBodyType": "Femme",
          "role": "solo",
          "extensions": { "author.actor": { "energy": 0.4 } }
        }
      ],
      "tags": ["belly"],
      "extensions": { "author.animation": { "style": "belly" } }
    },
    {
      "id": "author.dances.swing",
      "actors": [ { "file": "author\\Dances\\swing.hkx" } ]
    }
  ],
  "animationGroups": [
    {
      "id": "author.dances.sequence",
      "sequential": true,
      "stages": [
        {
          "animation": "author.dances.belly",
          "loops": 1,
          "extensions": { "author.stage": { "name": "opening" } }
        },
        {
          "animation": "author.dances.swing",
          "loops": { "min": 1, "max": 2 },
          "weight": 1
        }
      ]
    }
  ],
  "positionTrees": [
    {
      "id": "author.dances.tree",
      "root": {
        "id": "belly",
        "position": "author.dances.belly.position",
        "duration": 5.0,
        "forceComplete": false,
        "children": [
          {
            "id": "swing",
            "position": "author.dances.swing.position",
            "duration": 5.0
          }
        ]
      }
    }
  ],
  "positions": [
    {
      "id": "author.dances.belly.position",
      "animation": "author.dances.belly",
      "displayName": "Belly Dance",
      "tags": ["solo"]
    },
    {
      "id": "author.dances.swing.position",
      "animation": "author.dances.swing",
      "displayName": "Swing Dance"
    },
    {
      "id": "author.dances.staged",
      "animationGroup": "author.dances.sequence",
      "displayName": "Two-Stage Dance"
    },
    {
      "id": "author.dances.tree.position",
      "positionTree": "author.dances.tree",
      "displayName": "Dance Tree"
    }
  ]
}
```

## Identity and references

All animation, position, group, and tree IDs are explicit global,
case-insensitive registry IDs. `pack.id` identifies provenance and metadata; it
does not prefix or rewrite definition IDs. Reverse-domain names are recommended.
An HKX path is a resource, not an identity, so multiple definitions may use the
same file.

A position must specify exactly one non-empty `animation`, `animationGroup`, or
`positionTree`. Groups reference animation IDs. Tree nodes reference position
IDs. References may target definitions in the same document, another v2 file,
legacy JSON v1, or XML. All documents are collected before phased reference
registration, so array order and forward file references do not matter.

XML registers first and wins collisions. JSON sources use normalized lexical
path order; within a source, array order applies. The first definition of a
registry ID wins. A v1 animation/position pair remains atomic.

## Animations and actor slots

`animations` is an array with no framework-imposed small pack limit. Every
animation requires an `id` and non-empty `actors` array. Every actor requires an
HKX `file`. Actor array order is slot order.

Actor fields:

- `requirements.skeleton`: behavior skeleton name.
- `requirements.legacyGameSex`: `Any`, `Male`, or `Female`; this is the only
  hard game-record sex restriction.
- `expectedBodyType`: `Any`, `Masc`, or `Femme`; descriptive metadata only.
- `role`: optional `giver`, `receiver`, or `solo` metadata. `giver` and
  `receiver` describe applicable participant functions; `solo` describes a
  participant for whom that distinction is not meaningful, such as an
  individual dancer or poser. Omission means that no role was authored.
- `erection`: optional Boolean instruction metadata. `false` is distinct from
  omission; NAF does not itself apply erection state.
- `extensions`: opaque namespaced metadata attached to that slot.

Top-level actor `skeleton` and deprecated `gender` remain accepted for v1
compatibility.

Role is descriptive metadata only. It does not determine game-record sex,
expected anatomy or body type, erection state, actor eligibility, actor count,
or animation assignment. Single-actor animations do not require a role. A
minimal explicit solo slot is therefore valid:

```json
{
  "id": "author.dances.solo",
  "actors": [
    {
      "file": "author\\Dances\\solo.hkx",
      "role": "solo"
    }
  ]
}
```

## Defaults and overrides

Supported package defaults are:

- `defaults.requirements.skeleton` (default `Human`)
- `defaults.requirements.legacyGameSex` (default `Any`)
- `defaults.expectedBodyType` (default `Any`)
- `defaults.tags` (added to animations and positions)

An explicit actor value overrides its package default. Animation/position tags
are additive. Role, erection, stage behavior, conditions, and actor-specific
extensions are deliberately not inherited. `null` is not a clearing operator;
use valid explicit values or omit a field.

## Positions

Positions support `id`, one target reference, `displayName`, `hidden`, `tags`,
`locations`, `startMorphSet`, `stopMorphSet`, `startEquipmentSet`,
`stopEquipmentSet`, `loadPriority`, and namespaced `extensions`. Multiple
positions may reference one animation or group. Display names never replace IDs.

## Groups and staged positions

`animationGroups` uses the original NAF group runtime. `sequential: true`
advances through stages in authored order and loops back after completion.
`false` selects stages using `weight`. Each stage requires `animation`, accepts
`loops` as an integer or `{ "min": N, "max": N }`, accepts `weight`, and may
carry extensions. A selectable staged position references the group through
`animationGroup`.

Stage changes continue to emit existing animation/stage update reasons. The
ABI-v1 snapshot reports the resolved current animation, group ID, stage index,
actor metadata for that animation, group metadata, and active-stage extensions.

## Position trees

`positionTrees` maps directly to NAF's recursive position-tree runtime. Each
tree has an `id` and `root`. Nodes require `position` and accept `id`, `duration`,
`forceComplete`, recursive `children`, and extensions. Trees are bounded to 256
levels and 100,000 nodes per document. Existing runtime cycle checks remain in
force. The active node's metadata is added to the existing ABI-v1 metadata
array without changing the public structure layout.

## Metadata scopes

Namespaced extensions are preserved at package, animation, actor-slot,
position, group, stage, tree, and tree-node scope. Active scene snapshots expose
only metadata relevant to the selected position and currently resolved
animation/group stage/tree node. No JSON-library object crosses the native ABI.
Passion-specific gameplay semantics are intentionally not defined by NAF.

## Validation and error isolation

Invalid JSON syntax, format, schema version, `pack`, or top-level collection
types reject the document. Invalid individual definitions are rejected with
file/type/ID diagnostics while unrelated valid definitions remain eligible.
Missing references discard the dependent group, tree, or position. Duplicate
IDs resolve deterministically to the first valid registered definition.

The loader limits cached data to 512 MiB, normalized definitions to 100,000,
tree depth to 256, and tree nodes to 100,000. Missing HKX resources are reported
by the established dynamic-idle playback path when selected.

## Cache and reload

Both v1 and v2 use `Data/NAF/NAFPackCache.bin`. Cache format version 2 stores
pointer-free normalized documents as CBOR with the exact source manifest,
fingerprints, bounds, and checksum. Adding, removing, or editing a source
invalidates the whole cache. Writes remain atomic. Old cache versions are
rejected and rebuilt automatically.

Cold source normalization and warm-cache reconstruction use the same phased
registration. Hot reload clears all JSON metadata/registries and repeats the
same process. Active scenes retain their existing shared runtime objects under
NAF's established reload lifetime behavior.

## XML concept mapping

| XML concept | JSON v2 |
|---|---|
| `animationData/animation` | `animations[]` |
| animation `actor` | `animations[].actors[]` |
| `positionData/position animation=` | `positions[].animation` |
| `animationGroupData/animationGroup` | `animationGroups[]` |
| group `stage` | `animationGroups[].stages[]` |
| staged position referencing a group | `positions[].animationGroup` |
| `positionTreeData/tree/branch` | `positionTrees[].root/children` |
| position tags/display data | position `tags` / `displayName` |

Schema v2 does not add XML conditions, equipment/action-set definitions,
morph-set definitions, face timelines, furniture definitions, or Passion
gameplay behavior. Existing XML remains supported for those authoring surfaces.

Schema v1 remains documented separately and is dispatched explicitly without
reinterpretation.
