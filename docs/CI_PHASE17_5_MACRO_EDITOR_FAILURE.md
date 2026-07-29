# Phase 17.5 macro editor failure

- Transform: success
- Cleanup: success
- Node setup: success
- Frontend dependencies: success
- Format: success
- Frontend validation: failure
- Host dependencies: skipped
- ESP-IDF install: skipped
- Authoritative gate: skipped

## phase17-5-macro-editor-transform.log

```text

Phase 17.5 macro editor applied
```

## phase17-5-macro-editor-cleanup.log

```text

```

## phase17-5-macro-editor-frontend-deps.log

```text

npm warn deprecated whatwg-encoding@3.1.1: Use @exodus/bytes instead for a more spec-conformant and faster implementation
npm warn deprecated glob@10.5.0: Old versions of glob are not supported, and contain widely publicized security vulnerabilities, which have been fixed in the current version. Please update. Support for old versions may be purchased (at exorbitant rates) by contacting i@izs.me

added 471 packages, and audited 472 packages in 1m

18 vulnerabilities (16 high, 2 critical)

To address issues that do not require attention, run:
  npm audit fix

To address all issues (including breaking changes), run:
  npm audit fix --force

Run `npm audit` for details.
npm warn allow-scripts 2 packages have install scripts not yet covered by allowScripts:
npm warn allow-scripts   @tailwindcss/oxide@4.1.11 (postinstall: node ./scripts/install.js)
npm warn allow-scripts   esbuild@0.25.12 (postinstall: node install.js)
npm warn allow-scripts
npm warn allow-scripts Run `npm approve-scripts --allow-scripts-pending` to review, or `npm approve-scripts <pkg>` to allow.
```

## phase17-5-macro-editor-format.log

```text
10:[90msrc/api/errors.ts[39m 3ms (unchanged)
16:[90msrc/components/ErrorBanner.tsx[39m 3ms (unchanged)
55:[90mtests/error-banner.test.tsx[39m 4ms (unchanged)
75:[90msrc/api/errors.ts[39m 3ms (unchanged)
81:[90msrc/components/ErrorBanner.tsx[39m 3ms (unchanged)
120:[90mtests/error-banner.test.tsx[39m 3ms (unchanged)


> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 52ms (unchanged)
[90mindex.html[39m 29ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 28ms (unchanged)
[90msrc/api/client.ts[39m 103ms (unchanged)
[90msrc/api/errors.ts[39m 3ms (unchanged)
[90msrc/api/guards.ts[39m 39ms (unchanged)
[90msrc/api/README.md[39m 2ms (unchanged)
[90msrc/api/routes.ts[39m 17ms (unchanged)
[90msrc/App.tsx[39m 30ms (unchanged)
[90msrc/components/AppShell.tsx[39m 10ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 3ms (unchanged)
[90msrc/components/README.md[39m 2ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 11ms (unchanged)
[90msrc/features/auth/README.md[39m 4ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 13ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 14ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 7ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 3ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 5ms (unchanged)
[90msrc/features/execution/README.md[39m 5ms (unchanged)
src/features/macros/macroDraft.ts 19ms
src/features/macros/MacroEditorPage.tsx 53ms
src/features/macros/MacroLibraryPage.tsx 16ms
[90msrc/features/macros/README.md[39m 7ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 2ms (unchanged)
[90msrc/features/sets/README.md[39m 2ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 15ms (unchanged)
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 7ms (unchanged)
[90msrc/main.tsx[39m 1ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 1ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 7ms (unchanged)
[90msrc/styles.css[39m 48ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 7ms (unchanged)
[90msrc/types/README.md[39m 2ms (unchanged)
[90mstylelint.config.mjs[39m 2ms (unchanged)
[90mtests/api.test.ts[39m 31ms (unchanged)
[90mtests/app-auth.test.tsx[39m 19ms (unchanged)
[90mtests/app-execution.test.tsx[39m 19ms (unchanged)
tests/app-macros.test.tsx 19ms
[90mtests/app-routing.test.tsx[39m 7ms (unchanged)
[90mtests/app-sets.test.tsx[39m 12ms (unchanged)
[90mtests/app.test.ts[39m 1ms (unchanged)
[90mtests/appFixtures.ts[39m 8ms (unchanged)
[90mtests/error-banner.test.tsx[39m 4ms (unchanged)
[90mtests/fakeFetch.ts[39m 12ms (unchanged)
[90mtests/fakeLocation.ts[39m 3ms (unchanged)
[90mtests/guards.test.ts[39m 7ms (unchanged)
[90mtests/README.md[39m 4ms (unchanged)
[90mtests/render.tsx[39m 10ms (unchanged)
[90mtests/setup.ts[39m 7ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)

> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 42ms (unchanged)
[90mindex.html[39m 24ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 27ms (unchanged)
[90msrc/api/client.ts[39m 80ms (unchanged)
[90msrc/api/errors.ts[39m 3ms (unchanged)
[90msrc/api/guards.ts[39m 36ms (unchanged)
[90msrc/api/README.md[39m 2ms (unchanged)
[90msrc/api/routes.ts[39m 22ms (unchanged)
[90msrc/App.tsx[39m 28ms (unchanged)
[90msrc/components/AppShell.tsx[39m 11ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 3ms (unchanged)
[90msrc/components/README.md[39m 2ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 10ms (unchanged)
[90msrc/features/auth/README.md[39m 3ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 14ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 16ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 9ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 2ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 4ms (unchanged)
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/macroDraft.ts[39m 15ms (unchanged)
[90msrc/features/macros/MacroEditorPage.tsx[39m 45ms (unchanged)
[90msrc/features/macros/MacroLibraryPage.tsx[39m 18ms (unchanged)
[90msrc/features/macros/README.md[39m 6ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 3ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 19ms (unchanged)
[90msrc/features/settings/README.md[39m 3ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 8ms (unchanged)
[90msrc/main.tsx[39m 2ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 2ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 8ms (unchanged)
[90msrc/styles.css[39m 41ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 7ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 2ms (unchanged)
[90mtests/api.test.ts[39m 25ms (unchanged)
[90mtests/app-auth.test.tsx[39m 15ms (unchanged)
[90mtests/app-execution.test.tsx[39m 18ms (unchanged)
[90mtests/app-macros.test.tsx[39m 18ms (unchanged)
[90mtests/app-routing.test.tsx[39m 7ms (unchanged)
[90mtests/app-sets.test.tsx[39m 10ms (unchanged)
[90mtests/app.test.ts[39m 1ms (unchanged)
[90mtests/appFixtures.ts[39m 9ms (unchanged)
[90mtests/error-banner.test.tsx[39m 3ms (unchanged)
[90mtests/fakeFetch.ts[39m 9ms (unchanged)
[90mtests/fakeLocation.ts[39m 3ms (unchanged)
[90mtests/guards.test.ts[39m 7ms (unchanged)
[90mtests/README.md[39m 3ms (unchanged)
[90mtests/render.tsx[39m 10ms (unchanged)
[90mtests/setup.ts[39m 6ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)
```

## phase17-5-macro-editor-frontend.log

```text
24:  const [runtimeError, setRuntimeError] = useState<string | null>(null);
30:      setRuntimeError(null);
42:      setRuntimeError(null);
52:      } catch (loadError: unknown) {
54:          setRuntimeError(errorText(loadError));
72:      } catch (statusError: unknown) {
74:          setRuntimeError(errorText(statusError));
93:    setRuntimeError(null);
107:    setRuntimeError(null);
110:    } catch (logoutError: unknown) {
111:      setRuntimeError(errorText(logoutError));
119:        <ErrorBanner message={runtimeError} />
120:        {runtimeError === null ? (
220:import { ApiError } from "../../api/client";
221:import { errorText } from "../../api/errors";
228:import { ErrorBanner } from "../../components/ErrorBanner";
269:  | { kind: "error"; fingerprint: string; message: string };
288:  const [loadError, setLoadError] = useState<string | null>(null);
305:      setLoadError(null);
310:    setLoadError(null);
325:          throw new Error(
334:      .catch((error: unknown) => {
336:          setLoadError(errorText(error));
375:        .catch((error: unknown) => {
379:          const location = parseMacroLocation(error);
380:          if (error instanceof ApiError && error.status === 422) {
384:              message: errorText(error),
389:              kind: "error",
391:              message: errorText(error),
431:  const goToError = (): void => {
478:    } catch (error: unknown) {
479:      if (error instanceof ApiError && error.status === 409) {
482:        setSaveMessage(errorText(error));
514:        <p className="error-message" role="alert">
533:  if (loadError !== null || draft === null || localValidation === null) {
537:        <ErrorBanner message={loadError ?? "Macro is unavailable."} />
553:src/App.tsx(217,48): error TS2304: Cannot find name 'routeHash'.
554:src/features/macros/MacroEditorPage.tsx(26,3): error TS6133: 'utf8ByteLength' is declared but its value is never read.
555:src/features/macros/MacroEditorPage.tsx(276,31): error TS18047: 'activeSet' is possibly 'null'.

--- App.tsx lines 20-235 ---
import type { Screen } from "./routing";
import type {
  DeviceStatus,
  ExecutionStatus,
  MacroSet,
  Settings,
} from "./types/models";

interface AuthenticatedAppProps {
  initialStatus: DeviceStatus;
  logout: () => Promise<void>;
}

function AuthenticatedApp({
  initialStatus,
  logout,
}: AuthenticatedAppProps): React.JSX.Element {
  const [route, setRoute] = useState<Screen>(() => routeFromHash());
  const [settings, setSettings] = useState<Settings | null>(null);
  const [sets, setSets] = useState<MacroSet[] | null>(null);
  const [status, setStatus] = useState(initialStatus);
  const [execution, setExecution] = useState<ExecutionStatus | null>(null);
  const [runtimeError, setRuntimeError] = useState<string | null>(null);
  const [loadVersion, setLoadVersion] = useState(0);
  const [signingOut, setSigningOut] = useState(false);

  useEffect(() => {
    const onHashChange = (): void => {
      setRuntimeError(null);
      setRoute(routeFromHash());
    };
    window.addEventListener("hashchange", onHashChange);
    return () => {
      window.removeEventListener("hashchange", onHashChange);
    };
  }, []);

  useEffect(() => {
    let active = true;
    const load = async (): Promise<void> => {
      setRuntimeError(null);
      try {
        const [loadedSettings, loadedSets] = await Promise.all([
          getSettings(),
          listSets(),
        ]);
        if (active) {
          setSettings(loadedSettings);
          setSets(loadedSets);
        }
      } catch (loadError: unknown) {
        if (active) {
          setRuntimeError(errorText(loadError));
        }
      }
    };
    void load();
    return () => {
      active = false;
    };
  }, [loadVersion]);

  useEffect(() => {
    let active = true;
    const refresh = async (): Promise<void> => {
      try {
        const current = await getDeviceStatus();
        if (active) {
          setStatus(current);
        }
      } catch (statusError: unknown) {
        if (active) {
          setRuntimeError(errorText(statusError));
        }
      }
    };
    const timer = window.setInterval(() => {
      void refresh();
    }, 5_000);
    return () => {
      active = false;
      window.clearInterval(timer);
    };
  }, []);

  const activeSet = useMemo(
    () => sets?.find((set) => set.id === settings?.activeSetId) ?? null,
    [sets, settings],
  );

  const navigateTo = useCallback((target: Screen): void => {
    setRuntimeError(null);
    navigate(target);
  }, []);

  const onTerminal = useCallback(
    (terminal: ExecutionStatus): void => {
      setExecution(terminal);
      navigateTo("result");
    },
    [navigateTo],
  );

  const signOut = async (): Promise<void> => {
    setSigningOut(true);
    setRuntimeError(null);
    try {
      await logout();
    } catch (logoutError: unknown) {
      setRuntimeError(errorText(logoutError));
      setSigningOut(false);
    }
  };

  if (settings === null || sets === null) {
    return (
      <main className="standalone" aria-busy="true">
        <ErrorBanner message={runtimeError} />
        {runtimeError === null ? (
          <p role="status">Loading device configuration…</p>
        ) : (
          <button
            onClick={() => {
              setLoadVersion((version) => version + 1);
            }}
            type="button"
          >
            Retry
          </button>
        )}
      </main>
    );
  }

  const deferred = (title: string, message: string): React.JSX.Element => (
    <DeferredPage title={title} message={message} />
  );

  const content = (() => {
    switch (route) {
      case "sets":
        return (
          <SetSelectionPage
            onSelected={setSettings}
            sets={sets}
            settings={settings}
          />
        );
      case "settings":
        return <SettingsPage onUpdated={setSettings} settings={settings} />;
      case "execution":
        return <ExecutionPage onTerminal={onTerminal} />;
      case "result":
        return (
          <ExecutionResultPage
            execution={execution}
            onReturn={() => {
              navigateTo("procedures");
            }}
          />
        );
      case "procedures":
        return deferred(
          "Procedures",
          "Server-backed procedure and progress screens are the next Phase 17 slice.",
        );
      case "procedure":
        return deferred(
          "Procedure",
          "Select a server-backed procedure after the procedure workflow slice is installed.",
        );
      case "instruction":
        return deferred(
          "Instruction",
          "Instruction completion is unavailable until a live procedure and revision are loaded.",
        );
      case "procedure-editor":
        return deferred(
          "Edit procedure",
          "Procedure editing and accessible reordering are not enabled in this slice.",
        );
      case "macros":
        return (
          <MacroLibraryPage
            activeSet={activeSet}
            onCreate={() => {
              navigateToMacroEditor(null);
            }}
            onEdit={(macroId) => {
              navigateToMacroEditor(macroId);
            }}
          />
        );
      case "macro-editor":
        return (
          <MacroEditorPage
            activeSet={activeSet}
            key={`${activeSet?.id ?? "none"}:${routeHash}`}
            onBack={() => {
              navigateTo("macros");
            }}
            target={macroEditorTargetFromHash()}
          />
        );
      case "confirm":
        return deferred(
          "Confirm send",
          "Execution submission is unavailable until a live macro, current revision, USB readiness, and confirmation policy are loaded.",
        );
      case "manage-sets":
        return (
          <SetSelectionPage
            onSelected={setSettings}
            sets={sets}
            settings={settings}
          />
--- MacroEditorPage.tsx lines 1-330 ---
import { useEffect, useMemo, useRef, useState, type FormEvent } from "react";
import { ApiError } from "../../api/client";
import { errorText } from "../../api/errors";
import {
  createSetMacro,
  getSetMacro,
  updateSetMacro,
  validateSetMacro,
} from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { MacroEditorTarget } from "../../routing";
import { replaceMacroEditorTarget } from "../../routing";
import { limits } from "../../types/limits";
import type {
  Macro,
  MacroParseLocation,
  MacroSet,
  MacroValidation,
} from "../../types/models";
import {
  byteOffsetToCodeUnit,
  createMacroDraft,
  macroDirectives,
  macroFingerprint,
  parseMacroLocation,
  utf8ByteLength,
  validateMacroLocally,
  validationSummary,
} from "./macroDraft";

interface MacroEditorPageProps {
  activeSet: MacroSet | null;
  target: MacroEditorTarget;
  onBack: () => void;
}

type ValidationState =
  | { kind: "idle" }
  | { kind: "pending"; fingerprint: string }
  | {
      kind: "valid";
      fingerprint: string;
      result: MacroValidation;
    }
  | {
      kind: "invalid";
      fingerprint: string;
      message: string;
      location: MacroParseLocation | null;
    }
  | { kind: "error"; fingerprint: string; message: string };

function newUuid(): string {
  return crypto.randomUUID();
}

function macroBelongsToSet(macro: Macro, setId: string): boolean {
  return macro.scope === "set" && macro.set_id === setId;
}

export function MacroEditorPage({
  activeSet,
  target,
  onBack,
}: MacroEditorPageProps): React.JSX.Element {
  const [draft, setDraft] = useState<Macro | null>(null);
  const [persisted, setPersisted] = useState(false);
  const [loading, setLoading] = useState(false);
  const [loadVersion, setLoadVersion] = useState(0);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [validation, setValidation] = useState<ValidationState>({
    kind: "idle",
  });
  const [saving, setSaving] = useState(false);
  const [saveMessage, setSaveMessage] = useState<string | null>(null);
  const [conflict, setConflict] = useState(false);
  const sourceRef = useRef<HTMLTextAreaElement>(null);
  const pendingSelection = useRef<number | null>(null);

  const targetMacroId = target.kind === "edit" ? target.macroId : null;

  useEffect(() => {
    if (activeSet === null || target.kind === "invalid") {
      setDraft(null);
      setPersisted(false);
      setLoading(false);
      setLoadError(null);
      return;
    }
    setSaveMessage(null);
    setConflict(false);
    setLoadError(null);

    if (target.kind === "create") {
      setDraft(createMacroDraft(activeSet.id, newUuid()));
      setPersisted(false);
      setLoading(false);
      return;
    }

    let active = true;
    setDraft(null);
    setLoading(true);
    void getSetMacro(activeSet.id, target.macroId)
      .then((loaded) => {
        if (!macroBelongsToSet(loaded, activeSet.id)) {
          throw new Error(
            "The device returned a macro outside the active set.",
          );
        }
        if (active) {
          setDraft(loaded);
          setPersisted(true);
        }
      })
      .catch((error: unknown) => {
        if (active) {
          setLoadError(errorText(error));
        }
      })
      .finally(() => {
        if (active) {
          setLoading(false);
        }
      });
    return () => {
      active = false;
    };
  }, [activeSet, target.kind, targetMacroId, loadVersion]);

  const localValidation = useMemo(
    () => (draft === null ? null : validateMacroLocally(draft)),
    [draft],
  );
  const fingerprint = draft === null ? "" : macroFingerprint(draft);

  useEffect(() => {
    if (
      activeSet === null ||
      draft === null ||
      localValidation === null ||
      !localValidation.valid ||
      conflict
    ) {
      setValidation({ kind: "idle" });
      return;
    }
    let active = true;
    setValidation({ kind: "pending", fingerprint });
    const timer = window.setTimeout(() => {
      void validateSetMacro(activeSet.id, draft)
        .then((result) => {
          if (active) {
            setValidation({ kind: "valid", fingerprint, result });
          }
        })
        .catch((error: unknown) => {
          if (!active) {
            return;
          }
          const location = parseMacroLocation(error);
          if (error instanceof ApiError && error.status === 422) {
            setValidation({
              kind: "invalid",
              fingerprint,
              message: errorText(error),
              location,
            });
          } else {
            setValidation({
              kind: "error",
              fingerprint,
              message: errorText(error),
            });
          }
        });
    }, 300);
    return () => {
      active = false;
      window.clearTimeout(timer);
    };
  }, [activeSet, conflict, draft, fingerprint, localValidation]);

  useEffect(() => {
    if (pendingSelection.current === null) {
      return;
    }
    const position = pendingSelection.current;
    pendingSelection.current = null;
    sourceRef.current?.focus();
    sourceRef.current?.setSelectionRange(position, position);
  }, [draft?.source]);

  const updateDraft = (replacement: Partial<Macro>): void => {
    setSaveMessage(null);
    setDraft((current) =>
      current === null ? current : { ...current, ...replacement },
    );
  };

  const insertDirective = (value: string): void => {
    const textarea = sourceRef.current;
    if (textarea === null || draft === null) {
      return;
    }
    const start = textarea.selectionStart;
    const end = textarea.selectionEnd;
    const next = `${draft.source.slice(0, start)}${value}${draft.source.slice(end)}`;
    pendingSelection.current = start + value.length;
    updateDraft({ source: next });
  };

  const goToError = (): void => {
    if (
      draft === null ||
      validation.kind !== "invalid" ||
      validation.location === null
    ) {
      return;
    }
    const position = byteOffsetToCodeUnit(
      draft.source,
      validation.location.byteOffset,
    );
    sourceRef.current?.focus();
    sourceRef.current?.setSelectionRange(position, position);
  };

  const canSave =
    draft !== null &&
    localValidation?.valid === true &&
    validation.kind === "valid" &&
    validation.fingerprint === fingerprint &&
    !saving &&
    !conflict;

  const submit = async (event: FormEvent<HTMLFormElement>): Promise<void> => {
    event.preventDefault();
    if (!canSave || activeSet === null || draft === null) {
      return;
    }
    setSaving(true);
    setSaveMessage(null);
    try {
      const committed = persisted
        ? await updateSetMacro(activeSet.id, draft, draft.revision)
        : await createSetMacro(activeSet.id, draft);
      const created = !persisted;
      setDraft(committed);
      setPersisted(true);
      setConflict(false);
      if (created) {
        replaceMacroEditorTarget(committed.id);
      }
      setSaveMessage(
        created
          ? `Created revision ${String(committed.revision)}.`
          : `Saved revision ${String(committed.revision)}.`,
      );
    } catch (error: unknown) {
      if (error instanceof ApiError && error.status === 409) {
        setConflict(true);
      } else {
        setSaveMessage(errorText(error));
      }
    } finally {
      setSaving(false);
    }
  };

  const resolveConflict = (): void => {
    if (persisted) {
      setLoadVersion((version) => version + 1);
      return;
    }
    setDraft(createMacroDraft(activeSet.id, newUuid()));
    setConflict(false);
  };

  if (activeSet === null) {
    return (
      <section aria-labelledby="macro-editor-title">
        <h2 id="macro-editor-title">Macro editor</h2>
        <p>Select an active macro set before creating or editing a macro.</p>
        <button onClick={onBack} type="button">
          Back to macros
        </button>
      </section>
    );
  }

  if (target.kind === "invalid") {
    return (
      <section aria-labelledby="macro-editor-title">
        <h2 id="macro-editor-title">Macro editor</h2>
        <p className="error-message" role="alert">
          The macro editor URL contains an invalid macro identifier.
        </p>
        <button onClick={onBack} type="button">
          Back to macros
        </button>
      </section>
    );
  }

  if (loading) {
    return (
      <section aria-labelledby="macro-editor-title" aria-busy="true">
        <h2 id="macro-editor-title">Macro editor</h2>
        <p role="status">Loading macro…</p>
      </section>
    );
  }

  if (loadError !== null || draft === null || localValidation === null) {
    return (
      <section aria-labelledby="macro-editor-title">
        <h2 id="macro-editor-title">Macro editor</h2>
        <ErrorBanner message={loadError ?? "Macro is unavailable."} />
        <div className="form-actions">
          {target.kind === "edit" ? (
            <button
              onClick={() => {
                setLoadVersion((version) => version + 1);
              }}
              type="button"
            >
              Retry
            </button>
          ) : null}

> esp32-macro-keyboard-webapp@0.1.0 typecheck
> tsc -b --pretty false

src/App.tsx(217,48): error TS2304: Cannot find name 'routeHash'.
src/features/macros/MacroEditorPage.tsx(26,3): error TS6133: 'utf8ByteLength' is declared but its value is never read.
src/features/macros/MacroEditorPage.tsx(276,31): error TS18047: 'activeSet' is possibly 'null'.
```
