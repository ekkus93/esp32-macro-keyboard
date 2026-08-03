import { useEffect, useState } from "react";
import { errorText } from "../../api/errors";
import {
  factoryResetDevice,
  resetSettings,
  restartDevice,
  updateSettings,
} from "../../api/routes";
import { AccessibleDialog } from "../../components/AccessibleDialog";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { Screen } from "../../routing";
import type { Settings } from "../../types/models";

type DeviceAction = "restart" | "reset-settings" | "factory-reset" | null;

interface SettingsPageProps {
  settings: Settings;
  onUpdated: (settings: Settings) => void;
  navigate: (screen: Screen) => void;
}

function actionTitle(action: DeviceAction): string {
  switch (action) {
    case "restart":
      return "Restart device";
    case "reset-settings":
      return "Reset application settings";
    case "factory-reset":
      return "Factory reset device";
    case null:
      return "Device action";
  }
}

function actionDescription(action: DeviceAction): string {
  switch (action) {
    case "restart":
      return "Schedule a controlled restart after physical confirmation on the device.";
    case "reset-settings":
      return "Restore secure application defaults and clear the active-package selection.";
    case "factory-reset":
      return "Erase provisioning and user data, then restart into first-run setup.";
    case null:
      return "Confirm the selected device action.";
  }
}

export function SettingsPage({
  settings,
  onUpdated,
  navigate,
}: SettingsPageProps): React.JSX.Element {
  const [requirePhysicalConfirmation, setRequirePhysicalConfirmation] =
    useState(settings.requirePhysicalConfirmation);
  const [alwaysSelectPackage, setAlwaysSelectPackage] = useState(
    settings.alwaysSelectPackage,
  );
  const [saving, setSaving] = useState(false);
  const [action, setAction] = useState<DeviceAction>(null);
  const [factoryConfirmation, setFactoryConfirmation] = useState("");
  const [actionBusy, setActionBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [message, setMessage] = useState<string | null>(null);

  useEffect(() => {
    setRequirePhysicalConfirmation(settings.requirePhysicalConfirmation);
    setAlwaysSelectPackage(settings.alwaysSelectPackage);
  }, [settings]);

  const dirty =
    requirePhysicalConfirmation !== settings.requirePhysicalConfirmation ||
    alwaysSelectPackage !== settings.alwaysSelectPackage;

  const save = async (): Promise<void> => {
    setSaving(true);
    setError(null);
    setMessage(null);
    try {
      const committed = await updateSettings({
        expectedRevision: settings.revision,
        requirePhysicalConfirmation,
        alwaysSelectPackage,
      });
      onUpdated(committed);
      setMessage(`Saved settings revision ${String(committed.revision)}.`);
    } catch (saveError: unknown) {
      setError(errorText(saveError));
    } finally {
      setSaving(false);
    }
  };

  const closeAction = (): void => {
    if (actionBusy) {
      return;
    }
    setAction(null);
    setFactoryConfirmation("");
    setError(null);
  };

  const performAction = async (): Promise<void> => {
    if (
      action === null ||
      actionBusy ||
      (action === "factory-reset" && factoryConfirmation !== "FACTORY RESET")
    ) {
      return;
    }
    setActionBusy(true);
    setError(null);
    setMessage(null);
    try {
      switch (action) {
        case "restart": {
          await restartDevice();
          setMessage(
            "Restart scheduled. This page may disconnect while the device reboots.",
          );
          break;
        }
        case "reset-settings": {
          const committed = await resetSettings(settings.revision);
          onUpdated(committed);
          setMessage(
            `Application settings reset to secure defaults at revision ${String(
              committed.revision,
            )}.`,
          );
          break;
        }
        case "factory-reset": {
          await factoryResetDevice();
          setMessage(
            "Factory reset accepted. The device will restart into first-run setup.",
          );
          break;
        }
      }
      setAction(null);
      setFactoryConfirmation("");
    } catch (actionError: unknown) {
      setError(errorText(actionError));
    } finally {
      setActionBusy(false);
    }
  };

  return (
    <section aria-labelledby="settings-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Device administration</p>
          <h2 id="settings-title">Settings</h2>
          <p>
            Manage live policy, persisted data, diagnostics, and device
            lifecycle.
          </p>
        </div>
      </div>

      <ErrorBanner message={error} />
      {message === null ? null : (
        <p className="save-message" role="status" aria-live="polite">
          {message}
        </p>
      )}

      <form
        className="validation-card form-stack"
        onSubmit={(event) => {
          event.preventDefault();
          void save();
        }}
      >
        <h3>Execution policy</h3>
        <label className="checkbox-row">
          <input
            checked={alwaysSelectPackage}
            onChange={(event) => {
              setAlwaysSelectPackage(event.currentTarget.checked);
            }}
            type="checkbox"
          />
          Always ask which macro package to use
        </label>
        <label className="checkbox-row">
          <input
            checked={requirePhysicalConfirmation}
            onChange={(event) => {
              setRequirePhysicalConfirmation(event.currentTarget.checked);
            }}
            type="checkbox"
          />
          Require the device button before typing
        </label>
        <button className="primary" disabled={!dirty || saving} type="submit">
          {saving ? "Saving…" : "Save settings"}
        </button>
      </form>

      <div className="management-grid">
        <article className="validation-card">
          <h3>Macro packages</h3>
          <p>
            Create, edit, duplicate, reorder, and delete persisted packages.
          </p>
          <button
            onClick={() => {
              navigate("manage-packages");
            }}
            type="button"
          >
            Manage packages
          </button>
        </article>
        <article className="validation-card">
          <h3>Import and restore</h3>
          <p>
            Review the explicit Phase 18 transactional-operation boundaries.
          </p>
          <button
            onClick={() => {
              navigate("import");
            }}
            type="button"
          >
            Import and restore
          </button>
        </article>
        <article className="validation-card">
          <h3>Export and backup</h3>
          <p>Review export targets and secret-exclusion requirements.</p>
          <button
            onClick={() => {
              navigate("export");
            }}
            type="button"
          >
            Export and backup
          </button>
        </article>
        <article className="validation-card">
          <h3>Diagnostics</h3>
          <p>Inspect live mount health.</p>
          <button
            onClick={() => {
              navigate("diagnostics");
            }}
            type="button"
          >
            Open diagnostics
          </button>
        </article>
      </div>

      <section className="danger-zone" aria-labelledby="device-actions-title">
        <h3 id="device-actions-title">Device actions</h3>
        <p>
          These operations require deliberate confirmation. The request visibly
          waits for the configured physical device confirmation.
        </p>
        <div className="form-actions">
          <button
            onClick={() => {
              setAction("restart");
              setError(null);
              setMessage(null);
            }}
            type="button"
          >
            Restart device
          </button>
          <button
            onClick={() => {
              setAction("reset-settings");
              setError(null);
              setMessage(null);
            }}
            type="button"
          >
            Reset settings
          </button>
          <button
            className="danger"
            onClick={() => {
              setAction("factory-reset");
              setError(null);
              setMessage(null);
            }}
            type="button"
          >
            Factory reset
          </button>
        </div>
      </section>

      <AccessibleDialog
        description={actionDescription(action)}
        onClose={closeAction}
        open={action !== null}
        title={actionTitle(action)}
      >
        <div className="form-stack">
          {action === "factory-reset" ? (
            <label className="form-stack" htmlFor="factory-reset-confirmation">
              Type FACTORY RESET to continue
              <input
                autoComplete="off"
                id="factory-reset-confirmation"
                onChange={(event) => {
                  setFactoryConfirmation(event.currentTarget.value);
                }}
                value={factoryConfirmation}
              />
            </label>
          ) : null}
          {actionBusy ? (
            <div
              className="confirmation-panel"
              role="status"
              aria-live="assertive"
            >
              <strong>
                Send the <code>confirm</code> command on the device serial
                console.
              </strong>
              <p>
                The operation will not proceed unless the device accepts the
                physical confirmation before the bounded request timeout.
              </p>
            </div>
          ) : null}
          <ErrorBanner message={error} />
          <div className="form-actions">
            <button disabled={actionBusy} onClick={closeAction} type="button">
              Cancel
            </button>
            <button
              className={action === "factory-reset" ? "danger" : "primary"}
              disabled={
                actionBusy ||
                (action === "factory-reset" &&
                  factoryConfirmation !== "FACTORY RESET")
              }
              onClick={() => {
                void performAction();
              }}
              type="button"
            >
              {actionBusy
                ? "Waiting for device…"
                : `Confirm ${actionTitle(action)}`}
            </button>
          </div>
        </div>
      </AccessibleDialog>
    </section>
  );
}
