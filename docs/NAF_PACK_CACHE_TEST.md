# NAFPackCache production regression

`NAFPackCache.bin` is experimental JSON-definition cache data, not a release
asset. It is created at `Data/NAF/NAFPackCache.bin` and may be deleted safely.

Use a disposable mod-manager profile and retain `NAF.log` plus copies/hashes of
the cache after each step. Do not install synthetic scale fixtures in the live
profile.

1. Delete only `Data/NAF/NAFPackCache.bin`; launch and require `source files`.
2. Relaunch unchanged and require `NAFPackCache.bin` with identical registered
   definition IDs, labels and conflict results.
3. Reload NAF configuration unchanged and require the same warm result.
4. Add one valid `.naf.json`; reload and require a source rebuild plus the new ID.
5. Remove it; reload and require a rebuild with the ID absent.
6. Modify only display/actor metadata; reload and verify the new metadata.
7. Change an animation ID; require the old ID absent and new ID present.
8. Add JSON colliding with XML; require XML to win cold and warm.
9. Remove the conflicting XML and reload; require the cached JSON definition to
   become available through normal re-registration.
10. Reverse JSON filename ordering for a deliberate JSON/JSON collision and
    require lexical first-wins behavior.
11. Truncate or corrupt a copy of the cache; launch and require safe source
    rebuild, never partial registration or startup failure.
12. Alter the cache-version field in a copy; require rejection and rebuild.
13. Delete the cache and repeat the cold build.
14. Repeat an unchanged load and compare ordered IDs and metadata byte-for-byte.

For actor metadata, install the two-slot Phase 2C fixture with valid HKX paths.
The API harness must report `naf.extended.actor` keys associated with their slot:
slot 0 `Masc`/`giver`/`true`; slot 1 `Femme`/`receiver` with no erection key.
Stopping must deliver one Started before Updated, a rich SceneEnding snapshot,
then the snapshot-free SceneEnded tombstone.
