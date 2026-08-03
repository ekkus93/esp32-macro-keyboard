import { useEffect, useMemo, useState } from "react";
import { errorText } from "../../api/errors";
import { listPackageMacros } from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { Macro, MacroPackage } from "../../types/models";
import { utf8ByteLength } from "./macroDraft";

interface MacroLibraryPageProps {
  activePackage: MacroPackage | null;
  onCreate: () => void;
  onEdit: (macroId: string) => void;
  onSend: (macroId: string) => void;
}

export function MacroLibraryPage({
  activePackage,
  onCreate,
  onEdit,
  onSend,
}: MacroLibraryPageProps): React.JSX.Element {
  const [macros, setMacros] = useState<Macro[] | null>(null);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [loadVersion, setLoadVersion] = useState(0);
  const [query, setQuery] = useState("");

  useEffect(() => {
    if (activePackage === null) {
      setMacros(null);
      setLoadError(null);
      return;
    }
    let active = true;
    setMacros(null);
    setLoadError(null);
    void listPackageMacros(activePackage.id)
      .then((loaded) => {
        if (active) {
          setMacros(loaded);
        }
      })
      .catch((error: unknown) => {
        if (active) {
          setLoadError(errorText(error));
        }
      });
    return () => {
      active = false;
    };
  }, [activePackage, loadVersion]);

  const visibleMacros = useMemo(() => {
    if (macros === null) {
      return [];
    }
    const normalized = query.trim().toLowerCase();
    if (normalized.length === 0) {
      return macros;
    }
    return macros.filter(
      (macro) =>
        macro.name.toLowerCase().includes(normalized) ||
        macro.source.toLowerCase().includes(normalized),
    );
  }, [macros, query]);

  if (activePackage === null) {
    return (
      <section aria-labelledby="macro-library-title">
        <h2 id="macro-library-title">Macros</h2>
        <p>
          Select an active macro package before loading or creating
          package-owned macros.
        </p>
      </section>
    );
  }

  return (
    <section aria-labelledby="macro-library-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Active package</p>
          <h2 id="macro-library-title">Macros</h2>
          <p>{activePackage.name}</p>
        </div>
        <button className="primary" onClick={onCreate} type="button">
          Create macro
        </button>
      </div>

      <label className="form-stack" htmlFor="macro-search">
        Search macros
        <input
          id="macro-search"
          onChange={(event) => {
            setQuery(event.currentTarget.value);
          }}
          placeholder="Name or source"
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
      {macros === null && loadError === null ? (
        <p aria-busy="true" role="status">
          Loading macros…
        </p>
      ) : null}
      {macros !== null && visibleMacros.length === 0 ? (
        <p role="status">
          {macros.length === 0
            ? "This package has no macros yet."
            : "No macros match the search."}
        </p>
      ) : null}

      <div aria-label="Macro list">
        {visibleMacros.map((macro) => (
          <article className="card" key={macro.id}>
            <div>
              <h3>{macro.name}</h3>
              <p>Revision {String(macro.revision)}</p>
              <p>
                {String(utf8ByteLength(macro.source))} source bytes · press{" "}
                {String(macro.key_press_ms)} ms · gap{" "}
                {String(macro.inter_key_ms)} ms
              </p>
            </div>
            <div className="card-actions">
              <button
                className="primary"
                onClick={() => {
                  onSend(macro.id);
                }}
                type="button"
              >
                Send
              </button>
              <button
                onClick={() => {
                  onEdit(macro.id);
                }}
                type="button"
              >
                Edit
              </button>
            </div>
          </article>
        ))}
      </div>
    </section>
  );
}
