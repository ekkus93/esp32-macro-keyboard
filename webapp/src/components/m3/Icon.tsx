/**
 * The application's one icon set: 24x24 inline SVG, 2px round stroke, drawn
 * with `currentColor` so an icon always takes its control's text colour.
 *
 * Local by construction (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §3.2): no icon font,
 * no image file, no network request. Every icon here is decorative --
 * `aria-hidden` -- because meaning is carried by the control's visible text or
 * its accessible name, never by the glyph alone (§3.3, §8). An icon-only
 * control must therefore be built with `IconButton`, which requires a label.
 */
const ICON_PATHS = {
  add: "M12 5v14M5 12h14",
  arrowDown: "M12 5v14M6 13l6 6 6-6",
  arrowUp: "M12 19V5M6 11l6-6 6 6",
  archive:
    "M4 4h16a1 1 0 0 1 1 1v3a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V5a1 1 0 0 1 1-1zM5 9v10a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V9M10 13h4",
  back: "M15 5l-7 7 7 7",
  check: "M5 12.5l4.5 4.5L19 7.5",
  checkCircle: "M12 21a9 9 0 1 0 0-18 9 9 0 0 0 0 18zM8 12.5l3 3 5-6",
  chevronRight: "M9 5l7 7-7 7",
  close: "M6 6l12 12M18 6L6 18",
  copy: "M9 9h10a1 1 0 0 1 1 1v10a1 1 0 0 1-1 1H9a1 1 0 0 1-1-1V10a1 1 0 0 1 1-1zM16 9V5a1 1 0 0 0-1-1H5a1 1 0 0 0-1 1v10a1 1 0 0 0 1 1h3",
  download: "M12 4v11M7 11l5 5 5-5M5 20h14",
  edit: "M4 20h4L19 9l-4-4L4 16v4zM13 7l4 4",
  error: "M12 21a9 9 0 1 0 0-18 9 9 0 0 0 0 18zM12 7.5v5.5M12 16.5v.01",
  eye: "M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12zM12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6z",
  eyeOff:
    "M3 3l18 18M10.6 6.1A9.6 9.6 0 0 1 12 6c6.5 0 10 6 10 6a17 17 0 0 1-3.2 3.9M6.6 7.6C3.9 9.3 2 12 2 12s3.5 6 10 6c1.6 0 3-.4 4.3-.9M9.9 9.9a3 3 0 0 0 4.2 4.2",
  info: "M12 21a9 9 0 1 0 0-18 9 9 0 0 0 0 18zM12 11v6M12 7.5v.01",
  keyboard:
    "M4 6h16a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2zM6 10h.01M10 10h.01M14 10h.01M18 10h.01M7 14h10",
  layers: "M12 3L3 8l9 5 9-5-9-5zM3 13l9 5 9-5",
  more: "M12 6h.01M12 12h.01M12 18h.01",
  play: "M8 5.5v13l11-6.5-11-6.5z",
  save: "M5 4h11l3 3v12a1 1 0 0 1-1 1H6a1 1 0 0 1-1-1V4zM8 4v5h7V4M8 20v-6h8v6",
  settings:
    "M4 7h9M17 7h3M4 17h3M11 17h9M15 9.5a2.5 2.5 0 1 0 0-5 2.5 2.5 0 0 0 0 5zM9 19.5a2.5 2.5 0 1 0 0-5 2.5 2.5 0 0 0 0 5z",
  trash:
    "M4 7h16M10 11v6M14 11v6M6 7l1 12a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1l1-12M9 7V4h6v3",
  usb: "M12 3v13M12 16a2 2 0 1 0 0 4 2 2 0 0 0 0-4zM12 10l4-3v2M12 13l-4-3V8",
  warning: "M12 4L2.5 20h19L12 4zM12 10v4.5M12 17.5v.01",
} as const;

export type IconName = keyof typeof ICON_PATHS;

export interface IconProps {
  name: IconName;
  /** Rendered size in px; the 24x24 viewBox scales. */
  size?: number;
}

export function Icon({ name, size = 24 }: IconProps): React.JSX.Element {
  return (
    <svg
      aria-hidden="true"
      className="shrink-0"
      fill="none"
      focusable="false"
      height={size}
      stroke="currentColor"
      strokeLinecap="round"
      strokeLinejoin="round"
      strokeWidth={2}
      viewBox="0 0 24 24"
      width={size}
    >
      <path d={ICON_PATHS[name]} />
    </svg>
  );
}
