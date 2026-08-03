import { useMemo, useState } from "react";
import { errorText } from "../../api/errors";
import { selectPackage } from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { MacroPackage, Settings } from "../../types/models";

const recentPackagesKey = "esp32-macro-keyboard.recent-package-ids";
const maximumRecents = 5;

function readRecentPackageIds(): string[] {
  try {
    const raw = window.localStorage.getItem(recentPackagesKey);
    const value: unknown = raw === null ? [] : JSON.parse(raw);
    return Array.isArray(value) &&
      value.every((item) => typeof item === "string")
      ? value.slice(0, maximumRecents)
      : [];
  } catch {
    return [];
  }
}

function recordRecentPackage(packageId: string): void {
  const recent = readRecentPackageIds().filter((id) => id !== packageId);
  recent.unshift(packageId);
  window.localStorage.setItem(
    recentPackagesKey,
    JSON.stringify(recent.slice(0, maximumRecents)),
  );
}

function searchableText(pkg: MacroPackage): string {
  return pkg.name.toLocaleLowerCase();
}

interface PackageSelectionPageProps {
  packages: readonly MacroPackage[];
  settings: Settings;
  onManage: () => void;
  onSelected: (settings: Settings) => void;
}

export function PackageSelectionPage({
  packages,
  settings,
  onManage,
  onSelected,
}: PackageSelectionPageProps): React.JSX.Element {
  const [query, setQuery] = useState("");
  const [recentIds, setRecentIds] = useState(readRecentPackageIds);
  const [selectingId, setSelectingId] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  const visiblePackages = useMemo(() => {
    const normalized = query.trim().toLocaleLowerCase();
    const recentPosition = new Map(
      recentIds.map((id, index) => [id, index] as const),
    );
    return [...packages]
      .filter(
        (pkg) =>
          normalized.length === 0 || searchableText(pkg).includes(normalized),
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
        /* The device returns packages in index order (SPEC 12.3); preserve it. */
        return 0;
      });
  }, [query, recentIds, packages]);

  const choose = async (pkg: MacroPackage): Promise<void> => {
    setSelectingId(pkg.id);
    setError(null);
    try {
      const committed = await selectPackage(pkg.id, settings.revision);
      recordRecentPackage(pkg.id);
      setRecentIds(readRecentPackageIds());
      onSelected(committed);
    } catch (selectionError: unknown) {
      setError(errorText(selectionError));
    } finally {
      setSelectingId(null);
    }
  };

  return (
    <section aria-labelledby="package-selection-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Persisted device state</p>
          <h2 id="package-selection-title">Choose a macro package</h2>
        </div>
        <button onClick={onManage} type="button">
          Manage packages
        </button>
      </div>
      <label className="form-stack" htmlFor="package-search">
        Search packages
        <input
          id="package-search"
          onChange={(event) => {
            setQuery(event.currentTarget.value);
          }}
          placeholder="Name, manufacturer, model, board, or purpose"
          type="search"
          value={query}
        />
      </label>
      <ErrorBanner message={error} />
      {visiblePackages.length === 0 ? (
        <p role="status">No macro packages match this search.</p>
      ) : (
        <div aria-live="polite">
          {visiblePackages.map((pkg) => {
            const active = settings.activePackageId === pkg.id;
            return (
              <article className="card" key={pkg.id}>
                <div>
                  <div className="management-title-row">
                    <h3>{pkg.name}</h3>
                    {active ? (
                      <span className="status-badge status-good">
                        Active package
                      </span>
                    ) : null}
                  </div>
                  <dl className="metadata">
                    <dt>Revision</dt>
                    <dd>{String(pkg.revision)}</dd>
                  </dl>
                </div>
                <button
                  className={active ? "primary" : ""}
                  disabled={selectingId !== null || active}
                  onClick={() => {
                    void choose(pkg);
                  }}
                  type="button"
                >
                  {active
                    ? "Active"
                    : selectingId === pkg.id
                      ? "Selecting…"
                      : "Use this package"}
                </button>
              </article>
            );
          })}
        </div>
      )}
    </section>
  );
}
