import { SendStatus } from "../../../components/SendStatus";
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
    <SendStatus role={role}>
      <p>{message}</p>
      {extra}
      <button onClick={onDismiss} type="button">
        Dismiss
      </button>
    </SendStatus>
  );
}
