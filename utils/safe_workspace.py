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
    """Publish a generated file without ever replacing an existing destination."""
    source = os.path.abspath(source_path)
    destination = os.path.abspath(destination_path)
    if source == destination:
        raise ValueError("Source and destination must be different paths.")
    if not os.path.isfile(source) or os.path.islink(source):
        raise FileNotFoundError(f"Generated source file is missing or not regular: {source}")
    parent = os.path.dirname(destination)
    if parent:
        os.makedirs(parent, exist_ok=True)

    descriptor = os.open(destination, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o666)
    try:
        with os.fdopen(descriptor, "wb") as output, open(source, "rb") as input_file:
            shutil.copyfileobj(input_file, output)
            output.flush()
            os.fsync(output.fileno())
    except Exception:
        try:
            os.unlink(destination)
        except OSError:
            pass
        raise

    os.unlink(source)
    return destination
