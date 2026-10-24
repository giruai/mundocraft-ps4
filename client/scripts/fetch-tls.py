#!/usr/bin/env python3
"""Fetch and verify locked, public TLS inputs into this port's private tree."""

import argparse
import hashlib
import json
import pathlib
import shutil
import tarfile
import urllib.request


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def fetch(url, destination, expected):
    if not destination.exists():
        partial = destination.with_name(destination.name + ".partial")
        if partial.exists():
            partial.unlink()
        request = urllib.request.Request(url, headers={"User-Agent": "Luanti-PS4-source-fetch"})
        with urllib.request.urlopen(request, timeout=60) as response, partial.open("wb") as output:
            shutil.copyfileobj(response, output)
        if digest(partial) != expected:
            partial.unlink()
            raise RuntimeError(f"download hash mismatch: {destination.name}")
        partial.replace(destination)
    if digest(destination) != expected:
        raise RuntimeError(f"existing archive hash mismatch: {destination}")


def extract(archive, destination, expected):
    marker = destination / ".archive-sha256"
    if destination.exists():
        if marker.is_file() and marker.read_text().strip() == expected:
            # Verify contents too: a hash marker alone cannot detect source edits.
            originals = set()
            with tarfile.open(archive, "r:*") as bundle:
                for member in bundle.getmembers():
                    if not member.isfile():
                        continue
                    relative = pathlib.Path(*pathlib.PurePosixPath(member.name).parts[1:])
                    originals.add(relative)
                    original = bundle.extractfile(member).read()
                    actual = (destination / relative).read_bytes()
                    if relative.as_posix() == "lib/cf-socket.c" and destination.name.startswith("curl-"):
                        changed = original.replace(b"#ifdef IP_BIND_ADDRESS_NO_PORT\n", b"#if defined(IP_BIND_ADDRESS_NO_PORT) && defined(SOL_IP)\n")
                        if actual not in (original, changed):
                            raise RuntimeError(f"unexpected patched curl source: {relative}")
                    elif actual != original:
                        raise RuntimeError(f"modified TLS source: {relative}")
            actual_files = {p.relative_to(destination) for p in destination.rglob("*") if p.is_file() and not ("__pycache__" in p.parts and p.suffix == ".pyc")}
            if actual_files != originals | {pathlib.Path(".archive-sha256")}:
                raise RuntimeError(f"unexpected files in TLS sources: {destination.name}")
            return
        raise RuntimeError(f"source exists without matching archive marker: {destination}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    staging = destination.with_name(destination.name + ".partial")
    if staging.exists():
        raise RuntimeError(f"incomplete extraction exists: {staging}")
    staging.mkdir()
    try:
        with tarfile.open(archive, "r:*") as source:
            members = source.getmembers()
            root = destination.name
            for member in members:
                parts = pathlib.PurePosixPath(member.name).parts
                if not parts or parts[0] != root or ".." in parts or member.issym() or member.islnk():
                    raise RuntimeError(f"unexpected archive path or link: {member.name}")
                if not (member.isfile() or member.isdir()):
                    raise RuntimeError(f"unexpected archive member: {member.name}")
                relative = pathlib.Path(*parts[1:])
                target = staging / relative
                if member.isdir():
                    target.mkdir(parents=True, exist_ok=True)
                elif relative.parts:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with source.extractfile(member) as input_file, target.open("wb") as output:
                        shutil.copyfileobj(input_file, output)
                    target.chmod(member.mode & 0o777)
        (staging / ".archive-sha256").write_text(expected + "\n")
        staging.rename(destination)
    except Exception:
        shutil.rmtree(staging)
        raise


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    root = args.root.resolve()
    lock = json.loads((root / "client/tls-dependencies.lock.json").read_text())
    private = root / ".local/client-tls"
    downloads = private / "downloads"
    downloads.mkdir(parents=True, exist_ok=True)
    for name in ("curl", "mbedtls", "ca_bundle"):
        entry = lock[name]
        archive = downloads / entry["archive"]
        fetch(entry["url"], archive, entry["sha256"])
        if name == "ca_bundle":
            target = private / "cacert.pem"
            if target.exists() and digest(target) != entry["sha256"]:
                raise RuntimeError(f"existing CA bundle hash mismatch: {target}")
            if not target.exists():
                shutil.copyfile(archive, target)
        else:
            extract(archive, private / "sources" / f"{name}-{entry['version']}", entry["sha256"])
        print(f"verified {name}: {entry['sha256']}")


if __name__ == "__main__":
    main()
