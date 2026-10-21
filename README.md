# NAF Extended for Fallout 4 1.11.240

NAF Extended is an unofficial extension of Native Animation Framework (NAF)
1.0.14 for Fallout 4 `1.11.240.0` and F4SE `0.7.9`. It preserves the complete
framework restored by the `port/1.11.240` branch and adds Runtime Database
resolution, an optional native C API, JSON HKX registration and caching,
managed facial services, and deterministic furniture-group aggregation.

Original NAF was created by Snapdragon/Deweh. This project preserves that
lineage and is not an official upstream continuation.

## Installation model

NAF Extended is an update layered over the original NAF 1.0.14 installation.

1. Install the original NAF package and its assets.
2. Install F4SE `0.7.9` for Fallout 4 `1.11.240.0`.
3. Install Runtime Database and its matching Fallout 4 runtime data.
4. Install the Extended `NAF.dll` and `NAF.pex` at their normal Data paths.

Runtime Database is mandatory. Extended deliberately disables Address Library
fallback so a missing or wrong database fails closed rather than resolving a
different runtime's address. Runtime Database remains a separate dependency and
is not redistributed here.

The original menu SWFs, ESP, MCM configuration, XML/default data, and other
unchanged framework assets remain supplied by the original NAF installation.

## Compatibility

- Fallout 4 `1.11.240.0`
- F4SE `0.7.9`
- Runtime Database for Fallout 4 `1.11.240.0`
- Microsoft Visual C++ runtime

Optional integrations:

- LooksMenu 1.7.0.4, or the previously validated 1.7.0.3 binary, enables body
  morph support. Exact module/hash/ABI validation remains fail closed.
- MCM provides convenient hotkey configuration; the NAF menu remains callable
  without it through `cgf "NAF.ToggleMenu"` or Papyrus.
- OCBP is optional and its absence does not disable Body Creator or Studio.

NAF Bridge is a separate customized NAF implementation and cannot be installed
alongside Extended as a second `NAF.dll`. Hit Facial Expression is not a
supported integration because its external script lifecycle can reapply state
outside NAF's ownership.

LookAt Cam is retained as a legacy upstream mode with an orientation/order
caveat. Normal NAF FreeCam is recommended. If using LookAt, configure it before
starting the scene; disable it if its fixed-facing behavior becomes undesirable.

## Native API and content extensions

The dependency-free public C header is
[`f4se-plugin/include/NAFExtendedAPI.h`](f4se-plugin/include/NAFExtendedAPI.h).
It exposes negotiated scene, authored-face, and dynamic-expression services.
See [`docs/NAF_EXTENDED_NEXUS_API_GUIDE.md`](docs/NAF_EXTENDED_NEXUS_API_GUIDE.md)
for a short introduction and [`docs/NAF_EXTENDED_NATIVE_API_PROPOSAL.md`](docs/NAF_EXTENDED_NATIVE_API_PROPOSAL.md)
for the detailed contract.

Experimental `.naf.json` animation definitions coexist with XML and normalize
into the same runtime animation/position registries. Their schema is documented
in [`docs/NAF_EXTENDED_JSON_SCHEMA_V1_EXPERIMENTAL.md`](docs/NAF_EXTENDED_JSON_SCHEMA_V1_EXPERIMENTAL.md).

## Building from source

Requirements:

- Visual Studio 2022, Desktop development with C++, MSVC v143, and a Windows SDK;
- CMake 3.20 or newer;
- vcpkg configured through `VCPKG_ROOT`;
- Python 3 for audits;
- initialized Git submodules.

Clone the Extended branch and initialize its exact dependencies:

```powershell
git clone --branch extended/1.11.240 --recurse-submodules `
  https://github.com/LZB7/Native-Animation-Framework.git
cd Native-Animation-Framework
git submodule update --init --recursive
```

Configure and build without deployment or the development harness:

```powershell
cmake -S f4se-plugin -B build-release `
  -G "Visual Studio 17 2022" -A x64 `
  -DCOPY_BUILD=OFF -DBUILD_NAFEXT_API_TEST=OFF
cmake --build build-release --config Release
```

The DLL is `build-release/Release/NAF.dll`. vcpkg resolves `spdlog` and
`libzippp`; the other dependencies are pinned submodules:

- CommonLibF4RD: `171b6cf9b147d8baab18b1182861bb214747a000`
- ik: `b1700d9ad080c82143e2fb83e2594604bee1b9bb`
- pugixml: `6909df2478f7eb092e8e5b5cda097616b2595cc6`

`COPY_BUILD=OFF` is required for reproducible/non-deploying builds. The optional
API harness is a developer test plugin and is not part of the user package.

## Maintainers and distribution

Start with [`docs/NAF_EXTENDED_MAINTAINER_GUIDE.md`](docs/NAF_EXTENDED_MAINTAINER_GUIDE.md).
The release-file manifest is in
[`docs/NAF_EXTENDED_DISTRIBUTION.md`](docs/NAF_EXTENDED_DISTRIBUTION.md).
Detailed port evidence remains in `PORTING.md` and `PORTING_EXTENDED.md`.

## Credits and license

Native Animation Framework was created by Snapdragon/Deweh. Upstream and
third-party contributors remain credited in `COPYRIGHT.txt`. NAF Extended is
an unofficial extension of that work.

The source remains GPL-3.0 under `LICENSE.txt`. CommonLibF4RD, ik, pugixml, and
other third-party components retain their own licenses and attribution.
