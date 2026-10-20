# NAF Extended API Runtime Harness

`NAFExtAPITest.dll` is a development-only F4SE plugin. It consumes only the
public `NAFExtendedAPI.h` contract and documented F4SE/CommonLib interfaces.
It is not included in framework release packaging.

The harness listens for the NAF F4SE announcement and also attempts direct
acquisition through `NAFExt_GetAPI`. It validates provider identity, ABI,
capabilities and required functions, logs all four scene events, validates the
self-contained packed snapshot format, self-unregisters and resubscribes on the
first started scene, then performs subscribe-first active-scene enumeration.
After the first scene ends it unregisters permanently; a second scene should
produce no harness callbacks.

See `PORTING_EXTENDED.md` for the complete runtime procedure.
