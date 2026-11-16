# Krita Mobile Studio (experimental)

Unofficial ARM64 Android build of Krita 5.3.4, built from KDE source tag `v6.0.4`.
It keeps Krita's native painting engine and adds a touch-first host, home and
project gallery, draft/version recovery, and a compact workspace for phones and
foldables.

**Status:** experimental, no stable release. Local WSL builds compile and
package an ARM64 debug APK; functional verification is ongoing and on-device
coverage is partial. Confirm the exact commit result before treating a build as
good. Historical notes: [docs/RELEASE-NOTES.md](docs/RELEASE-NOTES.md).

**Working on the code?** Read [AGENTS.md](AGENTS.md) first. Current handoff:
[docs/DEVELOPMENT_STATE.md](docs/DEVELOPMENT_STATE.md). PC build status:
[Issue #1](https://github.com/jpegzilla/krita-android-custom-build/issues/1).

## Requirements

- Windows 10/11 with WSL2 Ubuntu and Docker Desktop WSL integration, or Linux
  with Docker. The scripts assume `bash`.
- Python 3.8+ (standard library only) and Git.
- Optional: authenticated GitHub CLI (`gh`) for build reporting and APK
  publishing.
- For the Qt-based C++ regressions: Qt5 Core/Gui/Widgets development packages.
  The host-only regressions need only a C++17 compiler.

## Build

```bash
git clone https://github.com/jpegzilla/krita-android-custom-build.git
cd krita-android-custom-build
bash local-build.sh setup    # first run only: docker env, Qt deps, container
bash local-build.sh build    # apply patches, incremental compile, package APK
```

`setup` clones KDE's Android Docker environment and downloads the Android Qt
dependencies. `build` clones Krita `v6.0.4` on first use, applies
`tools/patches/apply_all.py`, compiles incrementally in the container, and
copies the APK out.

Other modes:

- `bash local-build.sh status` lists existing APKs.
- `bash local-build.sh patch` applies the source patches without compiling.
- `python3 tools/patches/apply_all.py /path/to/krita-v6.0.4` applies the same
  patch order to a manual checkout.

Output defaults to `${KRITA_LOCAL_ROOT:-$HOME/krita-local-android}/persistent/krita-custom-apks`.
The `persistent/` directory holds the source checkout, container state, and
build tree; keep it to stay incremental.

Useful environment variables:

| Variable | Effect |
|---|---|
| `KRITA_LOCAL_ROOT` | Build root (default `~/krita-local-android`) |
| `KRITA_BUILD_JOBS` | Parallel compile jobs (default `6`) |
| `KRITA_FORCE_BUILD=1` | Rebuild even when app inputs are unchanged |
| `KRITA_RECONFIGURE=1` | Re-run CMake configure |

On the maintainer's PC, an installed WSL watcher builds pushed `main` commits
and publishes the result to commit status `local/android-wsl-build` and Issue
#1. That watcher is not part of this repository; if you run without it, build
manually as above. The watcher and its reporting conventions come from the
upstream `murrdusa/krita-android-custom-build` project.

Detailed Windows/WSL2 setup: [docs/LOCAL-BUILD.md](docs/LOCAL-BUILD.md).

### GitHub Actions fallback

The build workflow is manual-only. Use it when the PC is unavailable, not for
normal iteration. `.github/workflows/build-krita-android.yml` supports
`validate`, `preflight`, `compile`, and `build` modes. `validate` only applies
the patches and checks the diff; it does not compile C++ or Java.

## Tests

Python (standard library only):

```bash
python -m unittest discover -s tests/python
```

C++ regressions compile and run a small host program:

```bash
tests/run_mobile_layer_identity_regression.sh   # host compiler only
tests/run_mobile_layer_reorder_regression.sh    # host compiler only
tests/run_mobile_archive_regression.sh          # requires Qt5 Widgets
tests/run_mobile_native_view_stack_regression.sh # requires Qt5 Widgets
```

The runners add `mobile_studio/include` to the include path. A successful
Android compilation does not prove runtime behavior; manual and device checks
live in [docs/TEST-PLAN.md](docs/TEST-PLAN.md) and
[docs/FIRST-DEVICE-SMOKE.md](docs/FIRST-DEVICE-SMOKE.md).

## Install

- The debug APK uses package `org.krita.mobilestudio.debug`, separate from Play
  Store Krita (`org.krita`). Check Android's install prompt; do not uninstall
  Krita.
- Use disposable artwork and back up important drawings.
- Gallery **All Files Access** is optional, must be granted in Android
  Settings, and does not grant access to other apps' private `Android/data`
  directories.
- APK uploading is paused. Builds stay local unless an upload is explicitly
  requested; see [docs/APK-UPLOAD-REQUEST.md](docs/APK-UPLOAD-REQUEST.md).

## Repository layout

```
mobile_studio/     C++ overlay copied into Krita
  include/mobile_studio/   headers: host, engine adapters, policies, panels
  src/                     implementation files
tools/
  build/           local build, APK reporting, cache, size audit
  patches/         source patch scripts and apply_all.py
tests/
  cpp/             Qt and host-only C++ regressions
  python/          standard-library Python tests
docs/              design notes, plans, and project handoff
```

Root `local-build.sh` and the three `krita_build_*.py` files are compatibility
shims for the installed watcher. The implementations live under `tools/build/`.

## Documentation

- [AGENTS.md](AGENTS.md) — working rules, UI/gesture contracts, build and
  reporting workflow.
- [docs/DEVELOPMENT_STATE.md](docs/DEVELOPMENT_STATE.md) — current handoff and
  known limitations.
- [docs/PRODUCT-ROADMAP.md](docs/PRODUCT-ROADMAP.md) — approved feature
  priorities.
- [docs/LOCAL-BUILD.md](docs/LOCAL-BUILD.md) — Windows/WSL2 and Docker setup.
- [docs/BUILD-REPORTING.md](docs/BUILD-REPORTING.md) — watcher reporter,
  commit statuses, build dashboard.
- [docs/TEST-PLAN.md](docs/TEST-PLAN.md) — test coverage and device checks.
- [docs/ENGINE-BOUNDARIES.md](docs/ENGINE-BOUNDARIES.md) — host/engine
  boundaries.
- [docs/CORE-FUNCTIONALITY-GATES.md](docs/CORE-FUNCTIONALITY-GATES.md) —
  stabilization gates.
- [docs/NATIVE-CANVAS-ROADMAP.md](docs/NATIVE-CANVAS-ROADMAP.md) — native
  canvas staging.
- [docs/FINGER-CONTACT-PRESSURE.md](docs/FINGER-CONTACT-PRESSURE.md) —
  contact-area pressure input.
- [docs/RELEASE-NOTES.md](docs/RELEASE-NOTES.md) — historical release notes.

## Contributing

- Follow [AGENTS.md](AGENTS.md). Use Conventional Commits, one line.
- Batch related changes into a compilable increment; prefer the PC build over
  cloud jobs.
- Do not redesign the interface while the startup and engine work is moving.
  Preserve working controls, documents, and saved content.
- Do not claim device testing from a successful build.

## Limitations

- Unofficial build. Debug signing, broad optional storage permission, and no
  release process.
- Android SAF behavior limits cross-app storage access.
- Some tool panels still need narrow-width work; see
  [docs/RELEASE-NOTES.md](docs/RELEASE-NOTES.md) and
  [docs/DEVELOPMENT_STATE.md](docs/DEVELOPMENT_STATE.md).
