# ChatGPT-readable GitHub Actions CI status bridge

## Project adoption specification

This repository adopts the reusable ChatGPT-readable GitHub Actions CI status
bridge. The bridge makes ordinary push-triggered GitHub Actions runs discoverable
through stable GitHub issues without replacing native checks, logs, artifacts,
branch protection, or the Actions user interface.

The status issues are current-state indexes. They are overwritten in place and
are not historical logs.

## Resolved project configuration

```yaml
repository: ekkus93/esp32-macro-keyboard
default_branch: master

monitored_targets:
  - workflow_name: Quality
    workflow_file: .github/workflows/quality.yml
    branch: master
    issue_number: 19
    issue_title: "CI Status: Quality — master"

  - workflow_name: Host Tests
    workflow_file: .github/workflows/host-tests.yml
    branch: master
    issue_number: 20
    issue_title: "CI Status: Host Tests — master"

  - workflow_name: Browser Tests
    workflow_file: .github/workflows/browser-tests.yml
    branch: master
    issue_number: 21
    issue_title: "CI Status: Browser Tests — master"

  - workflow_name: Device Test Build
    workflow_file: .github/workflows/device-test-build.yml
    branch: master
    issue_number: 22
    issue_title: "CI Status: Device Test Build — master"

implementation_policy:
  create_branch: false
  create_pull_request: false
  preserve_existing_ci: true
  publish_raw_logs: false
  publish_artifact_metadata: true
```

The four selected workflows are the permanent quality gates used during normal
implementation work:

- `Quality` is the authoritative full repository gate.
- `Host Tests` supplies ordinary host tests, ASan/UBSan, native coverage, and
  frontend validation.
- `Browser Tests` supplies the real-Chrome management and execution workflow
  gate.
- `Device Test Build` supplies the ESP32-S3 compile-only device-test gate.

Only `master` is monitored. Pull-request branches, forks, tags, and unrelated
event categories cannot overwrite these issues.

## Implementation files

- Publisher workflow: `.github/workflows/publish-ci-status.yml`
- Payload generator: `tools/ci_status/publish_status.py`
- Generator tests: `tools/ci_status/test_publish_status.py`
- Permanent test registration: `scripts/check-scripts.sh`
- CI and operating documentation: `docs/CI_REPRODUCIBILITY.md`

The shared generator is used because four workflows are monitored. It is not an
unused helper and is tested by the authoritative Quality gate.

## Publisher trigger

The publisher handles these `workflow_run` activity types:

```yaml
types:
  - requested
  - in_progress
  - completed
```

The workflow file must remain on the default branch. GitHub does not activate a
`workflow_run` publisher that exists only on another branch.

## Permissions and trust boundary

The publisher uses only:

```yaml
permissions:
  actions: read
  contents: read
  issues: write
```

It checks out a sparse copy of `tools/ci_status` from the trusted `master`
branch with persisted credentials disabled. It never checks out or executes the
triggering run's commit or branch.

The publisher does not:

- execute artifacts;
- copy raw logs into an issue;
- publish environment variables or secrets;
- rerun workflows;
- alter production code, releases, tags, pull requests, or branches;
- update an issue outside the configured workflow-to-issue mapping.

Before writing, it fetches the configured issue and verifies all of the
following:

- the numeric issue ID is exact;
- the issue is open;
- the title is the configured current title or its one-time legacy title;
- the body begins with the expected automation ownership marker.

After writing, it fetches and validates the issue again.

## Branch isolation and stale-event rejection

A run is publishable only when:

- `head_repository.full_name` equals `ekkus93/esp32-macro-keyboard`;
- `head_branch` equals `master`;
- the event is `push` or `workflow_dispatch`;
- the run ID is the latest applicable run for the same workflow and branch.

The latest-run query is branch-scoped. The tested generator then filters the
returned run pages by repository, branch, and accepted event category. Older
requested, in-progress, or completed events exit without allowing later issue
steps to execute.

Concurrency is isolated by workflow ID and branch. A newer publisher event may
cancel an older publisher for the same workflow/branch pair without affecting
another monitored workflow.

## Published human-readable summary

Every issue begins with a compact summary containing:

- status and conclusion;
- exact run ID, URL, and attempt;
- exact commit SHA;
- branch and event;
- completed, failed/abnormal, running, and pending job counts;
- all abnormal job/step pairs;
- artifact count;
- job and artifact metadata availability state;
- publisher observation time.

Names derived from GitHub API data are rendered with injection-resistant
Markdown code spans. The JSON fence is selected dynamically so backticks in a
job, step, or artifact name cannot terminate the machine-readable block.

## Machine-readable schema

The issue includes a parseable JSON object with these top-level fields:

```json
{
  "schema_version": 1,
  "publisher": {},
  "workflow": {},
  "job_data_state": "available",
  "artifact_data_state": "available_empty",
  "details_compacted": false,
  "compaction_reason": null,
  "job_summary": {},
  "problem_steps": [],
  "jobs": [],
  "artifacts": []
}
```

`publisher` identifies the publisher workflow file, status issue, monitored
branch, observation time, and issue-body byte ceiling.

`workflow` identifies the workflow name and ID, run ID and attempt, URL, event,
status, conclusion, trusted head repository, branch, SHA, and GitHub timestamps.

Each job records its exact job ID, name, status, conclusion, timestamps, runner,
runner group, and steps. Each step records its exact number, name, status,
conclusion, and timestamps.

At minimum, these conclusions are abnormal:

- `failure`
- `cancelled`
- `timed_out`
- `action_required`
- `startup_failure`
- `stale`

Each artifact records its ID, name, byte size, expiration state, creation time,
and expiration time.

## Pagination and bounded issue generation

Jobs and artifacts are requested with GitHub CLI pagination and slurped into
page arrays. The generator validates every page and collection before producing
an issue body. Malformed or missing required API fields fail closed.

The generated issue body is limited to 60,000 UTF-8 bytes. If the full payload
would exceed that limit, the generator:

1. preserves the summary, workflow metadata, every job ID, every final job
   conclusion, every artifact, and every abnormal step;
2. removes successful-job step details first;
3. if still required, retains only abnormal steps within remaining oversized
   job detail;
4. sets `details_compacted` and `compaction_reason` explicitly;
5. fails instead of publishing invalid or silently truncated JSON if the bounded
   representation still does not fit.

## Explicit unavailable-data states

The bridge does not fabricate an empty successful job list while GitHub is
still creating jobs.

- An early run with no job records uses `job_data_state: not_yet_available`.
- A completed run with no job records uses
  `job_data_state: unavailable_after_completion`.
- Artifacts are not queried before completion and use
  `artifact_data_state: not_requested_until_completion`.
- A completed run with a valid empty artifact collection uses
  `artifact_data_state: available_empty`.

## Generator regression coverage

The permanent unit tests prove:

- branch-, repository-, and event-specific latest-run selection;
- legacy-to-current status issue ownership migration;
- multi-page job and artifact merging;
- exact abnormal job and step summaries;
- explicit pending metadata states;
- Markdown-fence injection resistance;
- bounded compaction while preserving all job IDs and abnormal steps;
- fail-closed behavior for malformed GitHub API shapes.

`actionlint` continues to validate the publisher workflow itself.

## ChatGPT operating procedure

For every Ralph-loop candidate:

1. Record the exact candidate SHA.
2. Read issues #19 through #22.
3. Ignore any issue whose machine-readable `workflow.head_sha` differs from the
   candidate SHA.
4. Treat queued or in-progress issue data as a discovery index, not completion
   evidence.
5. On failure, use the exact run ID and failed job ID to fetch only the relevant
   job log.
6. Patch the first meaningful failure and repeat.
7. Claim a gate is green only when its issue reports `success` for the exact
   candidate SHA.
8. Preserve the distinction between host success, browser success, compile-only
   device success, actual hardware-runtime validation, and manual acceptance.

## Remaining runtime limitations

- `workflow_run` publishes lifecycle transitions, not continuous per-step live
  polling. A long-running issue can remain unchanged between events.
- Normal master pushes do not always produce artifacts. Artifact collection is
  fully implemented and unit-tested, but a completed issue will legitimately
  report `available_empty` when its workflow produced none.
- The status issues are current snapshots and do not retain run history.
- Hardware-runtime success cannot be inferred from the compile-only Device Test
  Build workflow.
