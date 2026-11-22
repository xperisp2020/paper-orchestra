"""Helpers for creating user-data-safe output workspaces."""

import os


def create_new_workspace(parent_dir: str, workspace_name: str) -> str:
    """Create a fresh child directory without overwriting existing user data."""
    if (
        not workspace_name
        or workspace_name in {".", ".."}
        or os.path.isabs(workspace_name)
        or os.path.basename(workspace_name) != workspace_name
    ):
        raise ValueError("Workspace name must be a single, non-empty path component.")

    parent = os.path.realpath(parent_dir)
    os.makedirs(parent, exist_ok=True)
    target = os.path.join(parent, workspace_name)
    os.mkdir(target)
    return target
