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
16:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
55:[90mtests/error-banner.test.tsx[39m 3ms (unchanged)
75:[90msrc/api/errors.ts[39m 3ms (unchanged)
81:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
120:[90mtests/error-banner.test.tsx[39m 4ms (unchanged)


> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 49ms (unchanged)
[90mindex.html[39m 31ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 28ms (unchanged)
[90msrc/api/client.ts[39m 109ms (unchanged)
[90msrc/api/errors.ts[39m 3ms (unchanged)
[90msrc/api/guards.ts[39m 35ms (unchanged)
[90msrc/api/README.md[39m 2ms (unchanged)
[90msrc/api/routes.ts[39m 16ms (unchanged)
[90msrc/App.tsx[39m 31ms (unchanged)
[90msrc/components/AppShell.tsx[39m 9ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 1ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 9ms (unchanged)
[90msrc/features/auth/README.md[39m 3ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 12ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 13ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 7ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 3ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 5ms (unchanged)
[90msrc/features/execution/README.md[39m 5ms (unchanged)
src/features/macros/macroDraft.ts 18ms
src/features/macros/MacroEditorPage.tsx 43ms
src/features/macros/MacroLibraryPage.tsx 16ms
[90msrc/features/macros/README.md[39m 7ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 3ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 13ms (unchanged)
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 7ms (unchanged)
[90msrc/main.tsx[39m 1ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 1ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 8ms (unchanged)
[90msrc/styles.css[39m 46ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 10ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 3ms (unchanged)
[90mtests/api.test.ts[39m 27ms (unchanged)
[90mtests/app-auth.test.tsx[39m 15ms (unchanged)
[90mtests/app-execution.test.tsx[39m 18ms (unchanged)
tests/app-macros.test.tsx 19ms
[90mtests/app-routing.test.tsx[39m 6ms (unchanged)
[90mtests/app-sets.test.tsx[39m 10ms (unchanged)
[90mtests/app.test.ts[39m 1ms (unchanged)
[90mtests/appFixtures.ts[39m 6ms (unchanged)
[90mtests/error-banner.test.tsx[39m 3ms (unchanged)
[90mtests/fakeFetch.ts[39m 9ms (unchanged)
[90mtests/fakeLocation.ts[39m 3ms (unchanged)
[90mtests/guards.test.ts[39m 6ms (unchanged)
[90mtests/README.md[39m 3ms (unchanged)
[90mtests/render.tsx[39m 10ms (unchanged)
[90mtests/setup.ts[39m 6ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)

> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 39ms (unchanged)
[90mindex.html[39m 23ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 22ms (unchanged)
[90msrc/api/client.ts[39m 78ms (unchanged)
[90msrc/api/errors.ts[39m 3ms (unchanged)
[90msrc/api/guards.ts[39m 36ms (unchanged)
[90msrc/api/README.md[39m 2ms (unchanged)
[90msrc/api/routes.ts[39m 17ms (unchanged)
[90msrc/App.tsx[39m 31ms (unchanged)
[90msrc/components/AppShell.tsx[39m 10ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 2ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 9ms (unchanged)
[90msrc/features/auth/README.md[39m 3ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 13ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 13ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 7ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 3ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 4ms (unchanged)
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/macroDraft.ts[39m 15ms (unchanged)
[90msrc/features/macros/MacroEditorPage.tsx[39m 49ms (unchanged)
[90msrc/features/macros/MacroLibraryPage.tsx[39m 19ms (unchanged)
[90msrc/features/macros/README.md[39m 7ms (unchanged)
[90msrc/features/procedures/README.md[39m 3ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 3ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 14ms (unchanged)
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 7ms (unchanged)
[90msrc/main.tsx[39m 2ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 2ms (unchanged)
[90msrc/pages/README.md[39m 3ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 8ms (unchanged)
[90msrc/styles.css[39m 42ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 6ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 1ms (unchanged)
[90mtests/api.test.ts[39m 24ms (unchanged)
[90mtests/app-auth.test.tsx[39m 16ms (unchanged)
[90mtests/app-execution.test.tsx[39m 18ms (unchanged)
[90mtests/app-macros.test.tsx[39m 19ms (unchanged)
[90mtests/app-routing.test.tsx[39m 10ms (unchanged)
[90mtests/app-sets.test.tsx[39m 13ms (unchanged)
[90mtests/app.test.ts[39m 1ms (unchanged)
[90mtests/appFixtures.ts[39m 7ms (unchanged)
[90mtests/error-banner.test.tsx[39m 4ms (unchanged)
[90mtests/fakeFetch.ts[39m 8ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
[90mtests/guards.test.ts[39m 6ms (unchanged)
[90mtests/README.md[39m 3ms (unchanged)
[90mtests/render.tsx[39m 9ms (unchanged)
[90mtests/setup.ts[39m 6ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)
```

## phase17-5-macro-editor-frontend.log

```text
25:  const [runtimeError, setRuntimeError] = useState<string | null>(null);
31:      setRuntimeError(null);
44:      setRuntimeError(null);
54:      } catch (loadError: unknown) {
56:          setRuntimeError(errorText(loadError));
74:      } catch (statusError: unknown) {
76:          setRuntimeError(errorText(statusError));
95:    setRuntimeError(null);
109:    setRuntimeError(null);
112:    } catch (logoutError: unknown) {
113:      setRuntimeError(errorText(logoutError));
121:        <ErrorBanner message={runtimeError} />
122:        {runtimeError === null ? (
220:import { ApiError } from "../../api/client";
221:import { errorText } from "../../api/errors";
228:import { ErrorBanner } from "../../components/ErrorBanner";
268:  | { kind: "error"; fingerprint: string; message: string };
287:  const [loadError, setLoadError] = useState<string | null>(null);
304:      setLoadError(null);
309:    setLoadError(null);
319:      setLoadError("The macro editor URL does not identify a macro.");
328:          throw new Error(
337:      .catch((error: unknown) => {
339:          setLoadError(errorText(error));
378:        .catch((error: unknown) => {
382:          const location = parseMacroLocation(error);
383:          if (error instanceof ApiError && error.status === 422) {
387:              message: errorText(error),
392:              kind: "error",
394:              message: errorText(error),
434:  const goToError = (): void => {
481:    } catch (error: unknown) {
482:      if (error instanceof ApiError && error.status === 409) {
485:        setSaveMessage(errorText(error));
520:        <p className="error-message" role="alert">
539:  if (loadError !== null || draft === null || localValidation === null) {
543:        <ErrorBanner message={loadError ?? "Macro is unavailable."} />
579:[31m     → Unexpected console.error: An update to %s inside a test was not wrapped in act(...).
590:[31m     → Unexpected console.error: An update to %s inside a test was not wrapped in act(...).
601:[31m     → Unexpected console.error: An update to %s inside a test was not wrapped in act(...).
612:[31m     → Unexpected console.error: An update to %s inside a test was not wrapped in act(...).
634:[31m     → expected 'ESP32 Macro KeyboardLab Chromebook wo…' to contain 'Macro editor'[39m
648: [32m✓[39m tests/error-banner.test.tsx [2m([22m[2m3 tests[22m[2m)[22m[32m 32[2mms[22m[39m
657:[31m[1mError[22m: Unexpected console.error: An update to %s inside a test was not wrapped in act(...).
669:    [90m 61| [39m  [35mif[39m (consoleErrors[33m.[39mlength [33m!==[39m [34m0[39m) {
670:    [90m 62| [39m    [35mthrow[39m [35mnew[39m [33mError[39m(
672:    [90m 63| [39m      [32m`Unexpected console.error: [39m[36m${[39m[33mString[39m(consoleErrors[[34m0[39m][33m?.[39m[[34m0[39m])[36m}[39m[32m`[39m[33m,[39m
678:[31m[1mAssertionError[22m: expected 'ESP32 Macro KeyboardLab Chromebook wo…' to contain 'Macro editor'[39m
686:    [90m 65| [39m    [34mexpect[39m(document[33m.[39mbody[33m.[39mtextContent)[33m.[39m[34mtoContain[39m(expectedText)[33m;[39m
700:::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/setup.ts,title=tests/app-macros.test.tsx > server-backed macros > loads%2C validates%2C and updates an existing macro,line=62,column=11::Error: Unexpected console.error: An update to %25s inside a test was not wrapped in act(...).%0A%0AWhen testing, code that causes React state updates should be wrapped into act(...):%0A%0Aact(() => {%0A  /* fire events that update state */%0A});%0A/* assert on the output */%0A%0AThis ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act%0A ❯ tests/setup.ts:62:11%0A%0A
702:::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/setup.ts,title=tests/app-macros.test.tsx > server-backed macros > shows exact parser coordinates and keeps Save disabled,line=62,column=11::Error: Unexpected console.error: An update to %25s inside a test was not wrapped in act(...).%0A%0AWhen testing, code that causes React state updates should be wrapped into act(...):%0A%0Aact(() => {%0A  /* fire events that update state */%0A});%0A/* assert on the output */%0A%0AThis ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act%0A ❯ tests/setup.ts:62:11%0A%0A
704:::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/setup.ts,title=tests/app-macros.test.tsx > server-backed macros > inserts directives%2C validates%2C and creates a macro,line=62,column=11::Error: Unexpected console.error: An update to %25s inside a test was not wrapped in act(...).%0A%0AWhen testing, code that causes React state updates should be wrapped into act(...):%0A%0Aact(() => {%0A  /* fire events that update state */%0A});%0A/* assert on the output */%0A%0AThis ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act%0A ❯ tests/setup.ts:62:11%0A%0A
706:::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/setup.ts,title=tests/app-macros.test.tsx > server-backed macros > surfaces stale-revision conflicts without overwriting the draft,line=62,column=11::Error: Unexpected console.error: An update to %25s inside a test was not wrapped in act(...).%0A%0AWhen testing, code that causes React state updates should be wrapped into act(...):%0A%0Aact(() => {%0A  /* fire events that update state */%0A});%0A/* assert on the output */%0A%0AThis ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act%0A ❯ tests/setup.ts:62:11%0A%0A
708:::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/app-routing.test.tsx,title=tests/app-routing.test.tsx > application routing > renders authenticated route /macro-editor,line=65,column=39::AssertionError: expected 'ESP32 Macro KeyboardLab Chromebook wo…' to contain 'Macro editor'%0A%0AExpected: "Macro editor"%0AReceived: "ESP32 Macro KeyboardLab Chromebook workflowUSB readySign outLab Chromebook workflowCreate macroRevision 1Back to macrosName0 / 64 UTF-8 bytesFavoriteKey press time (ms)Inter-key time (ms)Insert directiveInsert EnterInsert TabInsert EscapeInsert Ctrl+LInsert Ctrl+Shift+TInsert Alt+F4Insert 500 ms delayInsert literal {Insert literal }Macro source0 / 4096 UTF-8 bytesCorrect the local fields before validation.Name is required.Server validationWaiting for a locally valid draft.Create macroCancelMacro syntax referencePrintable ASCII types literally. Use doubled braces for literal braces, named-key or chord directives in braces, and delays from 1 to 10000 ms.The server parser is authoritative. The editor never executes source during validation.SetsProceduresMacrosSettings"%0A%0A ❯ tests/app-routing.test.tsx:65:39%0A%0A

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
  const [routeHash, setRouteHash] = useState(() => window.location.hash);
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
      setRouteHash(window.location.hash);
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

    if (targetMacroId === null) {
      setLoadError("The macro editor URL does not identify a macro.");
      return;
    }
    let active = true;
    setDraft(null);
    setLoading(true);
    void getSetMacro(activeSet.id, targetMacroId)
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
    if (activeSet === null || draft === null || !canSave) {
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
    if (activeSet === null) {
      return;
    }
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

> esp32-macro-keyboard-webapp@0.1.0 typecheck
> tsc -b --pretty false


> esp32-macro-keyboard-webapp@0.1.0 lint
> eslint . --max-warnings=0


> esp32-macro-keyboard-webapp@0.1.0 stylelint
> stylelint 'src/**/*.css' --max-warnings=0


> esp32-macro-keyboard-webapp@0.1.0 format:check
> prettier --check .

Checking formatting...
All matched files use Prettier code style!

> esp32-macro-keyboard-webapp@0.1.0 test
> vitest run


[1m[46m RUN [49m[22m [36mv3.2.4 [39m[90m/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp[39m

 [32m✓[39m tests/api.test.ts [2m([22m[2m20 tests[22m[2m)[22m[32m 46[2mms[22m[39m
 [32m✓[39m tests/app-auth.test.tsx [2m([22m[2m7 tests[22m[2m)[22m[32m 198[2mms[22m[39m
 [31m❯[39m tests/app-macros.test.tsx [2m([22m[2m5 tests[22m[2m | [22m[31m4 failed[39m[2m)[22m[32m 261[2mms[22m[39m
   [32m✓[39m server-backed macros[2m > [22mloads the active-set macro library[32m 69[2mms[22m[39m
[31m   [31m×[31m server-backed macros[2m > [22mloads, validates, and updates an existing macro[39m[32m 85[2mms[22m[39m
[31m     → Unexpected console.error: An update to %s inside a test was not wrapped in act(...).

When testing, code that causes React state updates should be wrapped into act(...):

act(() => {
  /* fire events that update state */
});
/* assert on the output */

This ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act[39m
[31m   [31m×[31m server-backed macros[2m > [22mshows exact parser coordinates and keeps Save disabled[39m[32m 30[2mms[22m[39m
[31m     → Unexpected console.error: An update to %s inside a test was not wrapped in act(...).

When testing, code that causes React state updates should be wrapped into act(...):

act(() => {
  /* fire events that update state */
});
/* assert on the output */

This ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act[39m
[31m   [31m×[31m server-backed macros[2m > [22minserts directives, validates, and creates a macro[39m[32m 32[2mms[22m[39m
[31m     → Unexpected console.error: An update to %s inside a test was not wrapped in act(...).

When testing, code that causes React state updates should be wrapped into act(...):

act(() => {
  /* fire events that update state */
});
/* assert on the output */

This ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act[39m
[31m   [31m×[31m server-backed macros[2m > [22msurfaces stale-revision conflicts without overwriting the draft[39m[32m 44[2mms[22m[39m
[31m     → Unexpected console.error: An update to %s inside a test was not wrapped in act(...).

When testing, code that causes React state updates should be wrapped into act(...):

act(() => {
  /* fire events that update state */
});
/* assert on the output */

This ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act[39m
 [32m✓[39m tests/app-execution.test.tsx [2m([22m[2m11 tests[22m[2m)[22m[32m 203[2mms[22m[39m
 [32m✓[39m tests/app-sets.test.tsx [2m([22m[2m4 tests[22m[2m)[22m[32m 154[2mms[22m[39m
 [31m❯[39m tests/app-routing.test.tsx [2m([22m[2m21 tests[22m[2m | [22m[31m1 failed[39m[2m)[22m[32m 257[2mms[22m[39m
   [32m✓[39m application routing[2m > [22munknown routes use the set selector after authentication[32m 49[2mms[22m[39m
   [32m✓[39m application routing[2m > [22munknown routes still require a valid session[32m 7[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /sets[32m 11[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /procedures[32m 9[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /procedure[32m 20[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /instruction[32m 9[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /procedure-editor[32m 5[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /macros[32m 11[2mms[22m[39m
[31m   [31m×[31m application routing[2m > [22mrenders authenticated route /macro-editor[39m[32m 40[2mms[22m[39m
[31m     → expected 'ESP32 Macro KeyboardLab Chromebook wo…' to contain 'Macro editor'[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /confirm[32m 8[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /execution[32m 10[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /result[32m 6[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /manage-sets[32m 8[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /set-editor[32m 5[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /import[32m 6[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /export[32m 5[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /delete-set[32m 5[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /settings[32m 9[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mrenders authenticated route /diagnostics[32m 7[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mremoves the hash listener on unmount[32m 9[2mms[22m[39m
   [32m✓[39m application routing[2m > [22mclears execution polling after route change[32m 19[2mms[22m[39m
 [32m✓[39m tests/guards.test.ts [2m([22m[2m3 tests[22m[2m)[22m[32m 13[2mms[22m[39m
 [32m✓[39m tests/error-banner.test.tsx [2m([22m[2m3 tests[22m[2m)[22m[32m 32[2mms[22m[39m
 [32m✓[39m tests/app.test.ts [2m([22m[2m1 test[22m[2m)[22m[32m 10[2mms[22m[39m

[31m⎯⎯⎯⎯⎯⎯⎯[39m[1m[41m Failed Tests 5 [49m[22m[31m⎯⎯⎯⎯⎯⎯⎯[39m

[41m[1m FAIL [22m[49m tests/app-macros.test.tsx[2m > [22mserver-backed macros[2m > [22mloads, validates, and updates an existing macro
[41m[1m FAIL [22m[49m tests/app-macros.test.tsx[2m > [22mserver-backed macros[2m > [22mshows exact parser coordinates and keeps Save disabled
[41m[1m FAIL [22m[49m tests/app-macros.test.tsx[2m > [22mserver-backed macros[2m > [22minserts directives, validates, and creates a macro
[41m[1m FAIL [22m[49m tests/app-macros.test.tsx[2m > [22mserver-backed macros[2m > [22msurfaces stale-revision conflicts without overwriting the draft
[31m[1mError[22m: Unexpected console.error: An update to %s inside a test was not wrapped in act(...).

When testing, code that causes React state updates should be wrapped into act(...):

act(() => {
  /* fire events that update state */
});
/* assert on the output */

This ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act[39m
[36m [2m❯[22m tests/setup.ts:[2m62:11[22m[39m
    [90m 60| [39m  }
    [90m 61| [39m  [35mif[39m (consoleErrors[33m.[39mlength [33m!==[39m [34m0[39m) {
    [90m 62| [39m    [35mthrow[39m [35mnew[39m [33mError[39m(
    [90m   | [39m          [31m^[39m
    [90m 63| [39m      [32m`Unexpected console.error: [39m[36m${[39m[33mString[39m(consoleErrors[[34m0[39m][33m?.[39m[[34m0[39m])[36m}[39m[32m`[39m[33m,[39m
    [90m 64| [39m    )[33m;[39m

[31m[2m⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[1/5]⎯[22m[39m

[41m[1m FAIL [22m[49m tests/app-routing.test.tsx[2m > [22mapplication routing[2m > [22mrenders authenticated route /macro-editor
[31m[1mAssertionError[22m: expected 'ESP32 Macro KeyboardLab Chromebook wo…' to contain 'Macro editor'[39m

Expected: [32m"Macro editor"[39m
Received: [31m"ESP32 Macro KeyboardLab Chromebook workflowUSB readySign outLab Chromebook workflowCreate macroRevision 1Back to macrosName0 / 64 UTF-8 bytesFavoriteKey press time (ms)Inter-key time (ms)Insert directiveInsert EnterInsert TabInsert EscapeInsert Ctrl+LInsert Ctrl+Shift+TInsert Alt+F4Insert 500 ms delayInsert literal {Insert literal }Macro source0 / 4096 UTF-8 bytesCorrect the local fields before validation.Name is required.Server validationWaiting for a locally valid draft.Create macroCancelMacro syntax referencePrintable ASCII types literally. Use doubled braces for literal braces, named-key or chord directives in braces, and delays from 1 to 10000 ms.The server parser is authoritative. The editor never executes source during validation.SetsProceduresMacrosSettings"[39m

[36m [2m❯[22m tests/app-routing.test.tsx:[2m65:39[22m[39m
    [90m 63| [39m    [35mconst[39m view [33m=[39m [35mawait[39m [34mrender[39m([33m<[39m[33mApp[39m [33m/[39m[33m>[39m)[33m;[39m
    [90m 64| [39m    [35mawait[39m [34mflushReact[39m()[33m;[39m
    [90m 65| [39m    [34mexpect[39m(document[33m.[39mbody[33m.[39mtextContent)[33m.[39m[34mtoContain[39m(expectedText)[33m;[39m
    [90m   | [39m                                      [31m^[39m
    [90m 66| [39m    [35mawait[39m view[33m.[39m[34munmount[39m()[33m;[39m
    [90m 67| [39m  })[33m;[39m

[31m[2m⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[2/5]⎯[22m[39m


[2m Test Files [22m [1m[31m2 failed[39m[22m[2m | [22m[1m[32m7 passed[39m[22m[90m (9)[39m
[2m      Tests [22m [1m[31m5 failed[39m[22m[2m | [22m[1m[32m70 passed[39m[22m[90m (75)[39m
[2m   Start at [22m 00:25:17
[2m   Duration [22m 3.05s[2m (transform 418ms, setup 212ms, collect 821ms, tests 1.17s, environment 4.08s, prepare 758ms)[22m


::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/setup.ts,title=tests/app-macros.test.tsx > server-backed macros > loads%2C validates%2C and updates an existing macro,line=62,column=11::Error: Unexpected console.error: An update to %25s inside a test was not wrapped in act(...).%0A%0AWhen testing, code that causes React state updates should be wrapped into act(...):%0A%0Aact(() => {%0A  /* fire events that update state */%0A});%0A/* assert on the output */%0A%0AThis ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act%0A ❯ tests/setup.ts:62:11%0A%0A

::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/setup.ts,title=tests/app-macros.test.tsx > server-backed macros > shows exact parser coordinates and keeps Save disabled,line=62,column=11::Error: Unexpected console.error: An update to %25s inside a test was not wrapped in act(...).%0A%0AWhen testing, code that causes React state updates should be wrapped into act(...):%0A%0Aact(() => {%0A  /* fire events that update state */%0A});%0A/* assert on the output */%0A%0AThis ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act%0A ❯ tests/setup.ts:62:11%0A%0A

::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/setup.ts,title=tests/app-macros.test.tsx > server-backed macros > inserts directives%2C validates%2C and creates a macro,line=62,column=11::Error: Unexpected console.error: An update to %25s inside a test was not wrapped in act(...).%0A%0AWhen testing, code that causes React state updates should be wrapped into act(...):%0A%0Aact(() => {%0A  /* fire events that update state */%0A});%0A/* assert on the output */%0A%0AThis ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act%0A ❯ tests/setup.ts:62:11%0A%0A

::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/setup.ts,title=tests/app-macros.test.tsx > server-backed macros > surfaces stale-revision conflicts without overwriting the draft,line=62,column=11::Error: Unexpected console.error: An update to %25s inside a test was not wrapped in act(...).%0A%0AWhen testing, code that causes React state updates should be wrapped into act(...):%0A%0Aact(() => {%0A  /* fire events that update state */%0A});%0A/* assert on the output */%0A%0AThis ensures that you're testing the behavior the user would see in the browser. Learn more at https://react.dev/link/wrap-tests-with-act%0A ❯ tests/setup.ts:62:11%0A%0A

::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/app-routing.test.tsx,title=tests/app-routing.test.tsx > application routing > renders authenticated route /macro-editor,line=65,column=39::AssertionError: expected 'ESP32 Macro KeyboardLab Chromebook wo…' to contain 'Macro editor'%0A%0AExpected: "Macro editor"%0AReceived: "ESP32 Macro KeyboardLab Chromebook workflowUSB readySign outLab Chromebook workflowCreate macroRevision 1Back to macrosName0 / 64 UTF-8 bytesFavoriteKey press time (ms)Inter-key time (ms)Insert directiveInsert EnterInsert TabInsert EscapeInsert Ctrl+LInsert Ctrl+Shift+TInsert Alt+F4Insert 500 ms delayInsert literal {Insert literal }Macro source0 / 4096 UTF-8 bytesCorrect the local fields before validation.Name is required.Server validationWaiting for a locally valid draft.Create macroCancelMacro syntax referencePrintable ASCII types literally. Use doubled braces for literal braces, named-key or chord directives in braces, and delays from 1 to 10000 ms.The server parser is authoritative. The editor never executes source during validation.SetsProceduresMacrosSettings"%0A%0A ❯ tests/app-routing.test.tsx:65:39%0A%0A
```
