export default {
  extends: ["stylelint-config-standard"],
  ignoreFiles: ["dist/**", "node_modules/**"],
  rules: {
    "import-notation": "string",
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
