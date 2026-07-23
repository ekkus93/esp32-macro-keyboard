interface StatusBadgeProps {
  label: string;
  state: "good" | "warning" | "bad" | "neutral";
}

export function StatusBadge({ label, state }: StatusBadgeProps): React.JSX.Element {
  return <span className={`status-badge status-${state}`}>{label}</span>;
}
