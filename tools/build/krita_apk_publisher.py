#!/usr/bin/env python3
"""Publish the latest *successful* WSL Android APK to a stable GitHub Release.

Called by local-build.sh after compilation or a verified incremental reuse.
Uses the existing GitHub CLI login (same one as krita_build_reporter.py).
Never requires a GitHub credential in this repository.
"""
import argparse
import datetime as dt
import json
import re
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import zipfile

from apk_size_report import digest as sha256

REPOSITORY = "murrdusa/krita-android-custom-build"
DEFAULT_TAG = "krita-android-latest"
ASSET = "krita-android-custom-arm64-debug.apk"
STATUS_CONTEXT = "local/android-apk-published"
MIN_APK_SIZE = 8_000_000


def run(args, timeout=600):
    return subprocess.run(args, text=True, capture_output=True, timeout=timeout)


def gh(*args, timeout=600):
    return run(["gh", *args], timeout=timeout)


def github_status(revision, state, description, target):
    try:
        gh("api", "--method", "POST",
           f"repos/{REPOSITORY}/statuses/{revision}",
           "-f", f"context={STATUS_CONTEXT}",
           "-f", f"state={state}",
           "-f", f"description={description[:140]}",
           "-f", f"target_url={target}", timeout=25)
    except (OSError, subprocess.TimeoutExpired):
        pass


def gh_checked(description, revision, target, *args, timeout=600):
    """Run gh; report a failure status if it cannot run or times out."""
    try:
        return gh(*args, timeout=timeout)
    except (OSError, subprocess.TimeoutExpired) as exc:
        print(f"[APK publish] {description} failed: {exc}", file=sys.stderr)
        github_status(revision, "failure", f"APK publish failed: {description}",
                      target)
        return None


def asset_matches(asset, size, checksum):
    """Require exact size and, when provided, a matching sha256 digest."""
    if not isinstance(asset, dict) or asset.get("size") != size:
        return False
    remote = str(asset.get("digest") or "")
    if remote.lower().startswith("sha256:"):
        return remote.split(":", 1)[1].strip().lower() == checksum.lower()
    return True


def choose_apk(directory):
    choices = sorted(
        (p for p in directory.glob("*.apk")
         if p.is_file() and p.stat().st_size > MIN_APK_SIZE),
        key=lambda p: (("arm64" in p.name.lower()), p.stat().st_mtime),
        reverse=True)
    for path in choices:
        try:
            if zipfile.is_zipfile(path):
                with zipfile.ZipFile(path) as archive:
                    if "AndroidManifest.xml" in archive.namelist():
                        return path
        except (OSError, zipfile.BadZipFile):
            continue
    return None


def release_metadata(tag):
    response = gh("api", f"repos/{REPOSITORY}/releases/tags/{tag}", timeout=25)
    if response.returncode:
        return None
    return json.loads(response.stdout)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--release-tag", default=DEFAULT_TAG,
                        help="Versioned preview tag; default is existing latest")
    parser.add_argument("--state-file", type=Path, required=True)
    args = parser.parse_args()
    revision = args.revision.strip()
    tag=args.release_tag.strip()
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]{2,63}",tag):
        print("[APK publish] Invalid release tag",file=sys.stderr)
        return 2
    release_url=f"https://github.com/{REPOSITORY}/releases/tag/{tag}"
    direct_url=f"https://github.com/{REPOSITORY}/releases/download/{tag}/{ASSET}"
    if len(revision) != 40 or any(x not in "0123456789abcdef" for x in revision):
        print("[APK publish] Invalid revision; refusing upload.", file=sys.stderr)
        return 2
    apk = choose_apk(args.output_dir)
    if apk is None:
        print("[APK publish] No valid APK found; nothing to upload.", file=sys.stderr)
        return 2
    if shutil.which("gh") is None:
        print("[APK publish] GitHub CLI is not installed. Existing build is safe; "
              "install gh once to enable releases.", file=sys.stderr)
        return 2
    authorized = gh_checked("GitHub CLI check", revision, release_url,
                            "auth", "status", timeout=25)
    if authorized is None:
        return 1
    if authorized.returncode:
        print("[APK publish] Existing GitHub CLI authentication is unavailable. "
              "Run 'gh auth login' once if needed.", file=sys.stderr)
        return 2

    checksum = sha256(apk)
    apk_size = apk.stat().st_size
    previous = {}
    try:
        previous = json.loads(args.state_file.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        pass
    try:
        release = release_metadata(tag)
    except (OSError, subprocess.TimeoutExpired) as exc:
        print("[APK publish] Could not read release metadata: " + str(exc),
              file=sys.stderr)
        github_status(revision, "failure",
                      "APK publish failed: GitHub CLI error", release_url)
        return 1
    existing_asset = next(
        (asset for asset in (release or {}).get("assets", [])
         if asset.get("name") == ASSET), None)
    if (previous.get("sha256") == checksum and
            previous.get("revision") == revision and existing_asset):
        if asset_matches(existing_asset, apk_size, checksum):
            github_status(revision, "success", "APK already published on GitHub",direct_url)
            print("[APK publish] Already published: " + direct_url)
            return 0
        print("[APK publish] Remote asset does not match the local APK; "
              "refusing to overwrite it.", file=sys.stderr)
        github_status(revision, "failure",
                      "APK publish refused: remote asset mismatch", release_url)
        return 1

    notes = (
        "Unofficial experimental ARM64 Krita debug build.\n\n"
        f"Source commit: `{revision}`\n\n"
        f"APK SHA-256: `{checksum}`\n\n"
        "Install only if you trust this repository; preserve backups of your "
        "drawings. This release is generated by the existing local WSL build.\n"
    )
    title = "Krita Mobile – experimental window preview" if tag != DEFAULT_TAG \
            else "Krita Android – latest test APK"
    if release is None:
        result = gh_checked("Release creation", revision, release_url,
                            "release", "create", tag,
                            "--repo", REPOSITORY,
                            "--target", revision,
                            "--title", title,
                            "--notes", notes, timeout=90)
        if result is None:
            return 1
        if result.returncode:
            print("[APK publish] Failed to create GitHub Release: " +
                  (result.stderr or result.stdout)[-650:], file=sys.stderr)
            github_status(revision, "failure", "APK publish failed: check GitHub auth permissions",release_url)
            return 1
    else:
        # Keep independent preview tags separate from the stable APK release.
        result = gh_checked("Release notes update", revision, release_url,
                            "release", "edit", tag,
                            "--repo", REPOSITORY,
                            "--title", title,
                            "--notes", notes, timeout=90)
        if result is not None and result.returncode:
            print("[APK publish] Could not update release notes: " +
                  (result.stderr or result.stdout)[-300:], file=sys.stderr)

    if previous.get("sha256") != checksum or not existing_asset:
        # Give the asset a fixed name without copying the entire APK twice.
        with tempfile.TemporaryDirectory(
                prefix=".krita-publish-", dir=str(args.output_dir)) as tmp:
            path = Path(tmp) / ASSET
            try:
                os.link(apk, path)
            except OSError:
                shutil.copy2(apk, path)
            result = gh_checked("APK upload", revision, release_url,
                                "release", "upload", tag, str(path),
                                "--repo", REPOSITORY, "--clobber", timeout=1200)
        if result is None:
            return 1
        if result.returncode:
            print("[APK publish] Upload failed: " +
                  (result.stderr or result.stdout)[-750:], file=sys.stderr)
            github_status(revision, "failure", "APK upload failed; see GitHub release permissions",release_url)
            return 1

    try:
        verified = release_metadata(tag)
    except (OSError, subprocess.TimeoutExpired) as exc:
        print("[APK publish] Could not verify the upload: " + str(exc),
              file=sys.stderr)
        github_status(revision, "failure",
                      "APK upload verification failed: GitHub CLI error",
                      release_url)
        return 1
    attached = next(
        (a for a in (verified or {}).get("assets", [])
         if a.get("name") == ASSET and asset_matches(a, apk_size, checksum)), None)
    if attached is None:
        print("[APK publish] GitHub did not confirm the uploaded asset "
              f"(expected {apk_size} bytes).", file=sys.stderr)
        github_status(revision, "failure", "APK upload was not verified",release_url)
        return 1
    record = {
        "revision": revision,
        "source": apk.name,
        "size": apk_size,
        "sha256": checksum,
        "url": direct_url,
        "release_tag": tag,
        "updated_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
    }
    args.state_file.parent.mkdir(parents=True, exist_ok=True)
    tmp = args.state_file.with_suffix(".tmp")
    tmp.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    tmp.replace(args.state_file)
    github_status(revision, "success", "Android APK published", direct_url)
    print("[APK publish] SUCCESS: " + direct_url)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, subprocess.TimeoutExpired, ValueError) as exc:
        print("[APK publish] Failed: " + str(exc), file=sys.stderr)
        raise SystemExit(1)
