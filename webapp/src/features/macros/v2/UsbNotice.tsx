import { Icon } from "../../../components/m3/Icon";
import type { UsbState } from "../../../v2/apiTypes";

/**
 * Says why Send is unavailable, next to the Send controls it disables
 * (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §5.2: "Disable Send when USB is not ready
 * and explain the state next to the control with concise actionable text").
 * Renders nothing while USB is ready. The warning and error treatments differ
 * by icon as well as colour.
 */
const NOTICE: Record<
  Exclude<UsbState, "ready">,
  { icon: "warning" | "error"; text: string }
> = {
  disconnected: {
    icon: "warning",
    text: "Not connected to a computer over USB. Connect the keyboard's USB port to the computer you want to type on.",
  },
  enumerating: {
    icon: "warning",
    text: "The computer is still setting up the keyboard. Sending unlocks in a moment.",
  },
  suspended: {
    icon: "warning",
    text: "The computer is asleep or has paused the USB link. Wake it to send macros.",
  },
  error: {
    icon: "error",
    text: "The USB connection has a problem. Unplug and reconnect the cable, then try again.",
  },
  uninitialized: {
    icon: "warning",
    text: "USB isn't ready yet. Sending is unavailable until the keyboard finishes starting.",
  },
};

export interface UsbNoticeProps {
  usbState: UsbState;
}

export function UsbNotice({
  usbState,
}: UsbNoticeProps): React.JSX.Element | null {
  if (usbState === "ready") {
    return null;
  }
  const { icon, text } = NOTICE[usbState];
  return (
    <p
      className={
        icon === "error"
          ? "mb-4 flex items-start gap-3 rounded-shape-md bg-error-container px-4 py-3 text-body-md text-on-error-container"
          : "mb-4 flex items-start gap-3 rounded-shape-md bg-warning-container px-4 py-3 text-body-md text-on-warning-container"
      }
      role="status"
    >
      <Icon name={icon} size={20} />
      <span>{text}</span>
    </p>
  );
}
