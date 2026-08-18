# Visual-regression baselines

Machine-written by `run-visual-tests.mjs --update-baselines`. Do not hand-edit.

One JSON file per `<scenario>--<viewport>.json`, holding the
`captureScenario()` result for that pair (`elements`, `pseudoElements`) minus
its screenshot — see "JSON-only, deliberately" below. 79 files, ~14MB, one per
`(scenario, viewport)` pair in `scenarios.mjs`.

## Updating a baseline

```bash
export NVM_DIR="$HOME/.nvm" && . "$NVM_DIR/nvm.sh" && nvm use 24.18.0
npm --prefix webapp run build
node webapp/tests/browser/visual/run-visual-tests.mjs --update-baselines
```

Regenerate **only** in response to a deliberate, understood change — never to
make a failing check pass without reading why it failed. Before running
`--update-baselines`:

1. Run without it first and read the diff. Confirm every reported property
   change is one you intended.
2. If anything is unexplained, stop and investigate — do not update over it.
3. Once satisfied, regenerate and **review the resulting `git diff` before
   committing it**, the same as reviewing any other change. A baseline update
   is a statement that the new rendering is correct, not a formality.
4. Commit the baseline update as its own change, separate from the code
   change that caused it, so the two are independently reviewable and
   revertible.

To update or check a subset while iterating, pass `--grep <substring>` (see
`run-visual-tests.mjs`'s header comment for the full flag list).

## JSON-only, deliberately

Screenshots are captured (`capture.mjs`) but **not** committed as baselines —
only the computed-style/geometry JSON is. This was a real decision, not a
default, made while building the harness (T1-1/T1-2):

- **The property walk is the stronger, cheaper signal.** During the review
  that produced this harness, the property comparison caught every
  regression a screenshot also caught, plus differences a full-page
  screenshot structurally cannot see — inside an inner scroll container, or
  behind a modal overlay. A screenshot adds confidence, not detection power,
  for the properties this harness already tracks.
- **Binary cost.** This is a firmware repository that also ships a flash
  image; PNG baselines at two viewports per scenario would roughly double
  the ~14MB this directory already costs, and every future baseline update
  would add another full set of binaries to history permanently (git does
  not diff binaries incrementally the way it does text).
- **Reviewability.** A `git diff` on these JSON files shows exactly which
  properties changed, on which element, in a PR — the actual mechanism
  `compare.mjs` uses. A changed PNG shows nothing in a text diff; a reviewer
  has to fetch and eyeball it.

Screenshots are still taken on every run and written to `--diff-dir` (default:
a temp directory, printed by the driver) whenever a scenario differs from
baseline — they exist for human debugging of a failure, not as committed
truth.

If a future defect turns out to be invisible to the property walk (a visual
change with no corresponding computed-style or geometry difference — a
background _image_ choice, for instance, rather than colour), that is grounds
to revisit this decision for the specific properties involved, not to add PNG
baselines wholesale.
