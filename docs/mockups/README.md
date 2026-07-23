# Web Application Mockups

This directory contains reference mockups for the ESP32 Macro Keyboard web application.

## Recommended format

Store each screen as an individual SVG so it can be viewed directly in GitHub, scaled without quality loss, inspected by implementation tools, and updated independently.

Use stable, numbered filenames that follow the user workflow, for example:

- `01-login.svg`
- `02-choose-macro-set.svg`
- `03-procedures-home.svg`
- `04-procedure-execution.svg`
- `05-instruction-step.svg`
- `06-edit-procedure-reorder.svg`
- `07-macros-library.svg`
- `08-macro-editor.svg`
- `09-send-confirmation.svg`
- `10-execution-progress.svg`
- `11-step-completed.svg`
- `12-manage-macro-sets.svg`
- `13-create-macro-set.svg`
- `14-import-macro-set.svg`
- `15-export-macro-set.svg`
- `16-delete-macro-set.svg`
- `17-settings.svg`
- `18-storage-diagnostics.svg`

Each SVG should represent one screen rather than one large composite image. Keep text as text where practical, use a consistent mobile viewport, and avoid embedding remote fonts or assets.

A later `index.md` should show every screen in workflow order and describe its purpose, entry conditions, actions, success states, and error states.
