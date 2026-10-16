import { useRef, useState } from "react";
import { FieldHelp } from "../../../components/FieldHelp";
import { Button } from "../../../components/m3/Button";
import { Icon } from "../../../components/m3/Icon";
import { v2Limits } from "../../../v2/limits";
import {
  allNamedKeys,
  literalEscapes,
  modifierKeys,
  specialKeyGroups,
  validateCombo,
} from "./macroInsertions";
import type { ComboKey, ModifierToken } from "./macroInsertions";

/**
 * The key and directive picker: a focused view the editor opens so the whole
 * macro vocabulary fits a phone (UXR-042). Every control inserts macro-language
 * source at the editor's cursor through `onInsert`; the picker owns no source
 * of its own and never changes it except through that call. Keys stay open for
 * several inserts in a row; `onDone` returns to the editor.
 *
 * What it offers: special keys (Tab, Enter, Esc, editing, navigation and
 * function keys), the four modifiers on their own, a combo builder that shows
 * the exact `[...]` source before inserting it, escapes for the characters the
 * language reserves, and a delay. The status line names only the *token*
 * inserted, never the person's macro source (SPEC_V2 §7: source must not reach
 * live regions, logs, or notifications).
 */
export interface MacroKeyPickerProps {
  onDone: () => void;
  onInsert: (source: string) => void;
}

const KEY_GRID =
  "grid [grid-template-columns:repeat(auto-fill,minmax(5rem,1fr))] gap-2";

function modifierToggleClass(selected: boolean): string {
  return selected
    ? "min-h-11 gap-1 bg-secondary-container px-3 text-on-secondary-container"
    : "min-h-11 gap-1 border border-outline bg-transparent px-3 text-on-surface";
}

export function MacroKeyPicker({
  onDone,
  onInsert,
}: MacroKeyPickerProps): React.JSX.Element {
  const [status, setStatus] = useState("");
  const [comboModifiers, setComboModifiers] = useState<ModifierToken[]>([]);
  // Each added key gets an id so the list has a stable React identity even
  // when the same key is added twice (which the compiler then rejects with its
  // own message, shown beside the preview).
  const [comboEntries, setComboEntries] = useState<
    { id: number; key: ComboKey }[]
  >([]);
  const nextKeyId = useRef(0);
  const comboKeys = comboEntries.map((entry) => entry.key);
  const [character, setCharacter] = useState("");
  const [delayMs, setDelayMs] = useState(500);

  const insert = (source: string, announcement: string): void => {
    onInsert(source);
    setStatus(announcement);
  };

  const combo = validateCombo(comboModifiers, comboKeys);
  const delayValid =
    Number.isSafeInteger(delayMs) &&
    delayMs >= 1 &&
    delayMs <= v2Limits.delayDirectiveMaxMs;

  const toggleModifier = (token: ModifierToken): void => {
    setComboModifiers((current) =>
      current.includes(token)
        ? current.filter((candidate) => candidate !== token)
        : [...current, token],
    );
  };
  const addKey = (key: ComboKey): void => {
    nextKeyId.current += 1;
    const id = nextKeyId.current;
    setComboEntries((current) => [...current, { id, key }]);
  };
  const keyLabel = (key: ComboKey): string =>
    key.kind === "character"
      ? key.character
      : (allNamedKeys.find((named) => named.token === key.token)?.label ??
        key.token);

  return (
    <section aria-labelledby="macro-key-picker-title">
      <div className="mb-2 flex items-center justify-between gap-3">
        <h2 className="m-0" id="macro-key-picker-title">
          Insert into macro
        </h2>
        <Button icon="check" onClick={onDone} variant="filled">
          Done
        </Button>
      </div>
      <p
        className="mt-0 mb-4 min-h-5 text-body-md text-on-surface-variant"
        role="status"
      >
        {status === "" ? "Added at the cursor in Macro source." : status}
      </p>

      <div className="grid gap-6">
        {specialKeyGroups.map((group) => (
          <section aria-label={group.label} key={group.label}>
            <h3>{group.label}</h3>
            <div className={KEY_GRID}>
              {group.keys.map((key) => (
                <button
                  aria-label={`Insert ${key.label} key`}
                  key={key.token}
                  onClick={() => {
                    insert(`{${key.token}}`, `Inserted {${key.token}}.`);
                  }}
                  type="button"
                >
                  {key.label}
                </button>
              ))}
            </div>
          </section>
        ))}

        <section aria-label="Modifier keys">
          <h3>Modifier keys</h3>
          <div className={KEY_GRID}>
            {modifierKeys.map((modifier) => (
              <button
                aria-label={`Insert ${modifier.label} key`}
                key={modifier.token}
                onClick={() => {
                  insert(
                    `{${modifier.token}}`,
                    `Inserted {${modifier.token}}.`,
                  );
                }}
                type="button"
              >
                {modifier.label}
              </button>
            ))}
          </div>
        </section>

        <section aria-label="Key combination">
          <h3>Key combination</h3>
          <FieldHelp as="p">
            Keys pressed together, such as Ctrl + Shift + T.
          </FieldHelp>
          <div
            aria-label="Modifiers to hold"
            className="mt-3 flex flex-wrap gap-2"
            role="group"
          >
            {modifierKeys.map((modifier) => {
              const selected = comboModifiers.includes(modifier.token);
              return (
                <button
                  aria-pressed={selected}
                  className={modifierToggleClass(selected)}
                  key={modifier.token}
                  onClick={() => {
                    toggleModifier(modifier.token);
                  }}
                  type="button"
                >
                  {selected ? <Icon name="check" size={16} /> : null}
                  {modifier.label}
                </button>
              );
            })}
          </div>

          <div className="mt-4 grid gap-3">
            <div className="flex items-end gap-2">
              <div className="w-24">
                <label htmlFor="macro-combo-character">Key</label>
                <input
                  autoComplete="off"
                  id="macro-combo-character"
                  maxLength={1}
                  onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                    setCharacter(event.target.value);
                  }}
                  spellCheck="false"
                  value={character}
                />
              </div>
              <button
                disabled={character === ""}
                onClick={() => {
                  addKey({ kind: "character", character });
                  setCharacter("");
                }}
                type="button"
              >
                Add key
              </button>
            </div>
            <div>
              <label htmlFor="macro-combo-special">Or add a special key</label>
              <select
                id="macro-combo-special"
                onChange={(event: React.ChangeEvent<HTMLSelectElement>) => {
                  if (event.target.value !== "") {
                    addKey({ kind: "named", token: event.target.value });
                  }
                }}
                value=""
              >
                <option value="">Choose a key…</option>
                {specialKeyGroups.map((group) => (
                  <optgroup key={group.label} label={group.label}>
                    {group.keys.map((key) => (
                      <option key={key.token} value={key.token}>
                        {key.label}
                      </option>
                    ))}
                  </optgroup>
                ))}
              </select>
            </div>
          </div>

          {comboEntries.length > 0 ? (
            <ul
              aria-label="Keys in this combination"
              className="m-0 mt-3 flex list-none flex-wrap gap-2 p-0"
            >
              {comboEntries.map((entry) => (
                <li key={entry.id}>
                  <button
                    aria-label={`Remove ${keyLabel(entry.key)}`}
                    className="min-h-11 gap-1 bg-surface-container-high px-3 text-on-surface"
                    onClick={() => {
                      setComboEntries((current) =>
                        current.filter((item) => item.id !== entry.id),
                      );
                    }}
                    type="button"
                  >
                    {keyLabel(entry.key)}
                    <Icon name="close" size={16} />
                  </button>
                </li>
              ))}
            </ul>
          ) : null}

          <div className="mt-4 rounded-shape-md bg-surface-container p-3">
            <p className="m-0 text-label-md text-on-surface-variant">
              This will insert
            </p>
            <code
              aria-label="Combination source"
              className="block py-1 text-body-lg"
            >
              {combo.source}
            </code>
            {combo.ok ? null : (
              <FieldHelp
                as="p"
                exceeded={comboKeys.length > 0 || comboModifiers.length > 0}
              >
                {combo.message}
              </FieldHelp>
            )}
          </div>
          <div className="mt-3">
            <Button
              disabled={!combo.ok}
              icon="add"
              onClick={() => {
                insert(combo.source, `Inserted ${combo.source}.`);
                setComboEntries([]);
                setComboModifiers([]);
              }}
              variant="filled"
            >
              Insert combination
            </Button>
          </div>
        </section>

        <section aria-label="Symbols and delay">
          <h3>Symbols and delay</h3>
          <FieldHelp as="p">
            Braces and brackets mean something in a macro. These insert the
            character itself.
          </FieldHelp>
          <div className={`mt-3 ${KEY_GRID}`}>
            {literalEscapes.map((escape) => (
              <button
                key={escape.source}
                onClick={() => {
                  insert(escape.source, `Inserted ${escape.source}.`);
                }}
                type="button"
              >
                {escape.label}
              </button>
            ))}
          </div>
          <div className="mt-4 flex items-end gap-2">
            <div className="w-36">
              <label htmlFor="macro-editor-delay-ms">Delay (ms)</label>
              <input
                aria-invalid={!delayValid}
                id="macro-editor-delay-ms"
                max={v2Limits.delayDirectiveMaxMs}
                min={1}
                onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                  setDelayMs(event.currentTarget.valueAsNumber);
                }}
                type="number"
                value={Number.isNaN(delayMs) ? "" : delayMs}
              />
            </div>
            <button
              disabled={!delayValid}
              onClick={() => {
                insert(
                  `{DELAY:${String(delayMs)}}`,
                  `Inserted {DELAY:${String(delayMs)}}.`,
                );
              }}
              type="button"
            >
              Insert delay
            </button>
          </div>
          <FieldHelp as="p" exceeded={!delayValid}>
            1 to {v2Limits.delayDirectiveMaxMs} ms.
          </FieldHelp>
        </section>
      </div>
    </section>
  );
}
