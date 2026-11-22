"""Helpers for creating user-data-safe output workspaces."""

import os
import shutil
from pathlib import PureWindowsPath


def create_new_workspace(parent_dir: str, workspace_name: str) -> str:
    """Create a fresh child directory without overwriting existing user data."""
    if (
        not workspace_name
        or workspace_name in {".", ".."}
        or os.path.isabs(workspace_name)
        or PureWindowsPath(workspace_name).is_absolute()
        or bool(PureWindowsPath(workspace_name).drive)
        or "/" in workspace_name
        or chr(92) in workspace_name
        or os.path.basename(workspace_name) != workspace_name
    ):
        raise ValueError("Workspace name must be a single, non-empty path component.")

    parent = os.path.realpath(parent_dir)
    os.makedirs(parent, exist_ok=True)
    target = os.path.join(parent, workspace_name)
    os.mkdir(target)
    return target



def publish_file_no_overwrite(source_path: str, destination_path: str) -> str:
    """Publish a generated file without replacing or deleting unrelated files."""
    source = os.path.abspath(source_path)
    destination = os.path.abspath(destination_path)
    if source == destination or paths_are_same(source, destination):
        raise ValueError("Source and destination must be different paths.")
    if not os.path.isfile(source) or os.path.islink(source):
        raise FileNotFoundError(f"Generated source file is missing or not regular: {source}")
    parent = os.path.dirname(destination)
    if parent:
        os.makedirs(parent, exist_ok=True)

    with open(source, "rb") as input_file:
        source_identity = os.fstat(input_file.fileno())
        path_identity = os.stat(source, follow_symlinks=False)
        if (source_identity.st_dev, source_identity.st_ino) != (
            path_identity.st_dev, path_identity.st_ino
        ) or not os.path.isfile(source):
            raise RuntimeError("Generated source changed while being opened.")

        descriptor = os.open(destination, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o666)
        destination_identity = os.fstat(descriptor)
        try:
            with os.fdopen(descriptor, "wb") as output:
                shutil.copyfileobj(input_file, output)
                output.flush()
                os.fsync(output.fileno())
        except Exception:
            # Keep a possibly partial destination rather than risk a pathname
            # TOCTOU race deleting a replacement created by another process.
            raise

    # Keep the generated source as a recoverable intermediate. Deleting by
    # pathname after a stat check has an unavoidable TOCTOU race on some
    # filesystems; callers can clean their own private workspace explicitly.
    return destination


def ensure_paths_absent(paths) -> None:
    """Fail before a pipeline starts if any generated output already exists."""
    for path in paths:
        if os.path.lexists(path):
            raise FileExistsError(
                f"Refusing to overwrite existing generated output: {path}"
            )


def paths_are_same(path_a: str, path_b: str) -> bool:
    """Compare paths after resolving relative segments and platform case rules."""
    return os.path.normcase(os.path.realpath(os.path.abspath(path_a))) == os.path.normcase(
        os.path.realpath(os.path.abspath(path_b))
    )


def create_next_workspace(parent_dir: str, workspace_name: str) -> str:
    """Create a new run workspace, choosing a suffix if previous runs exist.

    Existing workspaces are never reused or overwritten. mkdir provides the
    atomic collision check when concurrent batch workers start.
    """
    for attempt in range(1, 10001):
        candidate = (
            workspace_name
            if attempt == 1
            else f"{workspace_name}_attempt_{attempt}"
        )
        try:
            return create_new_workspace(parent_dir, candidate)
        except FileExistsError:
            continue
    raise FileExistsError(
        f"Could not allocate a unique workspace for {workspace_name!r}."
    )
