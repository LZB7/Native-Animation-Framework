# NAF Extended distribution manifest

## End-user payload

Install over a complete original NAF 1.0.14 installation:

```text
Data/
├── F4SE/
│   └── Plugins/
│       └── NAF.dll
└── Scripts/
    └── NAF.pex
```

`NAF.pex` is required because Extended adds the `ResetFaceExpression` Papyrus
native declaration. Publish the matching source separately for mod authors:

```text
Data/Scripts/Source/User/NAF.psc
```

The original NAF installation continues to supply its ESP, Interface SWFs, MCM
files, scripts not changed by Extended, XML/default data, and Creator/Studio
assets. Do not duplicate those unchanged assets in an Extended DLL update.

## Separate dependencies

- Fallout 4 1.11.240.0
- F4SE 0.7.9
- Runtime Database with matching 1.11.240 data
- Original NAF 1.0.14 installation

LooksMenu and MCM remain optional. OCBP is not required.

## Project provenance

NAF Extended is a community-maintained continuation and expansion of the
GPLv3-licensed Native Animation Framework created by Snapdragon/Deweh. Snapdragon
acknowledged the compatibility port and confirmed GPLv3 distribution of modified
versions. Extended is independently maintained and does not claim official
upstream status.

## Separate developer example

`NAFExtSceneExample` is an optional, source-included scene-listener plugin for
API developers. Distribute it as a separate developer resource; neither its DLL
nor its source belongs in the required NAF Extended runtime package.

## Exclusions

Do not ship `NAFExtAPITest.dll`, test fixtures, development expression presets,
benchmarks, PDB files in the ordinary user package, `NAFPackCache.bin`, Runtime
Database binaries, reference executables/databases, build directories, logs,
crash reports, converted third-party animation packs, or `NAFExtSceneExample`.

## Release checks

Build Release/x64 with MSVC v143, `COPY_BUILD=OFF`, and
`BUILD_NAFEXT_API_TEST=OFF`. Verify the DLL and PEX correspond to the same source
revision, record SHA-256 for the DLL, run the full audit suite, inspect the staged
payload against this manifest, and perform no automatic copy into a live game.
