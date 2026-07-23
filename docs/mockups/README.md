# Web Application Mockups

Individual screen mockups have not been committed yet. This directory reserves the
stable workflow names that will be used when the SVG and PNG assets are generated.

## Planned files

1. `01-login.svg`
2. `02-choose-macro-set.svg`
3. `03-procedures-home.svg`
4. `04-procedure-execution.svg`
5. `05-instruction-step.svg`
6. `06-edit-procedure-reorder.svg`
7. `07-macros-library.svg`
8. `08-macro-editor.svg`
9. `09-send-confirmation.svg`
10. `10-execution-progress.svg`
11. `11-step-completed.svg`
12. `12-manage-macro-sets.svg`
13. `13-create-macro-set.svg`
14. `14-import-macro-set.svg`
15. `15-export-macro-set.svg`
16. `16-delete-macro-set.svg`
17. `17-settings.svg`
18. `18-storage-diagnostics.svg`

Each screen should be stored as an individual SVG plus a same-named PNG preview.
SVG text should remain text where practical, use a consistent mobile viewport, and
must not depend on remote fonts or assets.

When the assets are added, create `index.md` with the workflow order, purpose,
entry conditions, actions, success states, and error states for every screen. Do
not reference an individual mockup from implementation documents until that file
exists in this directory.
