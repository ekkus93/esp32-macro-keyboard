export default {
  extends: ["stylelint-config-standard"],
  ignoreFiles: ["dist/**", "node_modules/**"],
  rules: {
    "import-notation": "string",
    // Tailwind v4 theme variables are namespaced with a double hyphen for
    // sub-properties (`--text-title-lg--line-height`, `--text-*--font-weight`).
    // That is Tailwind's own grammar, so the pattern is kebab-case with one
    // optional `--kebab-case` suffix -- not a general relaxation: `--fooBar`,
    // `--foo_bar`, and `--foo---bar` are still rejected.
    "custom-property-pattern": [
      "^[a-z][a-z0-9]*(-[a-z0-9]+)*(--[a-z][a-z0-9]*(-[a-z0-9]+)*)?$",
      {
        message:
          "Expected custom property name to be kebab-case with at most one --suffix (Tailwind theme sub-variable)",
      },
    ],
    // Tailwind v4's own at-rules -- stylelint-config-standard's
    // at-rule-no-unknown otherwise flags every one of these as invalid CSS.
    "at-rule-no-unknown": [
      true,
      {
        ignoreAtRules: [
          "theme",
          "apply",
          "layer",
          "reference",
          "custom-variant",
          "utility",
          "variant",
          "source",
          "config",
          "plugin",
        ],
      },
    ],
  },
};
