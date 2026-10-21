# NAF Extended deterministic furniture-group merging

## Why this differs from preservation NAF

Original NAF registers a parsed furniture group through the same
`IDMap::priority_insert` policy used by replaceable definitions. A second XML
group with the same case-insensitive ID therefore contributes nothing unless it
has a strictly higher `loadPriority`, in which case it replaces the complete
first group. Equal-priority results depend on which parallel XML parse reaches
the registry first.

NAF Extended limits the correction to furniture groups. Animations, positions,
trees, groups, and all other registries retain their existing collision rules.

## Merge rules

- Every valid definition with the same case-insensitive furniture group ID is a
  contributor, regardless of priority.
- Distinct plugin-qualified furniture forms and keyword EditorIDs are combined.
- Before game-data linking, forms are deduplicated by normalized plugin name and
  parsed local FormID. `0006F9DC` and `6F9DC` from `Fallout4.esm` therefore name
  one entry, while the same local ID in another plugin remains distinct.
- Exact duplicate keyword strings are removed before linking. After game data
  is ready, forms and keywords are deduplicated again by resolved engine pointer,
  catching textual aliases that resolve to the same record.
- Invalid form contributions retain existing parse/link diagnostics and are
  removed from the usable linked list without discarding other valid entries.
- `startAnimation` and `stopAnimation` are group-level metadata. They come as a
  pair from one winning definition rather than being mixed independently.
- Higher `loadPriority` wins group metadata. At equal priority, the
  lexicographically earlier normalized source path wins. A winning omitted
  property remains omitted; lower-priority metadata does not leak into it.
- Forms and keywords are stored in stable normalized order before linking, so
  merge results do not depend on parallel parse completion order.

Furniture entries currently have no separate per-entry metadata beyond their
form or keyword identity, so no entry conditions are flattened during merging.

## Cache and reload

The XML cache stores source filename/text rather than a premerged furniture
registry. Cold and warm loads therefore execute the same parser and merge path.
Existing source-set and timestamp validation invalidates the cache when a file
is added, removed, or changed.

Hot reload clears the furniture registry, reparses current sources, finalizes
the merged groups, and relinks forms. It does not append to a previous merged
result, so repeated reloads cannot accumulate entries and removing a contributor
removes only that contributor's unique entries.

When verbose configuration logging is enabled, a shared group emits one summary
containing its ID, contributor count, unique pre-link form/keyword counts, and
the source/priority that supplied group metadata. Runtime furniture lookup is
unchanged and has no new per-frame or per-lookup logging.

## Author guidance

Existing `furnitureData` XML remains valid. Independent packs may extend a
shared group simply by declaring the same group ID. Use priority only when a
pack intentionally needs its group-level start/stop animation metadata to win;
priority does not suppress distinct furniture contributions.

The bundled Phase 4 fixtures use `Phase4SharedFurniture` in three files. Their
expected normalized result is four Fallout4.esm furniture forms and three
keyword EditorIDs. They are development fixtures and are not release content.

## Production runtime coverage

The shared-group path is runtime-validated on Fallout 4 1.11.240. The production
log recorded all three fixture contributors loading and one deterministic merge
of `Phase4SharedFurniture` containing four form contributions and three keyword
contributions. The resulting `Phase4 Furniture Merge Test` position was started
and stopped successfully three times, including after a normal save reload, and
the in-game furniture selector exposed every intended chair category.

The explicit FormIDs in this development fixture did not resolve as
`TESBoundObject` records in that test installation and were rejected by the
existing link validation. The observed chair coverage therefore directly
validates additive keyword contributions and safe handling of invalid form
contributions. Runtime coverage of two distinct, valid explicit-form
contributions and the dedicated configuration hot-reload command remains
separate from this successful production test; neither gap changes the verified
startup merge behavior.

## NAF Bridge comparison

The NAF Bridge source is publicly available in the
[`NoAbleEngles/Native-Animation-Framework`](https://github.com/NoAbleEngles/Native-Animation-Framework)
fork. Source revision `460e9617755518c06675d73c557b0f1ed181dc91` contains two
furniture-specific aggregation paths:

- its runtime `Global::IDMap::priority_insert(std::shared_ptr<Data::Furniture>)`
  sorts both contributors' form lists and combines them with `std::set_union`;
- its NAFicator parsed-object registry merges furniture entries identified by
  their exact `source` and `form` strings, adding missing entries and conditionally
  enriching/replacing duplicates.

This confirms that additive handling of repeated furniture group IDs was an
intentional Bridge compatibility correction. The Bridge runtime overload only
combines the `forms` member; it does not combine keyword contributions or define
deterministic conflict handling for `startAnimation`, `stopAnimation`, and
equal-priority metadata. NAF Extended therefore uses the same additive behavior
as its compatibility baseline while retaining its stronger deterministic rules:
forms and keywords merge, entries are deduplicated before and after linking, and
one source deterministically supplies the group-level metadata. No Bridge code
was copied and NAF Extended has no NAF Bridge dependency.

## JSON format boundary

The experimental `NAFExtendedAnimation` JSON schema currently defines one
animation plus one position. Its loader registers only `Global::Animations` and
`Global::Positions`; it has no furniture-data object and never writes to
`Global::Furnitures`. Consequently, the XML duplicate-group failure mode cannot
occur within the current JSON parser.

A JSON animation/position may eventually refer to an XML-defined furniture group
when that schema gains location constraints; runtime lookup must continue to use
the same merged `Global::Furnitures` registry. If a future JSON schema directly
defines furniture groups, those definitions must be routed through
`FurnitureMap::merge_insert` and covered by the same deterministic merge/reload
audits rather than generic ID collision handling.

