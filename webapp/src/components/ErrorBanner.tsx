interface ErrorBannerProps {
  message: string | null;
}

export function ErrorBanner({
  message,
}: ErrorBannerProps): React.JSX.Element | null {
  if (message === null || message.length === 0) {
    return null;
  }
  return (
    <p className="error-message" role="alert">
      {message}
    </p>
  );
}
