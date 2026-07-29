import { useEffect, useRef, useState } from "react";

interface ConnectivityBannerProps {
  onReconnect: () => void;
}

type ConnectivityMessage = "offline" | "reconnected" | null;

export function ConnectivityBanner({
  onReconnect,
}: ConnectivityBannerProps): React.JSX.Element | null {
  const [message, setMessage] = useState<ConnectivityMessage>(() =>
    navigator.onLine ? null : "offline",
  );
  const mounted = useRef(false);

  useEffect(() => {
    mounted.current = true;
    const offline = (): void => {
      setMessage("offline");
    };
    const online = (): void => {
      setMessage("reconnected");
      onReconnect();
    };
    window.addEventListener("offline", offline);
    window.addEventListener("online", online);
    return () => {
      mounted.current = false;
      window.removeEventListener("offline", offline);
      window.removeEventListener("online", online);
    };
  }, [onReconnect]);

  useEffect(() => {
    if (!mounted.current || message !== "reconnected") {
      return;
    }
    const timer = window.setTimeout(() => {
      setMessage(null);
    }, 5_000);
    return () => {
      window.clearTimeout(timer);
    };
  }, [message]);

  if (message === null) {
    return null;
  }

  return (
    <div
      className={`connectivity-banner connectivity-${message}`}
      role="status"
      aria-live="polite"
    >
      {message === "offline"
        ? "Offline. Live reads and device changes are unavailable until the connection returns."
        : "Connection restored. Live device state is being refreshed."}
    </div>
  );
}
