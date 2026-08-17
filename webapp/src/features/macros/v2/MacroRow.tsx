import type { RepositoryMacro } from "../../../v2/repository";
import { MacroOverflowMenu } from "./MacroOverflowMenu";
import type { MoveAction } from "./macroSendStatus";

interface MacroRowProps {
  macro: RepositoryMacro;
  index: number;
  macroCount: number;
  revealed: boolean;
  sendDisabled: boolean;
  sending: boolean;
  onToggleReveal: () => void;
  onSend: () => void;
  onEdit: () => void;
  onPreview: () => void;
  onMove: (action: MoveAction) => void;
  onDuplicate: () => void;
  onDelete: () => void;
}

export function MacroRow({
  macro,
  index,
  macroCount,
  revealed,
  sendDisabled,
  sending,
  onToggleReveal,
  onSend,
  onEdit,
  onPreview,
  onMove,
  onDuplicate,
  onDelete,
}: MacroRowProps): React.JSX.Element {
  return (
    <article className="card">
      <div>
        <h3>{macro.name}</h3>
        {revealed ? (
          <p>
            <code>{macro.source}</code>
          </p>
        ) : (
          <p>Source hidden</p>
        )}
        <button
          aria-label={
            revealed
              ? `Hide source for ${macro.name}`
              : `Reveal source for ${macro.name}`
          }
          onClick={onToggleReveal}
          type="button"
        >
          {revealed ? "Hide source" : "Reveal"}
        </button>
      </div>
      <div className="flex flex-wrap content-start gap-[0.4rem] [&_button]:flex-initial max-[32rem]:w-full">
        <button
          aria-label={sending ? `Sending ${macro.name}` : `Send ${macro.name}`}
          className="primary"
          disabled={sendDisabled}
          onClick={onSend}
          type="button"
        >
          {sending ? "Sending…" : "Send"}
        </button>
        <button
          aria-label={`Edit ${macro.name}`}
          onClick={onEdit}
          type="button"
        >
          Edit
        </button>
        <div className="grid grid-cols-2 gap-2 max-[32rem]:grid-cols-1">
          <button
            aria-label={`Move ${macro.name} to first`}
            disabled={index === 0}
            onClick={() => {
              onMove("first");
            }}
            type="button"
          >
            Move first
          </button>
          <button
            aria-label={`Move ${macro.name} up`}
            disabled={index === 0}
            onClick={() => {
              onMove("up");
            }}
            type="button"
          >
            Move up
          </button>
          <button
            aria-label={`Move ${macro.name} down`}
            disabled={index === macroCount - 1}
            onClick={() => {
              onMove("down");
            }}
            type="button"
          >
            Move down
          </button>
          <button
            aria-label={`Move ${macro.name} to last`}
            disabled={index === macroCount - 1}
            onClick={() => {
              onMove("last");
            }}
            type="button"
          >
            Move last
          </button>
        </div>
        <MacroOverflowMenu
          macro={macro}
          onDelete={onDelete}
          onDuplicate={onDuplicate}
          onPreview={onPreview}
        />
      </div>
    </article>
  );
}
