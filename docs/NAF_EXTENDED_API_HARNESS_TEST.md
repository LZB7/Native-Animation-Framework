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
   `LZB7.NAF.Extended`, ABI 1, the table size/capabilities, and both the direct
   or announcement discovery result plus safe duplicate-discovery handling.
3. Start JSON position `J1` (shown as `Belly Dancing — JSON Test`).
4. Require `SceneStarted`, actor slot/FormID/handle/name/animation information,
   a valid packed copy, self-unregistration, resubscription, and late enumeration
   of the active scene.
5. Change scene speed or position. Require `SceneUpdated` and a line confirming
   the previously copied snapshot still validates after its originating callback.
6. Stop normally. Require `SceneEnding`, `SceneEnded`, and final test
   unregistration.
7. Start a second scene. The harness must emit no scene callbacks after its
   successful final unregistration.
8. Remove `NAFExtAPITest.dll` after testing.

## Missing-provider test

In a separate disposable profile, leave the harness installed and omit
`NAF.dll`. It must load normally and report that the provider is unavailable;
it must not dereference an API pointer or prevent the game from reaching the
main menu. This test does not require Runtime Database unless another installed
plugin requires it.

F4SE has no general hot-unload lifecycle for native plugins. The harness also
exports `NAFExtAPITest_Shutdown` for debugger-driven explicit cleanup, while the
normal deterministic unregistration test occurs after the first scene ends.
