# NAF Extended API Runtime Harness

`NAFExtAPITest.dll` is a development-only F4SE plugin. It consumes only the
public `NAFExtendedAPI.h` contract and the documented raw F4SE interface ABI.
It does not link CommonLib, Runtime Database, spdlog, or any NAF implementation
library.
It is not included in framework release packaging.

During plugin load the harness registers only for F4SE lifecycle messages. It
does not contact NAF until a save or new game has been entered. At that point it
acquires the API through `NAFExt_GetAPI`, validates provider identity, ABI,
capabilities and required functions, subscribes to all four scene events, and
performs active-scene enumeration. It validates the self-contained packed
snapshot format for delivered scenes. After the first scene ends it unregisters
permanently; a second scene should produce no harness callbacks.

See `PORTING_EXTENDED.md` for the complete runtime procedure.
