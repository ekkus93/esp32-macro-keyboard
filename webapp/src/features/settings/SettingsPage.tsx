import { useEffect, useState } from "react";
import { errorText } from "../../api/errors";
import { updateSettings } from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { Settings } from "../../types/models";

interface SettingsPageProps {
  settings: Settings;
  onUpdated: (settings: Settings) => void;
}

export function SettingsPage({
  settings,
  onUpdated,
}: SettingsPageProps): React.JSX.Element {
  const [requirePhysicalConfirmation, setRequirePhysicalConfirmation] =
    useState(settings.requirePhysicalConfirmation);
  const [alwaysSelectSet, setAlwaysSelectSet] = useState(
    settings.alwaysSelectSet,
  );
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    setRequirePhysicalConfirmation(settings.requirePhysicalConfirmation);
    setAlwaysSelectSet(settings.alwaysSelectSet);
  }, [settings]);

  const dirty =
    requirePhysicalConfirmation !== settings.requirePhysicalConfirmation ||
    alwaysSelectSet !== settings.alwaysSelectSet;

  const save = async (): Promise<void> => {
    setSaving(true);
    setError(null);
    try {
      const committed = await updateSettings({
        expectedRevision: settings.revision,
        requirePhysicalConfirmation,
        alwaysSelectSet,
        activeSetId: settings.activeSetId,
      });
      onUpdated(committed);
    } catch (saveError: unknown) {
      setError(errorText(saveError));
    } finally {
      setSaving(false);
    }
  };

  return (
    <section>
      <h2>Settings</h2>
      <div className="form-stack">
        <label className="checkbox-row">
          <input
            checked={alwaysSelectSet}
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setAlwaysSelectSet(event.target.checked);
            }}
            type="checkbox"
          />
          Always ask which macro set to use
        </label>
        <label className="checkbox-row">
          <input
            checked={requirePhysicalConfirmation}
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setRequirePhysicalConfirmation(event.target.checked);
            }}
            type="checkbox"
          />
          Require the device button before typing
        </label>
        <ErrorBanner message={error} />
        <button
          className="primary"
          disabled={!dirty || saving}
          onClick={() => {
            void save();
          }}
          type="button"
        >
          {saving ? "Saving…" : "Save settings"}
        </button>
      </div>
    </section>
  );
}
