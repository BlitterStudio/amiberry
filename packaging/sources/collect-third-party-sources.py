#!/usr/bin/env python3
"""Collect the exact third-party GPL/LGPL sources shipped in Amiberry's Apple builds.

Distributing Amiberry binaries that contain third-party GPL/LGPL code requires
offering the exact corresponding source (GPLv2 S3 / GPLv3 S6 / LGPL S4). This
tool builds the source archive that is published as an asset of every release
tag, next to the Apple binaries.

Two phases:

  scan      Run on the build machine, right after a macOS build. Inspects the
            dylibs dylibbundler copied into the .app bundle, maps each one back
            to the Homebrew formula (and exact keg version) it was poured from,
            and records the formula metadata. Cheap: no downloads.

  assemble  Run once per release. Takes one or more scan files, unions the
            components, downloads each formula's upstream source tarball
            (urls.stable.url, verifying the formula's sha256), the exact
            homebrew-core formula file for that version (which carries any
            patches Homebrew applied) and any external patch files, and packs
            everything with a manifest into
            amiberry-<tag>-third-party-sources.tar.xz.

Which components are archived is not hard-coded: anything whose Homebrew
licence string contains GPL or LGPL and whose dylibs are found in a bundle is
archived (libmpeg2, libmpg123, FFmpeg, x264, x265, LAME, libserialport, glib,
... whichever the runner's brew actually shipped). Components already in the
Amiberry repository (external/mt32emu, external/floppybridge, ...) are listed
in the manifest but not duplicated: the tagged Amiberry source tree is their
corresponding source. Permissively licensed dependencies are listed for
transparency; no source offer is owed for them.

Usage:
  collect-third-party-sources.py scan \
      --app build/Amiberry.app --variant macos --arch arm64 \
      --out third-party-scan-arm64.json
  collect-third-party-sources.py assemble \
      --scan third-party-scan-x86_64.json --scan third-party-scan-arm64.json \
      --tag v8.4.2 --out amiberry-v8.4.2-third-party-sources.tar.xz

The Mac App Store (MACOS_APP_STORE=ON) and iOS builds are built outside this
repository's CI; run `scan` for those bundles locally and pass the scan files
to `assemble` so a single archive can cover every distributed variant.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
SOURCE_ROOT_DEFAULT = SCRIPT_DIR.parent.parent

# dylibs that dylibbundler copies into the bundle but that do not come from
# Homebrew. Their corresponding source is offered elsewhere.
IN_TREE_DYLIBS = {
    "libcapsimage.dylib": {
        "component": "capsimage",
        "source": "Amiberry repository, external/capsimage",
        "licence": "LicenseRef-KryoFlux-NC (non-commercial; excluded from Mac App Store builds)",
    },
    "libfloppybridge.dylib": {
        "component": "floppybridge",
        "source": "Amiberry repository, external/floppybridge",
        "licence": "MPL-2.0 OR GPL-2.0-or-later",
    },
    "libmt32emu.dylib": {
        "component": "mt32emu",
        "source": "Amiberry repository, external/mt32emu",
        "licence": "LGPL-2.1-or-later (statically linked)",
    },
}
EXTERNAL_PLUGINS = {
    "qemu-uae.dylib": {
        "component": "qemu-uae",
        "source": "https://github.com/BlitterStudio/amiberry-qemu-uae (own source offer)",
        "licence": "GPL-2.0-or-later",
    },
}

HOMEBREW_CORE = "Homebrew/homebrew-core"
CORE_RAW = "https://raw.githubusercontent.com/Homebrew/homebrew-core"
CORE_API = "https://api.github.com/repos/Homebrew/homebrew-core"


def die(msg: str) -> "NoReturn":  # type: ignore[valid-type]
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def info(msg: str) -> None:
    print(f"==> {msg}")


def run(cmd: list[str], **kw) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def curl(url: str, dest: Path, max_bytes: int | None = None) -> None:
    # No Authorization header here: these are public upstream/homebrew URLs;
    # authenticated requests go through github_api() only.
    cmd = ["curl", "-fsSL", "--retry", "3", "--connect-timeout", "30", url, "-o", str(dest)]
    subprocess.run(cmd, check=True)
    if not dest.is_file() or dest.stat().st_size == 0:
        die(f"download produced nothing: {url}")
    if max_bytes and dest.stat().st_size > max_bytes:
        die(f"download too large ({dest.stat().st_size} bytes): {url}")


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def is_copyleft(licence: str) -> bool:
    """True for any SPDX string that grants GPL/LGPL terms (incl. dual/tri licences)."""
    return "GPL" in licence  # GPL-2.0-only, GPL-3.0-or-later, LGPL-2.1-or-later, ....


# ---------------------------------------------------------------------------
# scan
# ---------------------------------------------------------------------------

def brew_prefix(brew: str) -> Path:
    r = run([brew, "--prefix"])
    if r.returncode != 0:
        die(f"could not locate Homebrew: {r.stderr.strip()}")
    return Path(r.stdout.strip())


def brew_installed_json(brew: str) -> dict:
    r = run([brew, "info", "--json=v2", "--installed", "--quiet"])
    if r.returncode != 0:
        die(f"brew info --json=v2 --installed failed: {r.stderr.strip()}")
    return json.loads(r.stdout)


def cellar_dylib_index(prefix: Path) -> dict[str, set[tuple[str, str]]]:
    """Map dylib filename -> {(formula, keg_version)} across every installed keg.

    Follows symlinks (Homebrew ships libfoo.dylib -> libfoo.1.dylib -> libfoo.1.2.3.dylib),
    so a bundled file matches whichever name dylibbundler copied it under.
    """
    index: dict[str, set[tuple[str, str]]] = {}
    cellar = prefix / "Cellar"
    if not cellar.is_dir():
        die(f"no Homebrew Cellar at {cellar}")
    for formula_dir in sorted(cellar.iterdir()):
        if not formula_dir.is_dir():
            continue
        for keg in sorted(formula_dir.iterdir()):
            lib = keg / "lib"
            if not lib.is_dir():
                continue
            for root, _dirs, files in os.walk(lib):
                # dylibs live at lib/ or one level below (lib/gettext, ...); deeper
                # trees only contain headers/plugins.
                if Path(root).parent != lib and Path(root) != lib:
                    continue
                for name in files:
                    if name.endswith(".dylib"):
                        index.setdefault(name, set()).add((formula_dir.name, keg.name))
    return index


def bundle_dylibs(app: Path) -> list[Path]:
    dylibs: list[Path] = []
    for sub in ("Contents/Frameworks", "Contents/Resources/plugins", "plugins"):
        d = app / sub
        if d.is_dir():
            dylibs.extend(sorted(p for p in d.iterdir() if p.suffix == ".dylib"))
    return dylibs


def dylib_install_basename(path: Path) -> str:
    r = run(["otool", "-D", str(path)])
    if r.returncode != 0:
        die(f"otool -D failed for {path}: {r.stderr.strip()}")
    lines = [l for l in r.stdout.splitlines() if l.strip() and not l.startswith(str(path))]
    if not lines:
        return path.name
    return lines[0].strip().rsplit("/", 1)[-1]


def catalogue_gpl_components(catalogue_path: Path, variants: list[str]) -> list[dict]:
    """GPL/LGPL components whose source already lives in the Amiberry repository."""
    if not catalogue_path.is_file():
        die(f"licence catalogue not found: {catalogue_path}")
    catalogue = json.loads(catalogue_path.read_text())
    out = []
    for comp in catalogue.get("components", []):
        if not is_copyleft(comp.get("licence", "")):
            continue
        if not set(comp.get("platforms", [])) & set(variants):
            continue
        out.append(
            {
                "id": comp.get("id"),
                "name": comp.get("name"),
                "version": comp.get("version", ""),
                "licence": comp.get("licence"),
                "url": comp.get("url"),
            }
        )
    return sorted(out, key=lambda c: c["id"] or "")


def cmd_scan(args) -> None:
    app = Path(args.app).resolve()
    if not app.is_dir():
        die(f"app bundle not found: {app}")
    prefix = brew_prefix(args.brew)
    info(f"Homebrew prefix: {prefix}")

    index = cellar_dylib_index(prefix)
    formulas = {f["name"]: f for f in brew_installed_json(args.brew).get("formulae", [])}

    gpl: dict[str, dict] = {}
    permissive: dict[str, dict] = {}
    in_tree: dict[str, dict] = {}
    external: dict[str, dict] = {}
    unknown: list[str] = []

    for dylib in bundle_dylibs(app):
        basename = dylib.name
        if basename in IN_TREE_DYLIBS:
            meta = IN_TREE_DYLIBS[basename]
            entry = in_tree.setdefault(
                meta["component"],
                {"component": meta["component"], "source": meta["source"], "licence": meta["licence"], "dylibs": []},
            )
            entry["dylibs"].append(basename)
            continue
        if basename in EXTERNAL_PLUGINS:
            meta = EXTERNAL_PLUGINS[basename]
            entry = external.setdefault(
                meta["component"],
                {"component": meta["component"], "source": meta["source"], "licence": meta["licence"], "dylibs": []},
            )
            entry["dylibs"].append(basename)
            continue

        candidates = index.get(basename) or index.get(dylib_install_basename(dylib))
        if not candidates:
            unknown.append(f"{dylib.relative_to(app.parent)}")
            continue
        if len({c[0] for c in candidates}) > 1:
            unknown.append(f"{dylib.relative_to(app.parent)} (ambiguous formulae: "
                           f"{', '.join(sorted(f'{f}@{v}' for f, v in candidates))})")
            continue
        formula_name = next(iter(candidates))[0]
        kegs = sorted(v for (f, v) in candidates if f == formula_name)
        if len(kegs) > 1:
            # Several kegs of the same formula are installed (mid-upgrade);
            # the newest is what the build linked against.
            print(f"    warning: multiple {formula_name} kegs {kegs}; assuming {kegs[-1]}")
        keg_version = kegs[-1]
        formula = formulas.get(formula_name)
        if formula is None:
            unknown.append(f"{dylib.relative_to(app.parent)} (formula {formula_name} not reported installed)")
            continue
        licence = formula.get("license") or "(licence not declared)"
        target = gpl if is_copyleft(licence) else permissive
        entry = target.setdefault(
            formula_name,
            {
                "formula": formula_name,
                "keg_version": keg_version,
                "stable_version": formula.get("versions", {}).get("stable", ""),
                "licence": licence,
                "homepage": formula.get("homepage", ""),
                "dylibs": [],
            },
        )
        if entry["keg_version"] != keg_version:
            die(f"{formula_name}: bundle mixes kegs {entry['keg_version']} and {keg_version}")
        entry["dylibs"].append(basename)

    for entry in list(gpl.values()) + list(permissive.values()) + list(in_tree.values()) + list(external.values()):
        entry["dylibs"] = sorted(set(entry["dylibs"]))

    if unknown and not args.allow_unknown:
        die(
            "unmapped dylibs in the bundle (no Homebrew formula, not a known in-tree "
            "or external plugin binary):\n  " + "\n  ".join(unknown) + "\n"
            "Either Homebrew upgraded the formula since this bundle was built (the "
            "old keg is gone — rebuild before scanning), the dylib comes from a "
            "formula not in the Cellar, or it needs an entry in IN_TREE_DYLIBS/"
            "EXTERNAL_PLUGINS. Use --allow-unknown to record it as unaccounted."
        )

    scan = {
        "schema": 1,
        "variant": args.variant,
        "arch": args.arch,
        "app": str(app),
        "homebrew_prefix": str(prefix),
        "scanned_at_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "gpl_components": sorted(gpl.values(), key=lambda c: c["formula"]),
        "permissive_components": sorted(permissive.values(), key=lambda c: c["formula"]),
        "in_tree_components": sorted(in_tree.values(), key=lambda c: c["component"]),
        "external_plugins": sorted(external.values(), key=lambda c: c["component"]),
        "unresolved_dylibs": sorted(unknown),
    }
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(scan, indent=2) + "\n")

    info(f"variant {args.variant} ({args.arch}) scan written to {out}")
    for c in scan["gpl_components"]:
        print(f"    GPL/LGPL  {c['formula']} {c['keg_version']} [{c['licence']}] -> {', '.join(c['dylibs'])}")
    for c in scan["permissive_components"]:
        print(f"    permissive {c['formula']} {c['keg_version']} [{c['licence']}] (no source offer owed)")
    for c in scan["in_tree_components"]:
        print(f"    in-tree   {c['component']} (source already in the Amiberry repository)")
    for c in scan["external_plugins"]:
        print(f"    external  {c['component']} ({c['source']})")
    if unknown:
        print(f"    UNACCOUNTED: {unknown}")


# ---------------------------------------------------------------------------
# assemble
# ---------------------------------------------------------------------------

def formula_path_in_core(formula: str) -> str:
    # Homebrew shards homebrew-core as Formula/lib/ for lib*-prefixed names
    # and Formula/<first character>/ otherwise (non-letters go to Formula/@/);
    # see CoreTap#new_formula_subdirectory.
    shard = "lib" if formula.startswith("lib") else (formula[0].lower() if formula[0].isalpha() else "@")
    return f"Formula/{shard}/{formula}.rb"


def github_token() -> str | None:
    for var in ("GH_TOKEN", "GITHUB_TOKEN"):
        if os.environ.get(var):
            return os.environ[var]
    return None


def github_api(url: str, token: str | None) -> dict | list:
    cmd = ["curl", "-fsSL", "--retry", "3", "--connect-timeout", "30"]
    if token:
        cmd += ["-H", f"Authorization: Bearer {token}"]
    cmd.append(url)
    r = run(cmd)
    if r.returncode != 0:
        die(f"GitHub API request failed: {url}\n{r.stderr.strip()}")
    return json.loads(r.stdout)


def split_keg(keg_version: str) -> tuple[str, int]:
    """'0.5.1_1' -> ('0.5.1', 1); 'r3222' -> ('r3222', 0).

    A trailing _N on a keg directory is the formula's packaging revision
    (revision N in the .rb), not part of the upstream version.
    """
    if "_" in keg_version:
        upstream, _, suffix = keg_version.rpartition("_")
        if suffix.isdigit():
            return upstream, int(suffix)
    return keg_version, 0


def text_pkg_revision(rb_text: str) -> int | None:
    """Top-level `revision N` stanza (packaging revision, not a git ref)."""
    m = re.search(r'^  revision (\d+)$', rb_text, re.MULTILINE)
    return int(m.group(1)) if m else None


def formula_version(rb_text: str) -> str | None:
    m = re.search(r'^\s*version "([^"]+)"', rb_text, re.MULTILINE)
    return m.group(1) if m else None

def fetch_formula_rb(formula: str, keg_version: str, formulae_dir: Path,
                     origin_dir: Path, token: str | None, brew: str) -> Path:
    """Download the homebrew-core formula file for exactly keg_version.

    keg_version is the full keg directory name (e.g. 0.5.1_1); the formula's
    upstream version *and* packaging revision must both match. A formula's
    `version` is often implicit (derived from the stable URL), so a text
    match alone is not enough: when the file carries no explicit `version`
    stanza, brew's own parser decides whether it is the right one.

    The file is stored under <formula>/<keg_version>/ so two builds with
    different versions of the same formula (Intel + ARM legs) coexist.
    """
    upstream, pkg_rev = split_keg(keg_version)
    core_path = formula_path_in_core(formula)
    dest = formulae_dir / formula / keg_version / f"{formula}.rb"
    dest.parent.mkdir(parents=True, exist_ok=True)
    origin_dir.mkdir(parents=True, exist_ok=True)
    origin_file = origin_dir / f"{formula}-{keg_version}.origin"
    probe_dir = origin_dir.parent / "probes" / formula / keg_version
    probe_dir.mkdir(parents=True, exist_ok=True)

    def save(text: str, origin: str) -> None:
        dest.write_text(text)
        origin_file.write_text(origin + "\n")

    def matches(text: str) -> bool:
        explicit = formula_version(text)
        if explicit is not None:
            if explicit != upstream:
                return False
            rev = text_pkg_revision(text)
            # No `revision N` stanza means packaging revision 0.
            return (rev if rev is not None else 0) == pkg_rev
        # No explicit version stanza: let brew derive it from the URL. The
        # probe must be named <formula>.rb so brew can derive the class name.
        probe = probe_dir / f"{formula}.rb"
        probe.write_text(text)
        try:
            spec = parse_formula(probe, brew)
        except SystemExit:
            return False
        return (spec.get("version") == upstream
                and int(spec.get("pkg_revision") or 0) == pkg_rev)

    head_url = f"{CORE_RAW}/HEAD/{core_path}"
    r = run(["curl", "-fsSL", "--retry", "3", head_url])
    if r.returncode == 0 and r.stdout.strip():
        if matches(r.stdout):
            save(r.stdout, head_url)
            return dest
        info(f"{formula}: HEAD formula does not match installed keg {keg_version}; "
             f"walking homebrew-core history")
    else:
        info(f"{formula}: could not fetch HEAD formula ({r.stderr.strip()}); "
             f"walking homebrew-core history")

    commits: list[dict] = []
    for page in range(1, 4):  # up to 300 commits; formulae rarely lag further
        batch = github_api(f"{CORE_API}/commits?path={core_path}&per_page=100&page={page}", token)
        if not isinstance(batch, list) or not batch:
            break
        commits.extend(batch)
        if len(batch) < 100:
            break
    if not commits:
        die(f"{formula}: no homebrew-core history found for {core_path}")

    # Formulae with an explicit `version` are checked by text (fast); the
    # derived-version ones need a brew parse each, so bound that work.
    parsed = 0
    for commit in commits:
        sha = commit["sha"]
        raw_url = f"{CORE_RAW}/{sha}/{core_path}"
        r = run(["curl", "-fsSL", "--retry", "2", raw_url])
        if r.returncode != 0:
            continue
        if formula_version(r.stdout) is None:
            parsed += 1
            if parsed > 50:
                break
        if matches(r.stdout):
            save(r.stdout, raw_url)
            return dest
    die(f"{formula}: no homebrew-core revision found carrying version {upstream} "
        f"revision {pkg_rev} (checked {len(commits)} commits, {parsed} needed a "
        f"brew parse). Pin a newer bottle or add the source URL to this "
        f"archive manually.")


def parse_formula_via_brew(rb_path: Path, brew: str) -> "dict | None":
    """url/checksum/licence straight from brew's own formula parser.

    Returns None when brew cannot load the file (e.g. historical formulae
    whose DSL today's brew rejects); the caller falls back to a regex parse.
    """
    # Named so Homebrew can derive the class name; HOMEBREW_DEVELOPER in the
    # environment keeps `brew ruby` from flipping persistent developer mode on.
    named = rb_path.parent / rb_path.name
    code = (
        "f = Formulary.factory(ARGV[0]);"
        "require 'json';"
        "license = begin;"
        "  SPDX.license_expression_to_string(f.license);"
        "rescue StandardError;"
        "  f.license.to_s;"
        "end;"
        "puts({url: f.stable.url,"
        " checksum: (f.stable.checksum&.to_s rescue nil),"
        " tag: f.stable.specs[:tag],"
        " revision: f.stable.specs[:revision],"
        " pkg_revision: f.revision,"
        " license: license,"
        " version: f.version.to_s}.to_json)"
    )
    r = run([brew, "ruby", "-e", code, str(named)], env={**os.environ, "HOMEBREW_DEVELOPER": "1"})
    if r.returncode != 0:
        return None
    return json.loads(r.stdout)


def parse_formula_via_regex(rb_path: Path) -> dict:
    """Fallback parser when brew cannot load a historical formula file.

    Top-level stanzas are indented two spaces; resource/bottle blocks are
    deeper, so anchored patterns pick the formula's own url/sha256.
    """
    text = rb_path.read_text()

    def first(pattern):
        m = re.search(pattern, text, re.MULTILINE)
        return m.group(1) if m else None

    url = first(r'^  url "([^"]+)"$')
    tag = first(r'^    tag "([^"]+)"$')
    revision = first(r'^    revision "([^"]+)"$')
    checksum = first(r'^  sha256 "([0-9a-f]{64})"$')
    licence = first(r'^  license "([^"]+)"$')
    if not url:
        die(f"could not parse a stable url from {rb_path}")
    pkg_rev = text_pkg_revision(text)
    return {"url": url, "checksum": checksum, "tag": tag, "revision": revision,
            "pkg_revision": pkg_rev if pkg_rev is not None else 0,
            "license": licence or "", "version": formula_version(text) or "", "parser": "regex"}


def parse_formula(rb_path: Path, brew: str) -> dict:
    spec = parse_formula_via_brew(rb_path, brew)
    if spec is not None:
        return spec
    info(f"brew could not parse {rb_path.name} (historical DSL?); using a regex parse")
    return parse_formula_via_regex(rb_path)


PATCH_BLOCK_RE = re.compile(r'patch\s+do\b(?P<body>.*?)^\s*end\b', re.DOTALL | re.MULTILINE)


def external_patches(rb_text: str) -> list[dict]:
    """Homebrew `patch do ... end` blocks that carry data outside the .rb.

    url-patches are downloaded from their declared URL (with the formula's
    sha256); file-patches live in the homebrew-core repository itself under
    the declared path (e.g. Patches/<formula>/foo.patch) and are fetched
    from the same commit as the formula file. Inline __END__ DATA patches
    are already inside the .rb we archive.
    """
    patches = []
    for m in PATCH_BLOCK_RE.finditer(rb_text):
        body = m.group("body")

        def stanza(pattern):
            s = re.search(pattern, body)
            return s.group(1) if s else None

        url = stanza(r'url\s+"([^"]+)"')
        file = stanza(r'file\s+"([^"]+)"')
        sha = stanza(r'sha256\s+"([0-9a-f]{64})"')
        if url:
            patches.append({"kind": "url", "url": url, "sha256": sha})
        elif file:
            patches.append({"kind": "file", "path": file, "sha256": sha})
    return patches


def patch_source_url(patch: dict, formula: str, formula_origin: str) -> str:
    """Download URL for a patch block; needs the .rb's origin for file patches."""
    if patch["kind"] == "url":
        return patch["url"]
    m = re.search(r'homebrew-core/([^/]+)/', formula_origin)
    if not m:
        die(f"{formula}: cannot locate the homebrew-core revision in "
            f"'{formula_origin}' to fetch patch {patch['path']}")
    return f"{CORE_RAW}/{m.group(1)}/{patch['path']}"


def verify_patch(path: Path, expected: str | None, formula: str) -> tuple[str, str]:
    """(sha256, status). A declared checksum that does not match fails assembly."""
    actual = sha256_file(path)
    if expected:
        if actual != expected:
            die(f"{formula}: patch sha256 mismatch for {path.name}\n"
                f"  formula: {expected}\n  actual:  {actual}")
        print(f"    patch sha256 OK ({expected[:16]}...)")
        return actual, "verified against homebrew-core formula"
    print(f"    patch sha256 recorded ({actual[:16]}...; none declared)")
    return actual, "recorded (no checksum declared in the formula)"


def source_download_target(url: str, tag: str | None, revision: str | None, formula: str) -> tuple[str, str]:
    """(download URL, filename). For git-URL specs, fetch the host's archive endpoint."""
    if url.endswith(".git"):
        # GitLab instances (gitlab.com, code.videolan.org, ...) expose
        # <repo>/-/archive/<ref>/<project>-<ref>.tar.gz; that covers every
        # git-URL formula homebrew-core currently ships.
        base = url[:-4]
        ref = tag or revision
        if not ref:
            die(f"{formula}: git source without tag/revision; cannot build an archive URL")
        return f"{base}/-/archive/{ref}/{base.rsplit('/', 1)[-1]}-{ref}.tar.gz", \
            f"{formula}-{ref}.tar.gz"
    return url, f"{formula}-{url.rsplit('/', 1)[-1]}"


def verify_download(path: Path, checksum: str | None, formula: str) -> str:
    actual = sha256_file(path)
    if checksum:
        if actual != checksum:
            die(f"{formula}: sha256 mismatch for {path.name}\n"
                f"  formula: {checksum}\n  actual:  {actual}")
        print(f"    sha256 OK ({checksum[:16]}...)")
    else:
        # git-archive sources carry no formula checksum; at least prove the
        # tarball parses. bsdtar/GNU tar both list this way.
        r = run(["tar", "-tf", str(path)])
        if r.returncode != 0 or not r.stdout.strip():
            die(f"{formula}: downloaded archive is not a readable tarball: {path.name}")
        print(f"    tar OK, sha256 recorded ({actual[:16]}...)")
    return actual


def amiberry_version(source_root: Path) -> str:
    text = source_root / "CMakeLists.txt"
    if text.is_file():
        m = re.search(r"project\s*\(\s*amiberry\s+VERSION\s+(\S+)", text.read_text())
        if m:
            return m.group(1).strip('"')
    return ""


def git_revision(source_root: Path) -> dict:
    rev = {"git": "", "dirty": False}
    r = run(["git", "-C", str(source_root), "rev-parse", "HEAD"])
    if r.returncode == 0:
        rev["git"] = r.stdout.strip()
        s = run(["git", "-C", str(source_root), "status", "--porcelain"])
        rev["dirty"] = bool(s.stdout.strip())
    return rev


def cmd_assemble(args) -> None:
    scans = []
    for path in args.scan:
        p = Path(path)
        if not p.is_file():
            die(f"scan file not found: {p}")
        scans.append(json.loads(p.read_text()))
    if not scans:
        die("no scan files given")
    variants = sorted({s["variant"] for s in scans})
    token = github_token()
    tag = args.tag

    # Union components across scans: same formula at the same keg version
    # merges; different versions of the same formula stay separate entries.
    components: dict[tuple[str, str], dict] = {}
    permissive: dict[tuple[str, str], dict] = {}
    for scan in scans:
        for comp in scan.get("gpl_components", []):
            key = (comp["formula"], comp["keg_version"])
            entry = components.setdefault(key, {**comp, "dylibs": [], "variants": [], "archs": []})
            entry["dylibs"] = sorted(set(entry["dylibs"]) | set(comp["dylibs"]))
            entry["variants"] = sorted(set(entry["variants"]) | {scan["variant"]})
            entry["archs"] = sorted(set(entry["archs"]) | {scan.get("arch", "")})
        for comp in scan.get("permissive_components", []):
            key = (comp["formula"], comp["keg_version"])
            entry = permissive.setdefault(key, {**comp, "variants": [], "archs": []})
            entry["variants"] = sorted(set(entry["variants"]) | {scan["variant"]})
            entry["archs"] = sorted(set(entry["archs"]) | {scan.get("arch", "")})

    # GPL/LGPL components whose source ships inside the Amiberry repository
    # (src/, external/): the tagged source tree is their corresponding source,
    # so they are listed but not duplicated into this archive. Components the
    # bundles get as Homebrew dylibs (gpl_components above) are not in-tree
    # and must not be listed here.
    FORMULA_TO_CATALOGUE_ID = {
        "libmpeg2": "libmpeg2", "mpg123": "mpg123", "ffmpeg": "ffmpeg",
        "x264": "x264", "x265": "x265", "lame": "lame",
        "libserialport": "libserialport",
    }
    formula_backed = {FORMULA_TO_CATALOGUE_ID.get(f) for (f, _v) in components}
    vendored = {"mt32emu", "floppybridge", "capsimage"}
    in_tree = []
    for c in catalogue_gpl_components(Path(args.catalogue), variants):
        if c["id"] in formula_backed:
            continue  # covered by gpl_components with its exact source
        in_tree.append({
            **c,
            "source": (f"https://github.com/BlitterStudio/amiberry/tree/"
                       f"{tag or 'master'}/external/{c['id']}") if c["id"] in vendored
            else "Amiberry source tree (src/); see the tagged Amiberry source",
        })
    external = sorted(
        {c["component"]: c for s in scans for c in s.get("external_plugins", [])}.values(),
        key=lambda c: c["component"],
    )
    unresolved = sorted({u for s in scans for u in s.get("unresolved_dylibs", [])})

    out = Path(args.out or f"amiberry-{tag or 'local'}-third-party-sources.tar.xz")
    workdir = Path(args.work_dir) if args.work_dir else Path(tempfile.mkdtemp(prefix="amiberry-3rdp-"))
    root = workdir / f"amiberry-{tag or 'local'}-third-party-sources"
    (root / "components").mkdir(parents=True, exist_ok=True)
    try:
        manifest_components = []
        for (formula, keg_version) in sorted(components):
            comp = components[(formula, keg_version)]
            info(f"{formula} {keg_version} [{comp['licence']}] "
                 f"({', '.join(comp['variants'])}/{','.join(comp['archs'])})")
            # Everything this keg version contributes lives under its own
            # directory so two builds with different versions of the same
            # formula (Intel + ARM legs) never overwrite each other.
            ver_dir = f"{formula}/{keg_version}"
            rb = fetch_formula_rb(formula, keg_version,
                                  root / "homebrew-formulae", workdir / "origins",
                                  token, args.brew)
            spec = parse_formula(rb, args.brew)
            url, filename = source_download_target(
                spec["url"], spec.get("tag"), spec.get("revision"), formula)
            dest = root / "components" / ver_dir / filename
            dest.parent.mkdir(parents=True, exist_ok=True)
            curl(url, dest, max_bytes=800 * 1024 * 1024)
            actual = verify_download(dest, spec.get("checksum"), formula)
            origin = (workdir / "origins" / f"{formula}-{keg_version}.origin").read_text().strip()

            patches = []
            for p in external_patches(rb.read_text()):
                purl = patch_source_url(p, formula, origin)
                pname = purl.rsplit("/", 1)[-1].split("?")[0] or "patch"
                pdest = root / "patches" / ver_dir / pname
                pdest.parent.mkdir(parents=True, exist_ok=True)
                curl(purl, pdest)
                psha, pstatus = verify_patch(pdest, p["sha256"], formula)
                patches.append({
                    "source": purl,
                    "kind": p["kind"],
                    "file": f"patches/{ver_dir}/{pname}",
                    "sha256": psha,
                    "sha256_status": pstatus,
                })

            manifest_components.append({
                "formula": formula,
                "version": split_keg(keg_version)[0],
                "keg_version": keg_version,
                "licence": spec.get("license") or comp["licence"],
                "homepage": comp["homepage"],
                "source_url": url,
                "source_sha256": actual,
                "source_sha256_status": "verified against homebrew-core formula"
                if spec.get("checksum") else "recorded (git archive; formula carries no checksum)",
                "file": f"components/{ver_dir}/{filename}",
                "formula_file": f"homebrew-formulae/{ver_dir}/{formula}.rb",
                "formula_file_origin": origin,
                "formula_parser": spec.get("parser", "brew-ruby"),
                "patches": patches,
                "dylibs": comp["dylibs"],
                "variants": comp["variants"],
                "archs": comp["archs"],
            })

        source_root = Path(args.source_root)
        manifest = {
            "schema": 1,
            "generated_by": "packaging/sources/collect-third-party-sources.py",
            "purpose": ("Corresponding source (GPLv2 S3 / GPLv3 S6 / LGPL S4) for the "
                        "third-party GPL/LGPL components in the Amiberry Apple binaries "
                        "of this release."),
            "amiberry_version": amiberry_version(source_root),
            "amiberry_revision": git_revision(source_root),
            "tag": tag or "",
            "created_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "variants": variants,
            "gpl_components": manifest_components,
            "permissive_components": [
                {k: c[k] for k in ("formula", "keg_version", "licence", "variants", "archs")}
                for c in sorted(permissive.values(), key=lambda c: c["formula"])
            ],
            "in_tree_components": in_tree,
            "external_plugins": external,
            "unresolved_dylibs": unresolved,
        }
        (root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")

        lines = [
            "Amiberry third-party sources",
            "============================",
            "",
            f"Release tag : {tag or '(none; local build)'}",
            f"Amiberry   : {manifest['amiberry_version']} rev {manifest['amiberry_revision']['git'][:12]}",
            f"Variants   : {', '.join(variants)}",
            "",
            "This archive offers the exact corresponding source of every third-party",
            "GPL/LGPL library that the Apple binaries of this release were built",
            "against (GPLv2 S3 / GPLv3 S6 / LGPL S4). Each components/<formula>/",
            "entry is the upstream source tarball, checksum-verified against",
            "the exact homebrew-core formula file under homebrew-formulae/,",
            "with the patches Homebrew applied (external patch URLs and",
            "homebrew-core file patches, sha256-verified where the formula",
            "declares one) under patches/; inline DATA patches are in the .rb.",
            "",
            "Components whose source already ships in the Amiberry repository",
            "(external/) are covered by the tagged Amiberry source tree and are not",
            "duplicated here; see in_tree_components in manifest.json.",
            "",
            "Contents:",
        ]
        for c in manifest_components:
            lines.append(f"  {c['file']:<52} {c['formula']} {c['version']} [{c['licence']}] "
                         f"-> {', '.join(c['variants'])}")
        (root / "README.txt").write_text("\n".join(lines) + "\n")

        info(f"packing {out}")
        if out.exists():
            out.unlink()
        r = run(["tar", "-cJf", str(out.resolve()), "-C", str(workdir), root.name])
        if r.returncode != 0:
            die(f"tar failed: {r.stderr.strip()}")
        print(f"{out} ({out.stat().st_size} bytes)")
        info("manifest summary:")
        print(json.dumps({k: manifest[k] for k in
                          ("tag", "variants", "gpl_components", "in_tree_components",
                           "external_plugins", "unresolved_dylibs")}, indent=2))
    finally:
        if args.work_dir:
            print(f"work dir kept: {workdir}")
        else:
            shutil.rmtree(workdir, ignore_errors=True)


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = p.add_subparsers(dest="cmd", required=True)

    ps = sub.add_parser("scan", help="map a built .app bundle to Homebrew formula versions")
    ps.add_argument("--app", required=True, help="path to the built Amiberry.app")
    ps.add_argument("--variant", required=True, choices=("macos", "macos-store", "ios"))
    ps.add_argument("--arch", default="", help="x86_64 / arm64 (informational)")
    ps.add_argument("--out", required=True, help="output scan JSON")
    ps.add_argument("--brew", default="brew")
    ps.add_argument("--allow-unknown", action="store_true",
                    help="record unmapped dylibs instead of failing")
    ps.set_defaults(func=cmd_scan)

    pa = sub.add_parser("assemble", help="download verified sources and build the release archive")
    pa.add_argument("--scan", action="append", required=True, help="scan JSON (repeatable)")
    pa.add_argument("--tag", default="", help="release tag (archive name + manifest)")
    pa.add_argument("--out", default="", help="output .tar.xz (default from tag)")
    pa.add_argument("--brew", default="brew")
    pa.add_argument("--catalogue", default=str(SOURCE_ROOT_DEFAULT / "packaging/licences/catalogue.json"))
    pa.add_argument("--source-root", default=str(SOURCE_ROOT_DEFAULT),
                    help="Amiberry checkout (version/revision/catalogue)")
    pa.add_argument("--work-dir", default="", help="keep staging here instead of a temp dir")
    pa.set_defaults(func=cmd_assemble)

    args = p.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
