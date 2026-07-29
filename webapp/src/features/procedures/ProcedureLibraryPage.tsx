import { useEffect, useMemo, useState } from "react";
import { errorText } from "../../api/errors";
import { listSetProcedures } from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { MacroSet, ProcedureSummary } from "../../types/models";
import {
  loadProcedureProgressState,
  procedureProgressLabel,
  summaryProgressIssue,
  type ProcedureProgressState,
} from "./procedureState";

interface ProcedureLibraryPageProps {
  activeSet: MacroSet | null;
  onOpen: (procedureId: string) => void;
}

type ProgressResult =
  | { kind: "loaded"; state: ProcedureProgressState }
  | { kind: "error"; message: string };

interface ProcedureCard {
  summary: ProcedureSummary;
  progress: ProgressResult;
}

export function ProcedureLibraryPage({
  activeSet,
  onOpen,
}: ProcedureLibraryPageProps): React.JSX.Element {
  const [cards, setCards] = useState<ProcedureCard[] | null>(null);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [loadVersion, setLoadVersion] = useState(0);
  const [query, setQuery] = useState("");

  useEffect(() => {
    if (activeSet === null) {
      setCards(null);
      setLoadError(null);
      return;
    }

    let active = true;
    setCards(null);
    setLoadError(null);
    const load = async (): Promise<void> => {
      try {
        const summaries = await listSetProcedures(activeSet.id);
        const loadedCards = await Promise.all(
          summaries.map(async (summary): Promise<ProcedureCard> => {
            try {
              const state = await loadProcedureProgressState(
                activeSet.id,
                summary.id,
              );
              const issue = summaryProgressIssue(summary, activeSet.id, state);
              return issue === null
                ? { summary, progress: { kind: "loaded", state } }
                : { summary, progress: { kind: "error", message: issue } };
            } catch (error: unknown) {
              return {
                summary,
                progress: { kind: "error", message: errorText(error) },
              };
            }
          }),
        );
        if (active) {
          setCards(loadedCards);
        }
      } catch (error: unknown) {
        if (active) {
          setLoadError(errorText(error));
        }
      }
    };
    void load();
    return () => {
      active = false;
    };
  }, [activeSet, loadVersion]);

  const visibleCards = useMemo(() => {
    if (cards === null) {
      return [];
    }
    const normalized = query.trim().toLowerCase();
    if (normalized.length === 0) {
      return cards;
    }
    return cards.filter(
      ({ summary }) =>
        summary.name.toLowerCase().includes(normalized) ||
        summary.description.toLowerCase().includes(normalized),
    );
  }, [cards, query]);

  if (activeSet === null) {
    return (
      <section aria-labelledby="procedure-library-title">
        <h2 id="procedure-library-title">Procedures</h2>
        <p>Select an active macro set before loading procedures.</p>
      </section>
    );
  }

  return (
    <section aria-labelledby="procedure-library-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Active set</p>
          <h2 id="procedure-library-title">Procedures</h2>
          <p>{activeSet.name}</p>
        </div>
      </div>

      <label className="form-stack" htmlFor="procedure-search">
        Search procedures
        <input
          id="procedure-search"
          onChange={(event) => {
            setQuery(event.currentTarget.value);
          }}
          placeholder="Name or description"
          type="search"
          value={query}
        />
      </label>

      <ErrorBanner message={loadError} />
      {loadError !== null ? (
        <button
          onClick={() => {
            setLoadVersion((version) => version + 1);
          }}
          type="button"
        >
          Retry
        </button>
      ) : null}
      {cards === null && loadError === null ? (
        <p aria-busy="true" role="status">
          Loading procedures and progress…
        </p>
      ) : null}
      {cards !== null && visibleCards.length === 0 ? (
        <p role="status">
          {cards.length === 0
            ? "This set has no procedures yet."
            : "No procedures match the search."}
        </p>
      ) : null}

      <div aria-label="Procedure list">
        {visibleCards.map(({ summary, progress }) => (
          <article className="card" key={summary.id}>
            <div>
              <h3>{summary.name}</h3>
              <p>{summary.description}</p>
              <p>
                Revision {String(summary.revision)} ·{" "}
                {String(summary.step_count)} steps
              </p>
              {progress.kind === "loaded" ? (
                <p>{procedureProgressLabel(summary, progress.state)}</p>
              ) : (
                <p className="error-message" role="alert">
                  Progress unavailable: {progress.message}
                </p>
              )}
            </div>
            <button
              disabled={progress.kind === "error"}
              onClick={() => {
                onOpen(summary.id);
              }}
              type="button"
            >
              Open procedure
            </button>
          </article>
        ))}
      </div>
    </section>
  );
}
