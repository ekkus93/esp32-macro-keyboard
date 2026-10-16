import { Icon } from "./m3/Icon";

interface ErrorBannerProps {
  message: string | null;
}

/**
 * An inline error: the error container with a leading icon, so the failure is
 * signalled by shape and text as well as colour (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE
 * §3.3). `role="alert"` announces it once when it appears.
 */
export function ErrorBanner({
  message,
}: ErrorBannerProps): React.JSX.Element | null {
  if (message === null || message.length === 0) {
    return null;
  }
  return (
    <p
      className="m-0 flex items-start gap-3 rounded-shape-md bg-error-container px-4 py-3 text-body-md text-on-error-container"
      role="alert"
    >
      <Icon name="error" size={20} />
      <span>{message}</span>
    </p>
  );
}
