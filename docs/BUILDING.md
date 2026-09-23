# Rebuild and package notices

The Windows helper uses Unreal's development packaging path, passes `-MaxParallelActions=2` and `-NoUBA`, and packages with `-nodebuginfo`. The local build confirmed the two-action limit, while its log still identified a UBA local executor; the flag alone is not evidence that UBA was disabled. Copying notices does not invoke compilation or UAT.

Before a full package build, prepare a reviewed runtime-notice companion with schema `halveth.runtime-notices.v1`. Supply it explicitly:

```powershell
./scripts/build.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' `
  -Stage Package -RuntimeNoticesRoot 'D:\ReleaseNotices\Runtime-Notices'
```

The sample notice directory is a placeholder; it is not downloaded by the helper. `Package` and `All` require this parameter. Other stages do not. Missing or stale notices stop a successful package handoff rather than silently omitting the license companion.

The reviewed companion used for this release records legal-document bytes from `Engine/Source/ThirdParty/Licenses` and selected license/notice text files in `Engine/Extras/ThirdPartyNotUE`. Each manifest entry binds its relative installed-engine source path, relative companion destination path, type, byte length and SHA-256. It includes 1,299 source files in the current snapshot. The copy helper accepts only the declared legal-document categories and extensions, checks path containment, rejects links, and verifies every prepared file against both its manifest and the current installed engine file before writing package data.

After packaging, the helper copies and verifies:

- The validated runtime-notice companion, including its scope README and manifest.
- Original MIT source license and Unreal third-party notice.
- Original icon license and provenance.
- The Windows player instructions (`PLAY-WINDOWS.txt`).
- Poly Haven CC0 license, attribution/source README and map manifest.

The copy under `Artwork-Notices/README.md` receives an explicit note that manifest paths and hashes identify the raw PNGs in the source release. The packaged game uses imported/cooked Unreal assets; raw PNGs are not placed in the notice directory. The CC0 license and map manifest retain their source bytes.

The helper does not copy engine program source, developer tools, binaries or unrelated engine directories. A broad notice companion is not a runtime dependency inventory and does not imply that every named library is included in the game. Retain the notices already emitted by Unreal's own packaging process.

To add or verify notices in an already-built package without invoking UAT:

```powershell
./scripts/copy_runtime_notices.ps1 -PackageRoot './Builds/Windows' `
  -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' `
  -RuntimeNoticesRoot 'D:\ReleaseNotices\Runtime-Notices'
```

The copy command emits a compact JSON result with hash-check outcomes. It deliberately makes no storefront-acceptance or complete legal-compliance claim. Regenerate or review the companion after engine/plugin changes when installed-source hashes no longer match.
