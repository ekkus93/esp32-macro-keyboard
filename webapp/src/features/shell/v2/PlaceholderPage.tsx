/**
 * A stub destination for a bottom-navigation item whose real screen has not
 * been built yet (TODO_V2 V2-090: "only Macros needs to be a real
 * destination right now"). Packages (Phase 10), Snapshots (Phase 11), and
 * Settings (Phase 12) route here until their own phases land — this keeps
 * the navigation itself real and reachable without fabricating behavior
 * those phases own.
 */
export interface PlaceholderPageProps {
  title: string;
  phase: string;
}

export function PlaceholderPage({
  title,
  phase,
}: PlaceholderPageProps): React.JSX.Element {
  return (
    <section aria-labelledby="placeholder-title">
      <h2 id="placeholder-title">{title}</h2>
      <p>This screen is not built yet. It arrives with {phase}.</p>
    </section>
  );
}
