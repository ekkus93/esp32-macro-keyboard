#!/usr/bin/env python3
"""Regression tests for the ChatGPT-readable CI status payload generator."""

from __future__ import annotations

import copy
import importlib.util
import json
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).with_name("publish_status.py")
SPEC = importlib.util.spec_from_file_location("publish_status", MODULE_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("could not load publish_status.py")
publish_status = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(publish_status)

REPOSITORY = "owner/repository"
BRANCH = "master"
MARKER = (
    "<!-- maintained by .github/workflows/publish-ci-status.yml; "
    "workflow=Quality; branch=master; issue=19 -->"
)


def step(number: int, name: str, conclusion: str = "success") -> dict[str, object]:
    return {
        "number": number,
        "name": name,
        "status": "completed",
        "conclusion": conclusion,
        "started_at": "2026-07-30T00:00:00Z",
        "completed_at": "2026-07-30T00:00:01Z",
    }


def job(
    job_id: int,
    name: str,
    conclusion: str = "success",
    steps: list[dict[str, object]] | None = None,
) -> dict[str, object]:
    return {
        "id": job_id,
        "name": name,
        "status": "completed",
        "conclusion": conclusion,
        "started_at": "2026-07-30T00:00:00Z",
        "completed_at": "2026-07-30T00:01:00Z",
        "runner_name": "GitHub Actions 1",
        "runner_group_name": "GitHub Actions",
        "steps": steps if steps is not None else [step(1, "Run")],
    }


def artifact(artifact_id: int, name: str) -> dict[str, object]:
    return {
        "id": artifact_id,
        "name": name,
        "size_in_bytes": 1234,
        "expired": False,
        "created_at": "2026-07-30T00:01:00Z",
        "expires_at": "2026-10-28T00:01:00Z",
    }


def payload(
    jobs_pages: object,
    artifacts_pages: object,
    *,
    status: str = "completed",
    conclusion: str | None = "success",
) -> dict[str, object]:
    return publish_status.build_payload(
        jobs_pages=jobs_pages,
        artifacts_pages=artifacts_pages,
        workflow_file=".github/workflows/publish-ci-status.yml",
        issue_number=19,
        monitored_branch=BRANCH,
        workflow_name="Quality",
        workflow_id=44,
        run_id=55,
        run_attempt=1,
        run_url="https://github.com/owner/repository/actions/runs/55",
        event="push",
        status=status,
        conclusion=conclusion,
        head_repository=REPOSITORY,
        head_branch=BRANCH,
        head_sha="a" * 40,
        created_at="2026-07-30T00:00:00Z",
        updated_at="2026-07-30T00:01:00Z",
    )


def machine_payload(body: str) -> dict[str, object]:
    lines = body.splitlines()
    heading_index = lines.index("## Machine-readable status")
    opening_index = heading_index + 2
    opening = lines[opening_index]
    if not opening.endswith("json"):
        raise AssertionError("missing JSON opening fence")
    fence = opening[:-4]
    closing_index = lines.index(fence, opening_index + 1)
    return json.loads("\n".join(lines[opening_index + 1 : closing_index]))


class PublishStatusTests(unittest.TestCase):
    def test_latest_run_is_branch_and_repository_specific(self) -> None:
        pages = [
            {
                "workflow_runs": [
                    {
                        "id": 1,
                        "head_repository": {"full_name": "fork/repository"},
                        "head_branch": BRANCH,
                        "event": "push",
                    },
                    {
                        "id": 2,
                        "head_repository": {"full_name": REPOSITORY},
                        "head_branch": "feature/test",
                        "event": "push",
                    },
                    {
                        "id": 3,
                        "head_repository": {"full_name": REPOSITORY},
                        "head_branch": BRANCH,
                        "event": "pull_request",
                    },
                    {
                        "id": 4,
                        "head_repository": {"full_name": REPOSITORY},
                        "head_branch": BRANCH,
                        "event": "push",
                    },
                ]
            }
        ]
        self.assertEqual(
            4,
            publish_status.latest_applicable_run_id(
                pages,
                repository=REPOSITORY,
                branch=BRANCH,
                accepted_events={"push", "workflow_dispatch"},
            ),
        )

    def test_issue_owner_accepts_legacy_marker_only_for_known_issue(self) -> None:
        issue = {
            "number": 19,
            "state": "open",
            "title": "CI Status: Quality",
            "body": publish_status.LEGACY_MARKER + "\nold body",
        }
        publish_status.validate_issue_owner(
            issue,
            issue_number=19,
            expected_marker=MARKER,
            accepted_titles={"CI Status: Quality", "CI Status: Quality — master"},
        )
        wrong = copy.deepcopy(issue)
        wrong["title"] = "Ordinary bug"
        with self.assertRaises(publish_status.StatusDataError):
            publish_status.validate_issue_owner(
                wrong,
                issue_number=19,
                expected_marker=MARKER,
                accepted_titles={"CI Status: Quality", "CI Status: Quality — master"},
            )

    def test_payload_merges_pages_and_reports_problems_and_artifacts(self) -> None:
        jobs_pages = [
            {"total_count": 2, "jobs": [job(10, "host")]},
            {
                "total_count": 2,
                "jobs": [
                    job(
                        11,
                        "device",
                        "failure",
                        [step(1, "Compile", "failure")],
                    )
                ],
            },
        ]
        artifacts_pages = [
            {"total_count": 2, "artifacts": [artifact(20, "logs")]},
            {"total_count": 2, "artifacts": [artifact(21, "firmware")]},
        ]
        result = payload(
            jobs_pages,
            artifacts_pages,
            conclusion="failure",
        )
        self.assertEqual([10, 11], [item["id"] for item in result["jobs"]])
        self.assertEqual([20, 21], [item["id"] for item in result["artifacts"]])
        self.assertEqual(1, result["job_summary"]["failed_or_abnormal"])
        self.assertEqual(
            {
                "job_id": 11,
                "job": "device",
                "step_number": 1,
                "step": "Compile",
                "conclusion": "failure",
            },
            result["problem_steps"][0],
        )

    def test_pending_run_does_not_fabricate_job_or_artifact_availability(self) -> None:
        result = payload(
            [{"total_count": 0, "jobs": []}],
            [],
            status="in_progress",
            conclusion=None,
        )
        self.assertEqual("not_yet_available", result["job_data_state"])
        self.assertEqual(
            "not_requested_until_completion", result["artifact_data_state"]
        )
        self.assertEqual([], result["jobs"])
        self.assertEqual([], result["artifacts"])

    def test_markdown_fence_cannot_be_closed_by_job_name(self) -> None:
        result = payload(
            [
                {
                    "total_count": 1,
                    "jobs": [
                        job(
                            10,
                            "host ``` injected",
                            "failure",
                            [step(1, "bad ``` step", "failure")],
                        )
                    ],
                }
            ],
            [{"total_count": 0, "artifacts": []}],
            conclusion="failure",
        )
        compacted, body = publish_status.compact_payload(
            result, "Latest Quality Run — master", MARKER
        )
        parsed = machine_payload(body)
        self.assertEqual(compacted, parsed)
        self.assertIn("host ``` injected", parsed["jobs"][0]["name"])

    def test_compaction_preserves_all_jobs_and_problem_steps(self) -> None:
        large_steps = [step(index + 1, "x" * 1000) for index in range(25)]
        jobs = [
            job(index + 1, f"success-{index}", steps=large_steps)
            for index in range(25)
        ]
        jobs.append(
            job(
                999,
                "failure-job",
                "failure",
                large_steps + [step(99, "actual failure", "failure")],
            )
        )
        result = payload(
            [{"total_count": len(jobs), "jobs": jobs}],
            [{"total_count": 0, "artifacts": []}],
            conclusion="failure",
        )
        compacted, body = publish_status.compact_payload(
            result, "Latest Quality Run — master", MARKER
        )
        parsed = machine_payload(body)
        self.assertLessEqual(
            len(body.encode("utf-8")), publish_status.ISSUE_BODY_MAX_BYTES
        )
        self.assertTrue(parsed["details_compacted"])
        self.assertEqual(
            list(range(1, 26)) + [999],
            [item["id"] for item in parsed["jobs"]],
        )
        self.assertEqual("actual failure", parsed["problem_steps"][0]["step"])
        self.assertEqual(compacted, parsed)

    def test_malformed_api_shape_fails_closed(self) -> None:
        with self.assertRaises(publish_status.StatusDataError):
            publish_status.normalize_jobs([{"jobs": "not-an-array"}])
        with self.assertRaises(publish_status.StatusDataError):
            publish_status.normalize_artifacts(
                [{"artifacts": [{"id": 1, "name": "x", "expired": "no"}]}]
            )


if __name__ == "__main__":
    unittest.main()
