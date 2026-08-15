interface DismissibleBannerProps {
  message: string;
  onDismiss: () => void;
  role: "status" | "alert";
  extra?: React.ReactNode;
}

export function DismissibleBanner({
  message,
  onDismiss,
  role,
  extra,
}: DismissibleBannerProps): React.JSX.Element {
  return (
    <div aria-live="polite" className="send-status" role={role}>
      <p>{message}</p>
      {extra}
      <button onClick={onDismiss} type="button">
        Dismiss
      </button>
    </div>
  );
}
