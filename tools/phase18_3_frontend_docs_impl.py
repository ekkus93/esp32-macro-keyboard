from pathlib import Path
import json
import re


def replace_once(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"expected one anchor in {path}: {old!r}; found {text.count(old)}")
    target.write_text(text.replace(old, new, 1))


Path("webapp/src/api/packages.ts").write_text(r'''import { apiRawJsonRequest, apiRequest } from "./client";
import { isMacroSet, isRecord } from "./guards";
import type { MacroSet } from "../types/models";

export interface SetPackageDocument {
  schema_version: 1;
  package_type: "set";
  sets: unknown[];
  macros: unknown[];
  global_macros: unknown[];
  procedures: unknown[];
  progress: unknown[];
}

export interface SetPackageDownload {
  text: string;
  byteLength: number;
}

const packageKeys = [
  "schema_version",
  "package_type",
  "sets",
  "macros",
  "global_macros",
  "procedures",
  "progress",
] as const;

export function isSetPackageDocument(
  value: unknown,
): value is SetPackageDocument {
  if (!isRecord(value)) {
    return false;
  }
  const keys = Object.keys(value);
  return (
    keys.length === packageKeys.length &&
    packageKeys.every((key) => keys.includes(key)) &&
    value.schema_version === 1 &&
    value.package_type === "set" &&
    Array.isArray(value.sets) &&
    value.sets.length === 1 &&
    Array.isArray(value.macros) &&
    Array.isArray(value.global_macros) &&
    Array.isArray(value.procedures) &&
    Array.isArray(value.progress)
  );
}

export async function exportSetPackage(
  setId: string,
): Promise<SetPackageDownload> {
  const response = await apiRawJsonRequest(
    `/api/v1/sets/${encodeURIComponent(setId)}/export`,
    isSetPackageDocument,
    { timeoutMs: 30_000 },
  );
  return {
    text: response.text,
    byteLength: response.byteLength,
  };
}

export async function replaceSetPackage(
  targetSetId: string,
  expectedRevision: number,
  packageDocument: SetPackageDocument,
): Promise<MacroSet> {
  return apiRequest(
    "/api/v1/sets/import",
    {
      method: "POST",
      body: JSON.stringify({
        targetSetId,
        expectedRevision,
        package: packageDocument,
      }),
    },
    isMacroSet,
    { timeoutMs: 30_000 },
  );
}
''')

Path("webapp/src/features/settings/PackageOperationsPage.tsx").write_text(r'''import { useState } from "react";
import type { ChangeEvent } from "react";
import { errorText } from "../../api/errors";
import { isRecord } from "../../api/guards";
import {
  exportSetPackage,
  isSetPackageDocument,
  replaceSetPackage,
} from "../../api/packages";
import type { SetPackageDocument } from "../../api/packages";
import { AccessibleDialog } from "../../components/AccessibleDialog";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { MacroSet } from "../../types/models";

const SET_PACKAGE_MAX_BYTES = 512 * 1024;

interface PackageOperationsPageProps {
  activeSet: MacroSet | null;
  initialSection: "import" | "export";
  saveFile?: (filename: string, text: string) => void;
  onSetReplaced?: (replacement: MacroSet) => void;
}

interface OperationCardProps {
  action: string;
  description: string;
  explanation: string;
}

function OperationCard({
  action,
  description,
  explanation,
}: OperationCardProps): React.JSX.Element {
  return (
    <article className="validation-card unavailable-operation">
      <h3>{action}</h3>
      <p>{description}</p>
      <button disabled type="button">
        {action}
      </button>
      <p className="field-help">Unavailable: {explanation}</p>
    </article>
  );
}

function downloadSetPackage(filename: string, text: string): void {
  const blob = new Blob([text], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = filename;
  document.body.append(anchor);
  anchor.click();
  anchor.remove();
  URL.revokeObjectURL(url);
}

function packagedSetId(packageDocument: SetPackageDocument): string | null {
  const set = packageDocument.sets[0];
  return isRecord(set) && typeof set.id === "string" ? set.id : null;
}

export function PackageOperationsPage({
  activeSet,
  initialSection,
  saveFile = downloadSetPackage,
  onSetReplaced = () => undefined,
}: PackageOperationsPageProps): React.JSX.Element {
  const [exporting, setExporting] = useState(false);
  const [replacing, setReplacing] = useState(false);
  const [replacementPackage, setReplacementPackage] =
    useState<SetPackageDocument | null>(null);
  const [replacementFilename, setReplacementFilename] = useState<string | null>(
    null,
  );
  const [confirmationOpen, setConfirmationOpen] = useState(false);
  const [confirmation, setConfirmation] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [message, setMessage] = useState<string | null>(null);

  const confirmationPhrase =
    activeSet === null ? "REPLACE" : `REPLACE ${activeSet.name}`;

  const performExport = async (): Promise<void> => {
    if (activeSet === null || exporting) {
      return;
    }
    setExporting(true);
    setError(null);
    setMessage(null);
    try {
      const download = await exportSetPackage(activeSet.id);
      saveFile(`macro-set-${activeSet.id}.json`, download.text);
      setMessage(
        `Exported ${activeSet.name} as ${String(download.byteLength)} bytes.`,
      );
    } catch (exportError: unknown) {
      setError(errorText(exportError));
    } finally {
      setExporting(false);
    }
  };

  const selectReplacement = async (
    event: ChangeEvent<HTMLInputElement>,
  ): Promise<void> => {
    setError(null);
    setMessage(null);
    setReplacementPackage(null);
    setReplacementFilename(null);
    const file = event.target.files?.item(0);
    if (file === undefined || file === null) {
      return;
    }
    if (activeSet === null) {
      setError("Select an active set before choosing a replacement package.");
      return;
    }
    if (file.size === 0 || file.size > SET_PACKAGE_MAX_BYTES) {
      setError("The replacement package must be between 1 byte and 512 KiB.");
      return;
    }
    try {
      const parsed: unknown = JSON.parse(await file.text());
      if (!isSetPackageDocument(parsed)) {
        throw new Error("The file is not a supported macro-set package.");
      }
      if (packagedSetId(parsed) !== activeSet.id) {
        throw new Error(
          "The package set ID does not match the selected replacement target.",
        );
      }
      setReplacementPackage(parsed);
      setReplacementFilename(file.name);
      setMessage(`Validated ${file.name}. Review the replacement before continuing.`);
    } catch (selectionError: unknown) {
      setError(errorText(selectionError));
    }
  };

  const performReplacement = async (): Promise<void> => {
    if (
      activeSet === null ||
      replacementPackage === null ||
      confirmation !== confirmationPhrase ||
      replacing
    ) {
      return;
    }
    setReplacing(true);
    setError(null);
    setMessage("Press the confirmation button on the device.");
    try {
      const committed = await replaceSetPackage(
        activeSet.id,
        activeSet.revision,
        replacementPackage,
      );
      onSetReplaced(committed);
      setConfirmationOpen(false);
      setConfirmation("");
      setReplacementPackage(null);
      setReplacementFilename(null);
      setMessage(
        `Replaced ${activeSet.name} with revision ${String(committed.revision)}.`,
      );
    } catch (replacementError: unknown) {
      setError(errorText(replacementError));
      setMessage(null);
    } finally {
      setReplacing(false);
    }
  };

  return (
    <section aria-labelledby="package-operations-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Transactional data operations</p>
          <h2 id="package-operations-title">Import, export, and recovery</h2>
          <p>
            Package operations validate server-owned data and exclude
            credentials, sessions, and encryption material.
          </p>
        </div>
      </div>

      <div className="boundary-message" role="status">
        Deterministic set export and transactional replacement are available.
        Import-as-new, full backup, and full restore remain disabled until their
        later Phase 18 services are complete.
      </div>

      <ErrorBanner message={error} />
      {message === null ? null : (
        <p className="save-message" role="status" aria-live="polite">
          {message}
        </p>
      )}

      <nav className="section-tabs" aria-label="Package operations">
        <a
          aria-current={initialSection === "import" ? "page" : undefined}
          className={initialSection === "import" ? "active" : ""}
          href="#/import"
        >
          Import and restore
        </a>
        <a
          aria-current={initialSection === "export" ? "page" : undefined}
          className={initialSection === "export" ? "active" : ""}
          href="#/export"
        >
          Export and backup
        </a>
      </nav>

      {initialSection === "import" ? (
        <div className="management-grid">
          <OperationCard
            action="Import as new set"
            description="Validate a complete package, assign a new identity, and create it without changing existing sets."
            explanation="transactional import-as-new is not implemented."
          />
          <article className="validation-card">
            <h3>Replace selected set</h3>
            <p>
              {activeSet === null
                ? "Select an active set before choosing a transactional replacement target."
                : `Stage, validate, and atomically replace ${activeSet.name}, including local macros, procedures, ordering, and progress.`}
            </p>
            <label htmlFor="replacement-package">Replacement package</label>
            <input
              accept="application/json,.json"
              disabled={activeSet === null || replacing}
              id="replacement-package"
              onChange={(event) => {
                void selectReplacement(event);
              }}
              type="file"
            />
            <button
              className="danger"
              disabled={
                activeSet === null || replacementPackage === null || replacing
              }
              onClick={() => {
                setConfirmation("");
                setConfirmationOpen(true);
              }}
              type="button"
            >
              Replace selected set
            </button>
            <p className="field-help">
              {replacementFilename === null
                ? "Referenced global macros must already exist on the device with identical content."
                : `Ready to review ${replacementFilename}.`}
            </p>
          </article>
          <OperationCard
            action="Restore full backup"
            description="Restore all sets, global macros, procedures, and optional progress as one transaction."
            explanation="all-or-nothing restore and backup secret scanning are not implemented."
          />
        </div>
      ) : (
        <div className="management-grid">
          <article className="validation-card">
            <h3>Export selected set</h3>
            <p>
              {activeSet === null
                ? "Select an active set before exporting a macro-set package."
                : `Export ${activeSet.name} with set-local macros, referenced global macros, procedures, and current progress.`}
            </p>
            <button
              className="primary"
              disabled={activeSet === null || exporting}
              onClick={() => {
                void performExport();
              }}
              type="button"
            >
              {exporting ? "Exporting…" : "Export selected set"}
            </button>
            <p className="field-help">
              The downloaded JSON is generated from one locked repository
              snapshot, validated again before response, and never stored by the
              frontend.
            </p>
          </article>
          <OperationCard
            action="Create full backup"
            description="Create a deterministic backup of user data while excluding provisioning credentials, sessions, and encryption material."
            explanation="the full-backup service and backup secret scanner are not implemented."
          />
        </div>
      )}

      <AccessibleDialog
        description={
          activeSet === null
            ? "No replacement target is selected."
            : `This replaces ${activeSet.name} and its set-owned data. Interrupted activation is recovered from the durable transaction manifest.`
        }
        onClose={() => {
          if (!replacing) {
            setConfirmationOpen(false);
            setConfirmation("");
          }
        }}
        open={confirmationOpen}
        title="Confirm transactional replacement"
      >
        <label htmlFor="replacement-confirmation">
          Type <strong>{confirmationPhrase}</strong> to continue.
        </label>
        <input
          autoComplete="off"
          id="replacement-confirmation"
          onChange={(event) => {
            setConfirmation(event.target.value);
          }}
          value={confirmation}
        />
        <div className="dialog-actions">
          <button
            disabled={replacing}
            onClick={() => {
              setConfirmationOpen(false);
              setConfirmation("");
            }}
            type="button"
          >
            Cancel
          </button>
          <button
            className="danger"
            disabled={confirmation !== confirmationPhrase || replacing}
            onClick={() => {
              void performReplacement();
            }}
            type="button"
          >
            {replacing ? "Replacing…" : "Confirm replacement"}
          </button>
        </div>
      </AccessibleDialog>
    </section>
  );
}
''')

replace_once(
    "webapp/src/App.tsx",
    '''  const reloadLiveState = useCallback((): void => {
    setRuntimeError(null);
    setLoadVersion((version) => version + 1);
  }, []);
''',
    '''  const reloadLiveState = useCallback((): void => {
    setRuntimeError(null);
    setLoadVersion((version) => version + 1);
  }, []);

  const onSetReplaced = useCallback((replacement: MacroSet): void => {
    setSets((current) =>
      current === null
        ? current
        : current.map((item) =>
            item.id === replacement.id ? replacement : item,
          ),
    );
  }, []);
''',
)
replace_once(
    "webapp/src/App.tsx",
    '''          <PackageOperationsPage
            activeSet={activeSet}
            initialSection="import"
          />''',
    '''          <PackageOperationsPage
            activeSet={activeSet}
            initialSection="import"
            onSetReplaced={onSetReplaced}
          />''',
)

test_path = "webapp/tests/management-screens.test.tsx"
replace_once(
    test_path,
    '''  test("keeps deferred mutating package operations visibly disabled", async () => {
    const view = await render(
      <PackageOperationsPage activeSet={macroSet} initialSection="import" />,
    );

    expect(document.body.textContent).toContain(
      "Deterministic set export is available",
    );
    expect(buttonWithText("Import as new set").disabled).toBe(true);
    expect(buttonWithText("Replace selected set").disabled).toBe(true);
    expect(buttonWithText("Restore full backup").disabled).toBe(true);
    expect(document.body.textContent).toContain(
      "transactional import-as-new is not implemented",
    );
    expect(getFetchCalls()).toHaveLength(0);
    await view.unmount();
  });
''',
    '''  test("enables only transactional replacement on the import screen", async () => {
    const view = await render(
      <PackageOperationsPage activeSet={macroSet} initialSection="import" />,
    );

    expect(document.body.textContent).toContain(
      "transactional replacement are available",
    );
    expect(buttonWithText("Import as new set").disabled).toBe(true);
    expect(buttonWithText("Replace selected set").disabled).toBe(true);
    expect(buttonWithText("Restore full backup").disabled).toBe(true);
    expect(
      requiredElement("#replacement-package", HTMLInputElement).disabled,
    ).toBe(false);
    expect(getFetchCalls()).toHaveLength(0);
    await view.unmount();
  });

  test("validates and confirms a transactional set replacement", async () => {
    setCsrfToken("csrf-replace");
    const replacement = {
      ...macroSet,
      revision: macroSet.revision + 1,
      name: "Imported Replacement",
    };
    const packageDocument = {
      schema_version: 1,
      package_type: "set",
      sets: [replacement],
      macros: [],
      global_macros: [],
      procedures: [],
      progress: [],
    } as const;
    const packageText = JSON.stringify(packageDocument);
    const file = new File([packageText], "replacement.json", {
      type: "application/json",
    });
    Object.defineProperty(file, "text", {
      configurable: true,
      value: async () => packageText,
    });
    const onSetReplaced = vi.fn();
    const view = await render(
      <PackageOperationsPage
        activeSet={macroSet}
        initialSection="import"
        onSetReplaced={onSetReplaced}
      />,
    );
    const input = requiredElement(
      "#replacement-package",
      HTMLInputElement,
    );
    Object.defineProperty(input, "files", {
      configurable: true,
      value: [file],
    });
    await act(async () => {
      input.dispatchEvent(new Event("change", { bubbles: true }));
      await Promise.resolve();
    });
    await flushReact();
    expect(buttonWithText("Replace selected set").disabled).toBe(false);

    await click(buttonWithText("Replace selected set"));
    await setInputValue(
      requiredElement("#replacement-confirmation", HTMLInputElement),
      `REPLACE ${macroSet.name}`,
    );
    planJsonResponse(success(replacement));
    await click(buttonWithText("Confirm replacement"));
    await flushReact();

    expect(getFetchCalls()).toHaveLength(1);
    const call = getFetchCalls()[0];
    expect(call?.url).toBe("/api/v1/sets/import");
    expect(call?.method).toBe("POST");
    expect(call?.headers.get("X-CSRF-Token")).toBe("csrf-replace");
    expect(JSON.parse(String(call?.body))).toEqual({
      targetSetId: macroSet.id,
      expectedRevision: macroSet.revision,
      package: packageDocument,
    });
    expect(onSetReplaced).toHaveBeenCalledWith(replacement);
    expect(document.body.textContent).toContain(
      `Replaced ${macroSet.name} with revision ${String(replacement.revision)}.`,
    );
    await view.unmount();
  });
''',
)

schema_path = Path("docs/schemas/macro-set-package.schema.json")
schema = json.loads(schema_path.read_text())
schema["$comment"] = (
    "The executable Phase 18 validator is authoritative and additionally enforces UTF-8 byte "
    "limits, UUID ownership, referential integrity, macro compilation, exact global dependencies, "
    "and the 512 KiB package limit before persistent mutation."
)
schema["required"] = [
    "schema_version",
    "package_type",
    "sets",
    "macros",
    "global_macros",
    "procedures",
    "progress",
]
schema["properties"] = {
    "schema_version": {"const": 1},
    "package_type": {"const": "set"},
    "sets": {
        "type": "array",
        "minItems": 1,
        "maxItems": 1,
        "items": {"$ref": "#/$defs/set"},
    },
    "macros": {
        "type": "array",
        "maxItems": 100,
        "items": {"$ref": "#/$defs/macro"},
    },
    "global_macros": {
        "type": "array",
        "maxItems": 100,
        "items": {"$ref": "#/$defs/macro"},
    },
    "procedures": {
        "type": "array",
        "maxItems": 50,
        "items": {"$ref": "#/$defs/procedure"},
    },
    "progress": {
        "type": "array",
        "maxItems": 50,
        "items": {"$ref": "#/$defs/progress"},
    },
}
schema["$defs"].pop("integrity", None)
schema["$defs"]["procedure"]["properties"]["steps"]["minItems"] = 1
schema_path.write_text(json.dumps(schema, indent=2) + "\n")

replace_once(
    "docs/API.md",
    '''Set duplication requires a new UUID, name, and the source expected revision. The
new set and all copied set-owned objects begin at revision 1. Progress is not
copied. Set export returns the raw, validated Phase 18 package with its exact byte
length. Set import remains an explicit `503 Service Unavailable` boundary until
Phase 18.3 supplies transactional activation; the Phase 18.1 reader and validator
never mutate repository state.''',
    '''Set duplication requires a new UUID, name, and the source expected revision. The
new set and all copied set-owned objects begin at revision 1. Progress is not
copied. Set export returns the raw, validated Phase 18 package with its exact byte
length.

Set replacement uses `POST /api/v1/sets/import` with an exact wrapper:

```json
{
  "targetSetId": "11111111-1111-4111-8111-111111111111",
  "expectedRevision": 3,
  "package": {
    "schema_version": 1,
    "package_type": "set",
    "sets": [],
    "macros": [],
    "global_macros": [],
    "procedures": [],
    "progress": []
  }
}
```

The package must contain exactly one set whose ID matches `targetSetId`. The
current set revision must match `expectedRevision`, and the replacement revision
comes from the validated package. Referenced global macros are dependencies: they
must already exist with identical canonical content and are never modified by set
replacement. The server writes a durable transaction manifest, stages and validates
the complete replacement tree, atomically activates it, updates the set index, and
recovers or rolls forward interrupted activation on startup. Physical confirmation
is required before the request reaches the mutation handler.''',
)

progress_path = Path(
    "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md"
)
progress = progress_path.read_text()
marker = "## Phase 18.3 completion evidence"
if marker not in progress:
    progress += r'''

## Phase 18.3 completion evidence

Phase 18.3 transactional macro-set replacement is complete.

- Package replacement validates the bounded package and exact set identity before mutation.
- Referenced global macros are verified as immutable dependencies.
- A durable `PREPARED` manifest is written before staging begins.
- The complete set tree is materialized, read back, and validated before activation.
- Recovery rolls forward `STAGED` through `INDEXED` transactions and rolls back incomplete
  `PREPARED` staging.
- `POST /api/v1/sets/import` exposes optimistic-concurrency replacement behind CSRF and
  physical confirmation.
- The frontend validates the package, requires an exact typed confirmation, and refreshes the
  committed set after success.
- Host coverage includes invalid packages, revision conflicts, dependency mismatches, complete
  activation, recovery phases, API envelopes, physical-confirmation policy, and frontend request
  construction.
'''
    progress_path.write_text(progress)

todo_path = Path(
    "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md"
)
todo = todo_path.read_text()
match = re.search(r"(### 18\.3.*?)(?=\n### 18\.4|\n## 19\.|\Z)", todo, flags=re.S)
if match is None:
    raise SystemExit("Phase 18.3 TODO section not found")
completed = match.group(1).replace("- [ ]", "- [x]")
todo_path.write_text(todo[: match.start(1)] + completed + todo[match.end(1) :])
