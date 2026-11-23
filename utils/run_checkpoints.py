"""Integrity-checked, attempt-local checkpoints for resumable research runs.

The store deliberately fails closed: corrupt state, stale locks, symlinks, and
artifacts outside the attempt directory are never silently repaired or reused.
"""
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import uuid
import importlib.metadata
from pathlib import PureWindowsPath
from contextlib import contextmanager


SCHEMA_VERSION = 1
DEFAULT_DEPENDENCIES = {
    "prepare_inputs": (),
    "outline": ("prepare_inputs",),
    "literature": ("outline",),
    "plotting": ("outline",),
    "draft": ("literature",),
    "refinement": ("draft",),
}
STATE_FILENAME = ".run_checkpoints.json"
LOCK_FILENAME = ".run_checkpoints.lock"


def _sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def fingerprint_path(path):
    """Return a deterministic SHA-256 for one regular file or a directory tree."""
    path = os.path.abspath(os.fspath(path))
    if os.path.islink(path):
        raise ValueError("Fingerprint source must not be a symlink.")
    if os.path.isfile(path):
        return _sha256_file(path)
    if not os.path.isdir(path):
        raise FileNotFoundError("Fingerprint source is missing or not regular.")
    digest = hashlib.sha256()
    for root, dirs, files in os.walk(path, topdown=True, followlinks=False):
        dirs.sort()
        files.sort()
        for name in list(dirs):
            candidate = os.path.join(root, name)
            if os.path.islink(candidate) or not os.path.isdir(candidate):
                raise ValueError("Fingerprint tree contains a symlink or non-directory.")
            relative_dir = os.path.relpath(candidate, path).replace(os.sep, "/")
            digest.update(b"D\\0" + relative_dir.encode("utf-8") + b"\\0")
        for name in files:
            candidate = os.path.join(root, name)
            if os.path.islink(candidate) or not os.path.isfile(candidate):
                raise ValueError("Fingerprint tree contains a symlink or non-regular file.")
            relative = os.path.relpath(candidate, path).replace(os.sep, "/")
            digest.update(relative.encode("utf-8"))
            digest.update(b"\0")
            digest.update(bytes.fromhex(_sha256_file(candidate)))
    return digest.hexdigest()


def fingerprint_source_tree(path):
    """Fingerprint source/resource files while ignoring interpreter caches."""
    path = os.path.abspath(os.fspath(path))
    if os.path.islink(path) or not os.path.isdir(path):
        raise ValueError("Source tree must be an existing, non-symlink directory.")

    included_suffixes = {
        ".py", ".md", ".txt", ".json", ".yaml", ".yml", ".toml",
        ".tex", ".bib", ".csv", ".jinja", ".jinja2",
    }
    excluded_directories = {
        "__pycache__", ".git", ".venv", "venv", ".pytest_cache",
        "build", "dist", "node_modules",
    }
    digest = hashlib.sha256()
    for root, dirs, files in os.walk(path, topdown=True, followlinks=False):
        for directory in list(dirs):
            candidate = os.path.join(root, directory)
            if os.path.islink(candidate):
                raise ValueError("Source tree contains a symlink directory.")
            if directory in excluded_directories:
                dirs.remove(directory)
        dirs.sort()
        for name in sorted(files):
            candidate = os.path.join(root, name)
            suffix = os.path.splitext(name)[1].lower()
            if suffix not in included_suffixes:
                continue
            if os.path.islink(candidate) or not os.path.isfile(candidate):
                raise ValueError("Source tree contains a symlink or non-regular source file.")
            relative = os.path.relpath(candidate, path).replace(os.sep, "/")
            digest.update(b"F\\0" + relative.encode("utf-8") + b"\\0")
            digest.update(bytes.fromhex(_sha256_file(candidate)))
    return digest.hexdigest()


def fingerprint_runtime():
    """Fingerprint Python packages and the external LaTeX executables."""
    distributions = []
    for distribution in importlib.metadata.distributions():
        name = distribution.metadata.get("Name")
        if name:
            distributions.append(name.strip().lower().replace("_", "-") + "==" + distribution.version)
    package_digest = hashlib.sha256(
        "\\n".join(sorted(distributions)).encode("utf-8")
    ).hexdigest()

    latex_tools = {}
    for command in ("pdflatex", "bibtex"):
        executable = shutil.which(command)
        if not executable:
            latex_tools[command] = None
            continue
        try:
            completed = subprocess.run(
                [executable, "--version"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=15,
                check=False,
            )
            lines = (completed.stdout or "").splitlines()
            version = lines[0].strip() if lines else "exit:" + str(completed.returncode)
        except (OSError, subprocess.SubprocessError) as exc:
            version = "unavailable:" + type(exc).__name__
        latex_tools[command] = {
            "version": version,
            "binary_sha256": fingerprint_path(executable),
        }

    return {
        "python_version": platform.python_version(),
        "python_implementation": platform.python_implementation(),
        "platform": sys.platform,
        "installed_distributions_sha256": package_digest,
        "installed_distribution_count": len(distributions),
        "latex_tools": latex_tools,
    }


def fingerprint_inputs(inputs):
    """Hash a JSON-compatible description of input fingerprints/settings."""
    encoded = json.dumps(
        inputs, sort_keys=True, separators=(",", ":"), ensure_ascii=False
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


class CheckpointStore:
    """Atomic checkpoint manifest scoped to exactly one attempt directory."""

    def __init__(self, attempt_dir, input_fingerprint, dependencies=None):
        self.root = os.path.abspath(os.fspath(attempt_dir))
        if os.path.islink(self.root) or not os.path.isdir(self.root):
            raise ValueError("Attempt root must be an existing, non-symlink directory.")
        self.root = os.path.realpath(self.root)
        if not isinstance(input_fingerprint, str) or len(input_fingerprint) != 64:
            raise ValueError("input_fingerprint must be a SHA-256 hex digest.")
        try:
            bytes.fromhex(input_fingerprint)
        except ValueError as exc:
            raise ValueError("input_fingerprint must be a SHA-256 hex digest.") from exc
        self.input_fingerprint = input_fingerprint
        self.dependencies = dict(dependencies or DEFAULT_DEPENDENCIES)
        self._validate_graph()
        self.state_path = os.path.join(self.root, STATE_FILENAME)
        self.lock_path = os.path.join(self.root, LOCK_FILENAME)

    def _validate_graph(self):
        for stage, parents in self.dependencies.items():
            if not isinstance(stage, str) or not stage or not isinstance(parents, (tuple, list)):
                raise ValueError("Invalid checkpoint dependency graph.")
            if stage in parents or any(parent not in self.dependencies for parent in parents):
                raise ValueError("Invalid checkpoint dependency graph.")
        visiting = set()
        visited = set()

        def visit(stage):
            if stage in visiting:
                raise ValueError("Checkpoint dependency graph contains a cycle.")
            if stage in visited:
                return
            visiting.add(stage)
            for parent in self.dependencies[stage]:
                visit(parent)
            visiting.remove(stage)
            visited.add(stage)

        for stage in self.dependencies:
            visit(stage)

    @contextmanager
    def _lock(self):
        current_thread = threading.get_ident()
        if getattr(self, "_lock_owner", None) == current_thread:
            yield
            return
        descriptor = None
        try:
            descriptor = os.open(
                self.lock_path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600
            )
        except FileExistsError as exc:
            raise RuntimeError(
                "Checkpoint is locked; inspect the existing lock before recovery."
            ) from exc
        identity = os.fstat(descriptor)
        self._lock_owner = current_thread
        try:
            payload = json.dumps(
                {"pid": os.getpid(), "created": time.time(), "token": uuid.uuid4().hex}
            ).encode("utf-8")
            os.write(descriptor, payload)
            os.fsync(descriptor)
            yield
        finally:
            self._lock_owner = None
            if descriptor is not None:
                os.close(descriptor)
            try:
                current = os.stat(self.lock_path, follow_symlinks=False)
                if (current.st_dev, current.st_ino) == (identity.st_dev, identity.st_ino):
                    os.unlink(self.lock_path)
            except FileNotFoundError:
                pass

    def run_lock(self):
        """Hold the attempt lock across a complete multi-stage pipeline run."""
        return self._lock()

    def _artifact_path(self, relative_path):
        if not isinstance(relative_path, str) or not relative_path:
            raise ValueError("Artifact path must be a non-empty relative path.")
        windows_path = PureWindowsPath(relative_path)
        if (
            os.path.isabs(relative_path)
            or windows_path.is_absolute()
            or windows_path.drive
            or "\\" in relative_path
        ):
            raise ValueError("Artifact path must be relative to the attempt directory.")
        parts = relative_path.split("/")
        if any(part in ("", ".", "..") for part in parts):
            raise ValueError("Artifact path contains an unsafe component.")
        candidate = os.path.join(self.root, *parts)
        current = self.root
        for part in parts:
            current = os.path.join(current, part)
            if os.path.islink(current):
                raise ValueError("Checkpoint artifacts must not traverse symlinks.")
        resolved = os.path.realpath(candidate)
        if os.path.commonpath((self.root, resolved)) != self.root:
            raise ValueError("Artifact path escapes the attempt directory.")
        if not os.path.isfile(candidate):
            raise FileNotFoundError("Checkpoint artifact is missing or not a regular file.")
        return candidate

    def _load(self):
        if not os.path.lexists(self.state_path):
            return {"schema": SCHEMA_VERSION, "input_fingerprint": self.input_fingerprint, "stages": {}}
        if os.path.islink(self.state_path) or not os.path.isfile(self.state_path):
            raise ValueError("Checkpoint state must be a regular file.")
        with open(self.state_path, "r", encoding="utf-8") as stream:
            state = json.load(stream)
        if (
            not isinstance(state, dict)
            or state.get("schema") != SCHEMA_VERSION
            or not isinstance(state.get("stages"), dict)
        ):
            raise ValueError("Checkpoint state schema is invalid.")
        if state.get("input_fingerprint") != self.input_fingerprint:
            return {"schema": SCHEMA_VERSION, "input_fingerprint": self.input_fingerprint, "stages": {}}
        return state

    def _save(self, state):
        descriptor, temporary = tempfile.mkstemp(
            prefix=STATE_FILENAME + ".tmp-", dir=self.root
        )
        try:
            with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
                json.dump(state, stream, sort_keys=True, ensure_ascii=False, indent=2)
                stream.write("\n")
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary, self.state_path)
            if os.name != "nt":
                directory_fd = os.open(self.root, os.O_RDONLY)
                try:
                    os.fsync(directory_fd)
                finally:
                    os.close(directory_fd)
        except Exception:
            try:
                os.unlink(temporary)
            except FileNotFoundError:
                pass
            raise

    def _stage_valid(self, state, stage, visiting=None):
        if stage not in self.dependencies:
            raise KeyError("Unknown checkpoint stage: " + str(stage))
        visiting = set() if visiting is None else visiting
        if stage in visiting:
            return False
        visiting.add(stage)
        for parent in self.dependencies[stage]:
            if not self._stage_valid(state, parent, visiting):
                visiting.remove(stage)
                return False
        entry = state["stages"].get(stage)
        if not isinstance(entry, dict) or not isinstance(entry.get("artifacts"), dict):
            visiting.remove(stage)
            return False
        try:
            for relative, expected_hash in entry["artifacts"].items():
                path = self._artifact_path(relative)
                if not isinstance(expected_hash, str) or _sha256_file(path) != expected_hash:
                    visiting.remove(stage)
                    return False
        except (OSError, ValueError):
            visiting.remove(stage)
            return False
        visiting.remove(stage)
        return True

    def _invalidate_downstream(self, state, stage):
        invalid = {stage}
        changed = True
        while changed:
            changed = False
            for candidate, parents in self.dependencies.items():
                if candidate not in invalid and any(parent in invalid for parent in parents):
                    invalid.add(candidate)
                    changed = True
        for candidate in invalid:
            state["stages"].pop(candidate, None)

    def has_matching_state(self):
        """Whether this attempt contains checkpoint state for the current inputs."""
        with self._lock():
            if not os.path.lexists(self.state_path):
                return False
            state = self._load()
            return bool(state["stages"])

    def is_stage_valid(self, stage):
        with self._lock():
            state = self._load()
            valid = self._stage_valid(state, stage)
            if not valid and stage in state["stages"]:
                self._invalidate_downstream(state, stage)
                self._save(state)
            return valid

    def record_stage(self, stage, artifacts):
        """Record only completed stages whose dependencies and files verify."""
        if stage not in self.dependencies:
            raise KeyError("Unknown checkpoint stage: " + str(stage))
        if not isinstance(artifacts, (tuple, list)) or not artifacts:
            raise ValueError("A completed stage must declare at least one artifact.")
        with self._lock():
            state = self._load()
            for parent in self.dependencies[stage]:
                if not self._stage_valid(state, parent):
                    raise RuntimeError("Cannot checkpoint before dependencies are valid.")
            hashes = {}
            for relative in artifacts:
                path = self._artifact_path(relative)
                hashes[relative] = _sha256_file(path)
            self._invalidate_downstream(state, stage)
            state["stages"][stage] = {"artifacts": hashes, "completed_at": time.time()}
            self._save(state)

    def valid_stages(self):
        return [stage for stage in self.dependencies if self.is_stage_valid(stage)]



def create_or_resume_attempt(base_dir, input_fingerprint, resume=False, dependencies=None, create_if_missing=True):
    """Reuse the newest matching attempt only when resume is explicitly requested.

    Invalid/corrupt attempts are preserved for diagnosis and skipped. A live
    lock is never bypassed; it aborts selection instead of starting a duplicate.
    """
    os.makedirs(base_dir, exist_ok=True)
    if resume:
        candidates = []
        with os.scandir(base_dir) as entries:
            for entry in entries:
                suffix = entry.name[len("attempt_"):] if entry.name.startswith("attempt_") else ""
                if suffix.isdigit() and entry.is_dir(follow_symlinks=False):
                    candidates.append((int(suffix), entry.path))
        for _, attempt_dir in sorted(candidates, reverse=True):
            try:
                store = CheckpointStore(
                    attempt_dir, input_fingerprint, dependencies=dependencies
                )
                if store.has_matching_state():
                    return attempt_dir, store, True
            except (ValueError, json.JSONDecodeError):
                # Never overwrite or repair a corrupt historical attempt.
                continue

    if not create_if_missing:
        return None, None, False

    for number in range(1, 10001):
        attempt_name = "attempt_" + str(number)
        try:
            from utils.safe_workspace import create_new_workspace
            attempt_dir = create_new_workspace(base_dir, attempt_name)
        except FileExistsError:
            continue
        store = CheckpointStore(
            attempt_dir, input_fingerprint, dependencies=dependencies
        )
        return attempt_dir, store, False
    raise FileExistsError("Could not allocate a unique attempt workspace.")
