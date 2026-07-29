#!/usr/bin/env python3
"""Run the reviewed audit-policy patch implementation."""

from pathlib import Path


implementation = Path("scripts/code_review_fixes_audit_policy_patch_impl.py")
namespace = {"__name__": "__main__", "__file__": str(implementation)}
exec(compile(implementation.read_bytes(), str(implementation), "exec"), namespace)
