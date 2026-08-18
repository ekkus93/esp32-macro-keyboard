// Prettier has no built-in config discovery for `plugins`, so this file must
// exist even though every other setting stays at Prettier's default. The one
// plugin sorts Tailwind class strings into Tailwind's own canonical order
// (layer, then variants, then base utilities) so ordering stops being a
// manual convention and starts being enforced by `format:check` like
// everything else (T4-2, WEBAPP_TAILWIND_TODO_2026-08-18.md). Order has no
// effect on the cascade (WEBAPP_TAILWIND_SPEC_2026-08-18.md §3 rule 3) — this
// is a readability/consistency change only, never a rendering one.
//
// `tailwindStylesheet` points the plugin at the v4 CSS-first entry point
// (`@theme`, custom utilities) instead of a `tailwind.config.js`, which this
// project doesn't have.
//
// Pinned to 0.7.2, not the npm `latest` tag (0.8.1): 0.7.3+ throws
// `TypeError: e.charAt is not a function` for every TS/TSX file under
// Prettier 3.6.2 (tailwindlabs/prettier-plugin-tailwindcss#456, a known
// regression, confirmed reproduced here and fixed by this same pin).
export default {
  plugins: ["prettier-plugin-tailwindcss"],
  tailwindStylesheet: "./src/styles.css",
};
