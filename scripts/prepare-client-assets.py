#!/usr/bin/env python3
"""Stage exact Luanti 5.17.0 client assets and a SHA-256 package manifest."""

import hashlib
import json
import pathlib
import shutil
import sys
import uuid


task_root = pathlib.Path(sys.argv[1]).resolve()
repo_root = pathlib.Path(sys.argv[2]).resolve()
source_root = task_root / "client-sources/luanti"
state = json.loads((task_root / "client-source-state.json").read_text())
source_hashes = state["files"]
lock = json.loads((repo_root / "client/tls-dependencies.lock.json").read_text())
staging = task_root / "client-assets"
pending = task_root / f"client-assets.tmp-{uuid.uuid4().hex}"
pending.mkdir()
manifest = {}


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def add(source, destination, expected_sha=None):
    if not source.is_file() or source.is_symlink():
        raise ValueError(f"Missing or linked input: {source}")
    actual_sha = sha256(source)
    if expected_sha is not None and actual_sha != expected_sha:
        raise ValueError(f"Input changed: {source}")
    target = pending / destination
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    if sha256(target) != actual_sha:
        raise ValueError(f"Copy changed: {target}")
    manifest[destination.as_posix()] = {
        "sha256": actual_sha,
        "size": target.stat().st_size,
    }


try:
    for relative_dir in (
        "builtin",
        "client/shaders",
        "textures/base/pack",
        "fonts",
    ):
        source_dir = source_root / relative_dir
        if not source_dir.is_dir():
            raise ValueError(f"Missing Luanti asset directory: {source_dir}")
        files = sorted(path for path in source_dir.rglob("*") if path.is_file())
        if not files:
            raise ValueError(f"Empty Luanti asset directory: {source_dir}")
        for source in files:
            relative = source.relative_to(source_root)
            add(source, pathlib.Path("luanti/share") / relative,
                source_hashes.get(relative.as_posix()))
            if relative.as_posix() not in source_hashes:
                raise ValueError(f"Unrecorded Luanti asset: {relative}")
    add(source_root / "LICENSE.txt", pathlib.Path("luanti/share/LICENSE.txt"),
        source_hashes["LICENSE.txt"])
    # Binary recipients receive complete component terms, not just a summary.
    for source in sorted((repo_root / "LICENSES").rglob("*")):
        if source.is_file():
            add(source, pathlib.Path("luanti/share/licenses") /
                source.relative_to(repo_root / "LICENSES"))
    for name in ("LICENSE", "LICENSE-MIT.txt", "CREDITS.md", "THIRD_PARTY_NOTICES.md"):
        add(repo_root / name, pathlib.Path("luanti/share/licenses") / name)
    add(repo_root / "client/assets/default.conf",
        pathlib.Path("luanti/default.conf"))
    add(repo_root / "client/assets/launch.ini",
        pathlib.Path("luanti/launch.ini"))
    add(task_root / "client-deps/share/luanti/client/cacert.pem",
        pathlib.Path("client/cacert.pem"), lock["ca_bundle"]["sha256"])

    if staging.exists():
        shutil.rmtree(staging)
    pending.rename(staging)
    log_dir = task_root / "logs"
    log_dir.mkdir(exist_ok=True)
    output = log_dir / "client-package-assets.json"
    temporary = output.with_suffix(".json.tmp")
    temporary.write_text(json.dumps(dict(sorted(manifest.items())), indent=2) + "\n")
    temporary.replace(output)
    print(f"Staged {len(manifest)} verified files at {staging}")
    print(f"Manifest: {output}")
finally:
    if pending.exists():
        shutil.rmtree(pending)
