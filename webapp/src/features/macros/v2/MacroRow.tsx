import { Card } from "../../../components/Card";
import { Icon } from "../../../components/m3/Icon";
import type { RepositoryMacro } from "../../../v2/repository";
import { MacroOverflowMenu } from "./MacroOverflowMenu";
import type { MoveAction } from "./macroSendStatus";

interface MacroRowProps {
  macro: RepositoryMacro;
  index: number;
  macroCount: number;
  sendDisabled: boolean;
  sending: boolean;
  onSend: () => void;
  onEdit: () => void;
  onMove: (action: MoveAction) => void;
  onDuplicate: () => void;
  onDelete: () => void;
}

/**
 * One macro: its name, a one-line source preview, and the three controls a
 * person reaches for -- Send (prominent), Edit, and a labelled "More actions"
 * menu holding reorder, Duplicate and Delete (UXR-040). The source preview is
 * a single truncated line: the list stays scannable, and the full text is one
 * tap away in the editor (UI_UX_SPEC_V2 §5.2 keeps source visible here; it
 * must never reach logs, diagnostics or send acknowledgements).
 */
export function MacroRow({
  macro,
  index,
  macroCount,
  sendDisabled,
  sending,
  onSend,
  onEdit,
  onMove,
  onDuplicate,
  onDelete,
}: MacroRowProps): React.JSX.Element {
  return (
    <Card variant="flush">
      <div className="min-w-0">
        <h3>{macro.name}</h3>
        <p className="min-w-0">
          <code className="block truncate font-mono text-body-md">
            {macro.source}
          </code>
        </p>
      </div>
      <div className="flex items-center gap-2">
        <button
          aria-label={sending ? `Sending ${macro.name}` : `Send ${macro.name}`}
          className="primary flex-1 px-5 min-[34rem]:flex-none"
          disabled={sendDisabled}
          onClick={onSend}
          type="button"
        >
          <Icon name="play" size={18} />
          {sending ? "Sending…" : "Send"}
        </button>
        <button
          aria-label={`Edit ${macro.name}`}
          className="px-4"
          onClick={onEdit}
          type="button"
        >
          <Icon name="edit" size={18} />
          Edit
        </button>
        <MacroOverflowMenu
          index={index}
          macro={macro}
          macroCount={macroCount}
          onDelete={onDelete}
          onDuplicate={onDuplicate}
          onMove={onMove}
        />
      </div>
    </Card>
  );
}
