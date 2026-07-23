# Macro language reference

Version 0.1 targets the US English keyboard layout and printable ASCII.

## Literal text

Printable ASCII characters are converted to USB HID usages. Line feed produces
Enter and tab produces Tab. CRLF is normalized to line feed. Other Unicode is a
validation error with a byte offset, line, and column.

Literal braces use doubled braces:

```text
{{ -> {
}} -> }
```

## Named keys

```text
{ENTER} {TAB} {ESC} {BACKSPACE} {DELETE}
{INSERT} {HOME} {END} {PAGEUP} {PAGEDOWN}
{UP} {DOWN} {LEFT} {RIGHT} {SPACE}
{F1} ... {F12}
```

## Chords

Modifiers are `CTRL`, `ALT`, `SHIFT`, and `GUI`. A chord contains one or more
unique modifiers and exactly one non-modifier key.

```text
{CTRL+L}
{CTRL+SHIFT+T}
{ALT+F4}
{GUI+R}
```

Modifier-only chords, duplicate modifiers, multiple ordinary keys, spaces inside
directives, and unknown names are errors.

## Delay

```text
{DELAY:500}
```

The inclusive range is 1 through 10,000 milliseconds. The entire compiled plan
must remain below the 300-second duration limit.

## Execution invariant

Validation and compilation finish before execution starts. A failed parse yields
no partial plan. Every key or chord is followed by a release-all report, and all
terminal paths attempt release-all again.
