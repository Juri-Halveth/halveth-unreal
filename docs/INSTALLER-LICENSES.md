# Windows installer: NSIS notices and source availability

Verified 2026-09-23 for HALVETH Portal Garden 0.1.0, built with the official
NSIS 3.12 compiler and its solid LZMA compression module. This document concerns
the Setup executable. The portable game ZIP does not contain the NSIS installer.

## License scope

NSIS code, bundled plug-ins, documentation and graphics are generally under the
zlib/libpng license, subject to the exceptions identified in its own notices.
The LZMA compression module is under the Common Public License 1.0 with the
NSIS LZMA linking exception. That exception permits linked code to retain its own
license; modifications to the LZMA module itself remain subject to CPL 1.0.
This build uses the supplied NSIS modules without local modifications.

The project's original game code and installer script retain their stated MIT
license. This does not relicense NSIS, Unreal Engine, or their third-party
components. Game and artwork notices remain in the application payload.

The complete compiler-supplied terms, including CPL 1.0 and the LZMA exception,
are reproduced unchanged in [NSIS-COPYING.txt](NSIS-COPYING.txt). The official
reference is the [NSIS license page](https://nsis.sourceforge.io/License).

## Release assets and corresponding source

Distribute these companion files with the Windows Setup release:

- `NSIS-COPYING.txt`, the complete unchanged notice file.
- `INSTALLER-LICENSES.md`, this explanation and source location.
- `nsis-3.12-src.tar.bz2`, the unchanged official corresponding NSIS source archive.
- `nsis-3.12-src.tar.bz2.sha256`, its verification checksum.

The source archive and checksum are prepared in the project's `dist` directory.
The two notice documents are in `docs`. These are separate release assets; they
were not inserted into or used to change the already-built game payload.
Publish them alongside `HALVETH-Portal-Garden-Setup-0.1.0-x64.exe` so recipients
can obtain the source from the same distributor and release as the binary.
Preparation of these local files alone does not assert that an upload occurred.

The source is also available from the official upstream
[NSIS 3.12 source download](https://sourceforge.net/projects/nsis/files/NSIS%203/3.12/nsis-3.12-src.tar.bz2/download).
The [NSIS download page](https://nsis.sourceforge.io/Download) and
[versioned release directory](https://sourceforge.net/projects/nsis/files/NSIS%203/3.12/)
identify the corresponding version. Retain the complete source archive and its
existing copyright and license notices when redistributing it.

## Verification record

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `nsis-3.12-src.tar.bz2` | 1,818,389 | `f3ed7a8e4aa2cf4e8cf47d3b563a02559e0cb4934db2662b2f9661b824e2b186` |
| `NSIS-COPYING.txt` | 15,632 | `388357c1215ff403c5ebde3a5ecd273e68f8b79a579996775245d1ee65442aba` |

The downloaded archive's SHA-256 matches the value published on the official
SourceForge download page. The notice copy's SHA-256 matches the installed
NSIS 3.12 compiler package's `COPYING` file. The source archive was neither
extracted nor executed during this preparation.

## Contributor terms

For NSIS/LZMA, the complete accompanying terms govern. To the extent permitted
by applicable law, all NSIS/LZMA contributors supply their components without
warranties or conditions, including title, non-infringement, merchantability and
fitness for a particular purpose, and exclude liability for direct, indirect,
special, incidental and consequential damages, including lost profits.
Any HALVETH-specific promises or terms that differ from those contributor terms
are offered by HALVETH alone, not by NSIS/LZMA contributors. This notice does not
restrict rights or remedies that applicable law does not allow to be excluded.
