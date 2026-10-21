# NAF Extended API harness runtime test

The harness is a development tool and must not be shipped with NAF Extended.

## Installation

1. Install Fallout 4 1.11.240, F4SE 0.7.9, Runtime Database, and the current
   `NAF.dll`.
2. Copy the built `NAFExtAPITest.dll` from
   `Codex Production Files/build-release/api-test/Release` to
   `Data/F4SE/Plugins`.
3. Do not copy its PDB unless debugging.

## Test

1. Launch through F4SE and load an ordinary save.
2. Open `Documents/My Games/Fallout4/F4SE/NAFExtAPITest.log`. Require provider
   `LZB7.NAF.Extended`, ABI 1, the table size/capabilities, deferred direct
   discovery after game entry, and initial active-scene enumeration.
3. Start the Phase 2C two-actor JSON fixture (or installed equivalent).
4. Require `SceneStarted`, actor slot/FormID/handle/name/animation information,
   a valid packed copy, and namespaced `naf.extended.actor` records. Slot 0 must
   expose `Masc`, `giver`, and erection `true`; slot 1 must expose `Femme` and
   `receiver` with no erection key.
5. Change scene speed or position. Require `SceneUpdated` and a line confirming
   the previously copied snapshot still validates after its originating callback.
6. Stop normally. Require `SceneEnding`, `SceneEnded`, and final test
   unregistration. `SceneEnding` must retain the final position, animation,
   tags, and actor metadata; `SceneEnded` remains a tombstone.
7. Start a second scene. The harness must emit no scene callbacks after its
   successful final unregistration.
8. Remove `NAFExtAPITest.dll` after testing.

## Phase 3B facial-service controls

Copy `NAFExtAPITest.face.txt` beside the harness at
`Data/F4SE/Plugins/NAFExtAPITest.face.txt` and replace its two placeholder
lines with two valid authored face-animation IDs. After loading a save, the
harness acquires the optional facial table but performs no facial action until
a key is pressed:

- F6: play animation A on PlayerCharacter (`0x14`)
- F7: play animation B
- F8: query and log managed state
- F9: legacy stop
- F10: strong expression reset
- F11: request A and B immediately (B must win)
- F12: request A and immediately reset (A must not reappear)

The harness uses a small independent polling thread intentionally: this tests
the service's documented arbitrary-caller-thread queueing contract. It does not
link CommonLibF4RD, initialize Runtime Database, resolve engine pointers, or
perform automatic playback on load. Successful discovery logs the facial
service version, table size, capability mask, and four controlled negotiation
rejections. Query success logs status, generation, flags, and animation ID.

## Missing-provider test

In a separate disposable profile, leave the harness installed and omit
`NAF.dll`. It must load normally and report that the provider is unavailable;
it must not dereference an API pointer or prevent the game from reaching the
main menu. This test does not require Runtime Database unless another installed
plugin requires it.

F4SE has no general hot-unload lifecycle for native plugins. The harness also
exports `NAFExtAPITest_Shutdown` for debugger-driven explicit cleanup, while the
normal deterministic unregistration test occurs after the first scene ends.
