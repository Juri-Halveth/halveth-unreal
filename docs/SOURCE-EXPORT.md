# Public source export

Run `python scripts/package_source.py --check` to validate the public source surface without producing an archive. Run `python scripts/package_source.py` after final source edits to create a content-addressed ZIP and SHA-256 sidecar under the ignored `dist` directory. The default version comes from `Config/DefaultGame.ini`; `--version` explicitly selects another three-part version.

The exporter reads only these surfaces:

| Surface | Included content |
|---|---|
| Project root | `.uproject`, README, original LICENSE, third-party notices, `.gitignore`, `.gitattributes` |
| `Source` | C++ headers/implementation and Unreal C# build rules |
| `Config` | INI configuration |
| `scripts`, `tests` | Python/PowerShell build helpers and test sources |
| `docs` | Public Markdown/text/JSON documents and reviewed image captures |
| `assets` | Original icon and associated asset/license files |
| `ArtSource` | Source PNGs and their source/license records |
| `installer` | NSIS source files |
| Exact extra paths | `Build/Windows/Application.ico`, `.github/workflows/source.yml` |

Each tree has an explicit extension allowlist. Hidden subdirectories, environment files and compiled artifacts are not admitted. `Saved`, generated `Content`, `Binaries`, `Intermediate`, `Builds`, `DerivedDataCache`, `Engine` and repository internals are never traversed by this export. Unreal's generated texture import metadata therefore remains outside the public source ZIP.

The exporter rejects symlinks/junctions in admitted source locations, credential assignments in admitted text, recognizable private-key/token forms and absolute personal-directory paths. Error messages identify the file and rule without printing the matching value. This is a bounded automated gate, not a substitute for reviewing screenshots or determining asset rights.

Every admitted source-art PNG must belong to the recorded Poly Haven manifest. Byte counts, SHA-256, PNG header and declared dimensions are checked before packaging. Changed or unlisted source art stops the export rather than silently entering a release. Provider/license records accompany those files.

`SOURCE-MANIFEST.json` inside the ZIP records every exported file's relative path, size and SHA-256. ZIP entry order and timestamps are fixed. The archive filename includes the manifest digest prefix; repeating the same export with the same compression runtime produces identical bytes. The SHA-256 sidecar identifies the actual archive. The source limit defaults to 512 MiB and can be changed explicitly with `--max-source-mib`; crossing the limit stops the export.

## CI scope

`.github/workflows/source.yml` runs on Windows and Linux. It executes the Python contract tests, compiles and runs the actual production layout header with MSVC/GCC, checks project structure and verifies the export plus source-art hashes. The workflow uses read-only repository permissions and pinned official action commits. It requires no Unreal installation, game license, credentials or paid service.

This portable CI does not compile the Unreal modules or test the rendered game. Native engine compilation, material preparation, gameplay smoke, visual checks, packaging and installer checks retain their separate receipts. Publishing the repository triggers CI; merely including the workflow does not prove a completed GitHub run.
