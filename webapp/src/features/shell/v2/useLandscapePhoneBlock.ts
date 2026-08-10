import { useEffect, useState } from "react";

/**
 * UI_UX_SPEC_V2 §12.4 (TODO_V2 V2-131): tested coarse-pointer, orientation,
 * and short-viewport criteria that identify a phone-sized touch device in
 * landscape, without broadening the block to tablet, laptop, or desktop
 * landscape use — those devices are typically fine-pointer (laptop/desktop)
 * or fail the height ceiling (tablets are taller in landscape than a phone).
 */
export const landscapePhoneMediaQuery =
  "(orientation: landscape) and (pointer: coarse) and (max-height: 600px)";

function matchesLandscapePhoneQuery(): boolean {
  if (
    typeof window === "undefined" ||
    typeof window.matchMedia !== "function"
  ) {
    return false;
  }
  return window.matchMedia(landscapePhoneMediaQuery).matches;
}

/**
 * Reports whether the current viewport currently matches the phone-landscape
 * block criteria. Consumers keep their normal content mounted and merely
 * hide it while this is `true` (TODO_V2 V2-131 "do not reload, clear memory,
 * restart a send, or duplicate callbacks") — this hook itself never causes
 * anything to unmount.
 */
export function useLandscapePhoneBlock(): boolean {
  const [blocked, setBlocked] = useState<boolean>(matchesLandscapePhoneQuery);

  useEffect(() => {
    if (
      typeof window === "undefined" ||
      typeof window.matchMedia !== "function"
    ) {
      return undefined;
    }
    const mediaQueryList = window.matchMedia(landscapePhoneMediaQuery);
    const update = (): void => {
      setBlocked(mediaQueryList.matches);
    };
    update();
    mediaQueryList.addEventListener("change", update);
    return () => {
      mediaQueryList.removeEventListener("change", update);
    };
  }, []);

  return blocked;
}
