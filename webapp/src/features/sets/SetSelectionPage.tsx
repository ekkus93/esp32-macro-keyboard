import { useMemo, useState } from "react";
import { errorText } from "../../api/errors";
import { selectSet } from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { MacroSet, Settings } from "../../types/models";

const recentSetsKey = "esp32-macro-keyboard.recent-set-ids";
const maximumRecents = 5;

function readRecentSetIds(): string[] {
  try {
    const raw = window.localStorage.getItem(recentSetsKey);
    const value: unknown = raw === null ? [] : JSON.parse(raw);
    return Array.isArray(value) &&
      value.every((item) => typeof item === "string")
      ? value.slice(0, maximumRecents)
      : [];
  } catch {
    return [];
  }
}

function recordRecentSet(setId: string): void {
  const recent = readRecentSetIds().filter((id) => id !== setId);
  recent.unshift(setId);
  window.localStorage.setItem(
    recentSetsKey,
    JSON.stringify(recent.slice(0, maximumRecents)),
  );
}

function searchableText(set: MacroSet): string {
  return [set.name, set.description, set.manufacturer, set.model, set.board]
    .join(" ")
    .toLocaleLowerCase();
}

interface SetSelectionPageProps {
  sets: readonly MacroSet[];
  settings: Settings;
  onManage: () => void;
  onSelected: (settings: Settings) => void;
}

export function SetSelectionPage({
  sets,
  settings,
  onManage,
  onSelected,
}: SetSelectionPageProps): React.JSX.Element {
  const [query, setQuery] = useState("");
  const [recentIds, setRecentIds] = useState(readRecentSetIds);
  const [selectingId, setSelectingId] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  const visibleSets = useMemo(() => {
    const normalized = query.trim().toLocaleLowerCase();
    const recentPosition = new Map(
      recentIds.map((id, index) => [id, index] as const),
    );
    return [...sets]
      .filter(
        (set) =>
          normalized.length === 0 || searchableText(set).includes(normalized),
      )
      .sort((left, right) => {
        const leftRecent = recentPosition.get(left.id);
        const rightRecent = recentPosition.get(right.id);
        if (leftRecent !== undefined || rightRecent !== undefined) {
          return (
            (leftRecent ?? Number.MAX_SAFE_INTEGER) -
            (rightRecent ?? Number.MAX_SAFE_INTEGER)
          );
        }
        return left.sort_order - right.sort_order;
      });
  }, [query, recentIds, sets]);

  const choose = async (set: MacroSet): Promise<void> => {
    setSelectingId(set.id);
    setError(null);
    try {
      const committed = await selectSet(set.id, settings.revision);
      recordRecentSet(set.id);
      setRecentIds(readRecentSetIds());
      onSelected(committed);
    } catch (selectionError: unknown) {
      setError(errorText(selectionError));
    } finally {
      setSelectingId(null);
    }
  };

  return (
    <section aria-labelledby="set-selection-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Persisted device state</p>
          <h2 id="set-selection-title">Choose a macro set</h2>
        </div>
        <button onClick={onManage} type="button">
          Manage sets
        </button>
      </div>
      <label className="form-stack" htmlFor="set-search">
        Search sets
        <input
          id="set-search"
          onChange={(event) => {
            setQuery(event.currentTarget.value);
          }}
          placeholder="Name, manufacturer, model, board, or purpose"
          type="search"
          value={query}
        />
      </label>
      <ErrorBanner message={error} />
      {visibleSets.length === 0 ? (
        <p role="status">No macro sets match this search.</p>
      ) : (
        <div aria-live="polite">
          {visibleSets.map((set) => {
            const active = settings.activeSetId === set.id;
            return (
              <article className="card" key={set.id}>
                <div>
                  <div className="management-title-row">
                    <h3>{set.name}</h3>
                    {active ? (
                      <span className="status-badge status-good">
                        Active set
                      </span>
                    ) : null}
                  </div>
                  <p>{set.description || "No description"}</p>
                  <dl className="metadata">
                    <dt>Manufacturer</dt>
                    <dd>{set.manufacturer || "Not specified"}</dd>
                    <dt>Model</dt>
                    <dd>{set.model || "Not specified"}</dd>
                    <dt>Board</dt>
                    <dd>{set.board || "Not specified"}</dd>
                    <dt>Revision</dt>
                    <dd>{String(set.revision)}</dd>
                  </dl>
                </div>
                <button
                  className={active ? "primary" : ""}
                  disabled={selectingId !== null || active}
                  onClick={() => {
                    void choose(set);
                  }}
                  type="button"
                >
                  {active
                    ? "Active"
                    : selectingId === set.id
                      ? "Selecting…"
                      : "Use this set"}
                </button>
              </article>
            );
          })}
        </div>
      )}
    </section>
  );
}
