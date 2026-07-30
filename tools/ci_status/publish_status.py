#!/usr/bin/env python3
"""Build a bounded, ChatGPT-readable GitHub Actions status issue payload."""

from __future__ import annotations

import argparse
import copy
import json
import sys
from collections.abc import Iterable
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ABNORMAL_CONCLUSIONS = {
    "action_required",
    "cancelled",
    "failure",
    "startup_failure",
    "stale",
    "timed_out",
}
PENDING_STATUSES = {"queued", "pending", "waiting"}
ISSUE_BODY_MAX_BYTES = 60_000
LEGACY_MARKER = "<!-- maintained by publish-ci-status.yml -->"


class StatusDataError(ValueError):
    """Raised when GitHub API data does not match the expected shape."""


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise StatusDataError(f"{path}: invalid JSON: {error}") from error


def require_object(value: Any, context: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise StatusDataError(f"{context}: expected an object")
    return value


def require_array(value: Any, context: str) -> list[Any]:
    if not isinstance(value, list):
        raise StatusDataError(f"{context}: expected an array")
    return value


def require_string(value: Any, context: str, *, allow_empty: bool = False) -> str:
    if not isinstance(value, str) or (not allow_empty and value == ""):
        raise StatusDataError(f"{context}: expected a non-empty string")
    return value


def require_optional_string(value: Any, context: str) -> str | None:
    if value is None:
        return None
    return require_string(value, context, allow_empty=True)


def require_int(value: Any, context: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool):
        raise StatusDataError(f"{context}: expected an integer")
    return value


def page_objects(value: Any, context: str) -> list[dict[str, Any]]:
    if isinstance(value, dict):
        return [value]
    pages = require_array(value, context)
    return [
        require_object(page, f"{context}[{index}]")
        for index, page in enumerate(pages)
    ]


def flatten_collection_pages(
    value: Any, collection_key: str, context: str
) -> list[dict[str, Any]]:
    items: list[dict[str, Any]] = []
    for page_index, page in enumerate(page_objects(value, context)):
        raw_items = require_array(
            page.get(collection_key),
            f"{context}[{page_index}].{collection_key}",
        )
        for item_index, item in enumerate(raw_items):
            items.append(
                require_object(
                    item,
                    f"{context}[{page_index}].{collection_key}[{item_index}]",
                )
            )
    return items


def latest_applicable_run_id(
    pages: Any,
    *,
    repository: str,
    branch: str,
    accepted_events: set[str],
) -> int:
    runs = flatten_collection_pages(pages, "workflow_runs", "workflow run pages")
    for index, run in enumerate(runs):
        head_repository = require_object(
            run.get("head_repository"), f"workflow_runs[{index}].head_repository"
        )
        full_name = require_string(
            head_repository.get("full_name"),
            f"workflow_runs[{index}].head_repository.full_name",
        )
        head_branch = require_optional_string(
            run.get("head_branch"), f"workflow_runs[{index}].head_branch"
        )
        event = require_string(run.get("event"), f"workflow_runs[{index}].event")
        if (
            full_name == repository
            and head_branch == branch
            and event in accepted_events
        ):
            return require_int(run.get("id"), f"workflow_runs[{index}].id")
    raise StatusDataError(
        "no trusted workflow run matched the monitored repository, branch, and event"
    )


def validate_issue_owner(
    issue: Any,
    *,
    issue_number: int,
    expected_marker: str,
    accepted_titles: set[str],
) -> None:
    issue_object = require_object(issue, "issue")
    actual_number = require_int(issue_object.get("number"), "issue.number")
    if actual_number != issue_number:
        raise StatusDataError(
            f"issue number mismatch: expected {issue_number}, got {actual_number}"
        )
    state = require_string(issue_object.get("state"), "issue.state")
    if state != "open":
        raise StatusDataError(f"status issue #{issue_number} is not open")
    title = require_string(issue_object.get("title"), "issue.title")
    if title not in accepted_titles:
        raise StatusDataError(
            f"status issue #{issue_number} has unexpected title {title!r}"
        )
    body = require_string(issue_object.get("body"), "issue.body", allow_empty=True)
    if not (body.startswith(expected_marker) or body.startswith(LEGACY_MARKER)):
        raise StatusDataError(
            f"status issue #{issue_number} is missing its ownership marker"
        )


def normalize_step(raw: dict[str, Any], context: str) -> dict[str, Any]:
    return {
        "number": require_int(raw.get("number"), f"{context}.number"),
        "name": require_string(raw.get("name"), f"{context}.name"),
        "status": require_string(raw.get("status"), f"{context}.status"),
        "conclusion": require_optional_string(
            raw.get("conclusion"), f"{context}.conclusion"
        ),
        "started_at": require_optional_string(
            raw.get("started_at"), f"{context}.started_at"
        ),
        "completed_at": require_optional_string(
            raw.get("completed_at"), f"{context}.completed_at"
        ),
    }


def normalize_job(raw: dict[str, Any], context: str) -> dict[str, Any]:
    raw_steps = require_array(raw.get("steps", []), f"{context}.steps")
    steps = [
        normalize_step(
            require_object(step, f"{context}.steps[{index}]"),
            f"{context}.steps[{index}]",
        )
        for index, step in enumerate(raw_steps)
    ]
    return {
        "id": require_int(raw.get("id"), f"{context}.id"),
        "name": require_string(raw.get("name"), f"{context}.name"),
        "status": require_string(raw.get("status"), f"{context}.status"),
        "conclusion": require_optional_string(
            raw.get("conclusion"), f"{context}.conclusion"
        ),
        "started_at": require_optional_string(
            raw.get("started_at"), f"{context}.started_at"
        ),
        "completed_at": require_optional_string(
            raw.get("completed_at"), f"{context}.completed_at"
        ),
        "runner_name": require_optional_string(
            raw.get("runner_name"), f"{context}.runner_name"
        ),
        "runner_group_name": require_optional_string(
            raw.get("runner_group_name"), f"{context}.runner_group_name"
        ),
        "steps": steps,
    }


def normalize_jobs(pages: Any) -> list[dict[str, Any]]:
    raw_jobs = flatten_collection_pages(pages, "jobs", "job pages")
    return [
        normalize_job(job, f"jobs[{index}]")
        for index, job in enumerate(raw_jobs)
    ]


def _raise(message: str) -> Any:
    raise StatusDataError(message)


def normalize_artifact(raw: dict[str, Any], context: str) -> dict[str, Any]:
    return {
        "id": require_int(raw.get("id"), f"{context}.id"),
        "name": require_string(raw.get("name"), f"{context}.name"),
        "size_in_bytes": require_int(
            raw.get("size_in_bytes"), f"{context}.size_in_bytes"
        ),
        "expired": raw.get("expired")
        if isinstance(raw.get("expired"), bool)
        else _raise(f"{context}.expired: expected a boolean"),
        "created_at": require_string(
            raw.get("created_at"), f"{context}.created_at"
        ),
        "expires_at": require_optional_string(
            raw.get("expires_at"), f"{context}.expires_at"
        ),
    }


def normalize_artifacts(pages: Any) -> list[dict[str, Any]]:
    if pages == []:
        return []
    raw_artifacts = flatten_collection_pages(
        pages, "artifacts", "artifact pages"
    )
    return [
        normalize_artifact(artifact, f"artifacts[{index}]")
        for index, artifact in enumerate(raw_artifacts)
    ]


def collect_problem_steps(jobs: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    problems: list[dict[str, Any]] = []
    for job in jobs:
        for step in job["steps"]:
            if step["conclusion"] in ABNORMAL_CONCLUSIONS:
                problems.append(
                    {
                        "job_id": job["id"],
                        "job": job["name"],
                        "step_number": step["number"],
                        "step": step["name"],
                        "conclusion": step["conclusion"],
                    }
                )
    return problems


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def code_span(value: Any) -> str:
    text = str(value).replace("\r", " ").replace("\n", " ")
    longest = 0
    current = 0
    for character in text:
        if character == "`":
            current += 1
            longest = max(longest, current)
        else:
            current = 0
    fence = "`" * max(1, longest + 1)
    padding = " " if text.startswith("`") or text.endswith("`") else ""
    return f"{fence}{padding}{text}{padding}{fence}"


def json_fence(document: str) -> str:
    longest = 0
    current = 0
    for character in document:
        if character == "`":
            current += 1
            longest = max(longest, current)
        else:
            current = 0
    return "`" * max(3, longest + 1)


def job_counts(jobs: list[dict[str, Any]]) -> dict[str, int]:
    counts = {
        "total": len(jobs),
        "completed": 0,
        "failed_or_abnormal": 0,
        "running": 0,
        "pending": 0,
    }
    for job in jobs:
        if job["status"] == "completed":
            counts["completed"] += 1
        elif job["status"] == "in_progress":
            counts["running"] += 1
        elif job["status"] in PENDING_STATUSES:
            counts["pending"] += 1
        if job["conclusion"] in ABNORMAL_CONCLUSIONS:
            counts["failed_or_abnormal"] += 1
    return counts


def render_body(payload: dict[str, Any], heading: str, marker: str) -> str:
    workflow = payload["workflow"]
    counts = payload["job_summary"]
    problems = payload["problem_steps"]
    problem_lines = ["- **Problem steps:** None"]
    if problems:
        problem_lines = ["- **Problem steps:**"]
        problem_lines.extend(
            "  - "
            + code_span(item["job"])
            + " / "
            + code_span(item["step"])
            + " — "
            + code_span(item["conclusion"])
            for item in problems
        )
    document = json.dumps(payload, indent=2, sort_keys=True, ensure_ascii=False)
    fence = json_fence(document)
    lines = [
        marker,
        f"# {heading}",
        "",
        f"- **Status:** {code_span(workflow['status'])}",
        f"- **Conclusion:** {code_span(workflow['conclusion'] or 'pending')}",
        f"- **Run:** [{workflow['run_id']}]({workflow['run_url']})",
        f"- **Attempt:** {code_span(workflow['run_attempt'])}",
        f"- **Commit:** {code_span(workflow['head_sha'])}",
        (
            "- **Branch/event:** "
            f"{code_span(workflow['head_branch'])} / {code_span(workflow['event'])}"
        ),
        (
            "- **Jobs:** "
            f"{code_span(counts['completed'])} completed, "
            f"{code_span(counts['failed_or_abnormal'])} failed or abnormal, "
            f"{code_span(counts['running'])} running, "
            f"{code_span(counts['pending'])} pending"
        ),
        *problem_lines,
        f"- **Artifacts:** {code_span(len(payload['artifacts']))}",
        f"- **Job metadata:** {code_span(payload['job_data_state'])}",
        f"- **Artifact metadata:** {code_span(payload['artifact_data_state'])}",
        f"- **Observed:** {code_span(payload['publisher']['observed_at'])}",
        "",
        "## Machine-readable status",
        "",
        f"{fence}json",
        document,
        fence,
        "",
        (
            "This issue is overwritten whenever the latest applicable run changes "
            "state. It is not a historical log."
        ),
    ]
    return "\n".join(lines) + "\n"


def body_size(body: str) -> int:
    return len(body.encode("utf-8"))


def compact_payload(
    payload: dict[str, Any], heading: str, marker: str
) -> tuple[dict[str, Any], str]:
    candidate = copy.deepcopy(payload)
    body = render_body(candidate, heading, marker)
    if body_size(body) <= ISSUE_BODY_MAX_BYTES:
        return candidate, body

    candidate["details_compacted"] = True
    candidate["compaction_reason"] = "issue_body_size_limit"
    for job in candidate["jobs"]:
        if (
            job["conclusion"] == "success"
            and not any(
                step["conclusion"] in ABNORMAL_CONCLUSIONS
                for step in job["steps"]
            )
        ):
            job["steps"] = []
            job["steps_compacted"] = True
    body = render_body(candidate, heading, marker)
    if body_size(body) <= ISSUE_BODY_MAX_BYTES:
        return candidate, body

    for job in candidate["jobs"]:
        abnormal_steps = [
            step
            for step in job["steps"]
            if step["conclusion"] in ABNORMAL_CONCLUSIONS
        ]
        if len(abnormal_steps) != len(job["steps"]):
            job["steps"] = abnormal_steps
            job["steps_compacted"] = True
            job["problem_steps_preserved"] = True
    body = render_body(candidate, heading, marker)
    if body_size(body) <= ISSUE_BODY_MAX_BYTES:
        return candidate, body

    raise StatusDataError(
        "status issue body exceeds the bounded size after safe compaction"
    )


def build_payload(
    *,
    jobs_pages: Any,
    artifacts_pages: Any,
    workflow_file: str,
    issue_number: int,
    monitored_branch: str,
    workflow_name: str,
    workflow_id: int,
    run_id: int,
    run_attempt: int,
    run_url: str,
    event: str,
    status: str,
    conclusion: str | None,
    head_repository: str,
    head_branch: str,
    head_sha: str,
    created_at: str,
    updated_at: str,
) -> dict[str, Any]:
    jobs = normalize_jobs(jobs_pages)
    artifacts = normalize_artifacts(artifacts_pages)
    problems = collect_problem_steps(jobs)
    if not jobs and status != "completed":
        job_data_state = "not_yet_available"
    elif not jobs:
        job_data_state = "unavailable_after_completion"
    else:
        job_data_state = "available"

    if status != "completed":
        artifact_data_state = "not_requested_until_completion"
    elif artifacts:
        artifact_data_state = "available"
    else:
        artifact_data_state = "available_empty"

    return {
        "schema_version": 1,
        "publisher": {
            "workflow_file": workflow_file,
            "issue_number": issue_number,
            "monitored_branch": monitored_branch,
            "observed_at": utc_now(),
            "issue_body_max_bytes": ISSUE_BODY_MAX_BYTES,
        },
        "workflow": {
            "name": workflow_name,
            "workflow_id": workflow_id,
            "run_id": run_id,
            "run_attempt": run_attempt,
            "run_url": run_url,
            "event": event,
            "status": status,
            "conclusion": conclusion,
            "head_repository": head_repository,
            "head_branch": head_branch,
            "head_sha": head_sha,
            "created_at": created_at,
            "updated_at": updated_at,
        },
        "job_data_state": job_data_state,
        "artifact_data_state": artifact_data_state,
        "details_compacted": False,
        "compaction_reason": None,
        "job_summary": job_counts(jobs),
        "problem_steps": problems,
        "jobs": jobs,
        "artifacts": artifacts,
    }


def command_latest_run(args: argparse.Namespace) -> int:
    pages = load_json(args.runs)
    accepted_events = {
        event.strip() for event in args.accepted_events.split(",") if event.strip()
    }
    if not accepted_events:
        raise StatusDataError("accepted event set must not be empty")
    print(
        latest_applicable_run_id(
            pages,
            repository=args.repository,
            branch=args.branch,
            accepted_events=accepted_events,
        )
    )
    return 0


def command_validate_issue(args: argparse.Namespace) -> int:
    issue = load_json(args.issue)
    validate_issue_owner(
        issue,
        issue_number=args.issue_number,
        expected_marker=args.marker,
        accepted_titles=set(args.accepted_title),
    )
    return 0


def command_build(args: argparse.Namespace) -> int:
    jobs_pages = load_json(args.jobs)
    artifacts_pages = load_json(args.artifacts)
    payload = build_payload(
        jobs_pages=jobs_pages,
        artifacts_pages=artifacts_pages,
        workflow_file=args.workflow_file,
        issue_number=args.issue_number,
        monitored_branch=args.monitored_branch,
        workflow_name=args.workflow_name,
        workflow_id=args.workflow_id,
        run_id=args.run_id,
        run_attempt=args.run_attempt,
        run_url=args.run_url,
        event=args.event,
        status=args.status,
        conclusion=args.conclusion or None,
        head_repository=args.head_repository,
        head_branch=args.head_branch,
        head_sha=args.head_sha,
        created_at=args.created_at,
        updated_at=args.updated_at,
    )
    compacted_payload, body = compact_payload(payload, args.heading, args.marker)
    patch = {
        "title": args.issue_title,
        "body": body,
        "state": "open",
    }
    args.body.write_text(body, encoding="utf-8")
    args.patch.write_text(
        json.dumps(patch, ensure_ascii=False, sort_keys=True),
        encoding="utf-8",
    )
    if body_size(body) > ISSUE_BODY_MAX_BYTES:
        raise StatusDataError("generated issue body exceeds configured maximum")
    if compacted_payload["details_compacted"]:
        print("generated compacted status issue payload")
    else:
        print("generated complete status issue payload")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    latest = subparsers.add_parser("latest-run")
    latest.add_argument("--runs", type=Path, required=True)
    latest.add_argument("--repository", required=True)
    latest.add_argument("--branch", required=True)
    latest.add_argument("--accepted-events", default="push,workflow_dispatch")
    latest.set_defaults(handler=command_latest_run)

    validate = subparsers.add_parser("validate-issue")
    validate.add_argument("--issue", type=Path, required=True)
    validate.add_argument("--issue-number", type=int, required=True)
    validate.add_argument("--marker", required=True)
    validate.add_argument("--accepted-title", action="append", required=True)
    validate.set_defaults(handler=command_validate_issue)

    build = subparsers.add_parser("build")
    build.add_argument("--jobs", type=Path, required=True)
    build.add_argument("--artifacts", type=Path, required=True)
    build.add_argument("--body", type=Path, required=True)
    build.add_argument("--patch", type=Path, required=True)
    build.add_argument("--workflow-file", required=True)
    build.add_argument("--issue-number", type=int, required=True)
    build.add_argument("--issue-title", required=True)
    build.add_argument("--heading", required=True)
    build.add_argument("--marker", required=True)
    build.add_argument("--monitored-branch", required=True)
    build.add_argument("--workflow-name", required=True)
    build.add_argument("--workflow-id", type=int, required=True)
    build.add_argument("--run-id", type=int, required=True)
    build.add_argument("--run-attempt", type=int, required=True)
    build.add_argument("--run-url", required=True)
    build.add_argument("--event", required=True)
    build.add_argument("--status", required=True)
    build.add_argument("--conclusion", default="")
    build.add_argument("--head-repository", required=True)
    build.add_argument("--head-branch", required=True)
    build.add_argument("--head-sha", required=True)
    build.add_argument("--created-at", required=True)
    build.add_argument("--updated-at", required=True)
    build.set_defaults(handler=command_build)
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    try:
        return int(args.handler(args))
    except StatusDataError as error:
        print(f"CI status publisher failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
