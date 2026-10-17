import type { Repository } from "./repository";

/**
 * The in-memory repository working copy and dirty-state tracker, per
 * SPEC_V2 §8.6 and TODO_V2 V2-071.
 *
 * This store holds its state only in a JavaScript closure — never in the
 * browser's Web Storage APIs, IndexedDB, Cache Storage, a service worker, or
 * any other browser persistence mechanism (SPEC_V2 §8.6, TODO_V2 V2-072). A
 * caller (a later phase's React provider) is expected to hold one instance
 * for the lifetime of an authenticated tab, which is what makes a dirty
 * working copy survive in-tab reauthentication: nothing in this module's API
 * resets state as a side effect of authentication, so as long as the
 * instance itself is not discarded, dirty content is preserved.
 *
 * By construction, actions that must never dirty the repository — package
 * selection, sending a macro, cancellation, snapshot deletion, and UI
 * preference changes — have no corresponding mutator here at all: this
 * module exposes no operation those actions could call.
 */
export interface RepositoryWorkingCopySnapshot {
  repository: Repository;
  dirty: boolean;
}

export interface RepositorySaveToken {
  /** Changes for every working-copy state transition. */
  revision: number;
  /** Changes when the user deliberately discards or replaces the copy. */
  lineage: number;
}

export interface RepositoryWorkingCopyStore {
  /** The current working copy (edited or not). */
  getRepository: () => Repository;
  /** The last loaded-or-saved baseline (what "Discard changes" reverts to). */
  getBaseline: () => Repository;
  getIsDirty: () => boolean;
  getSnapshot: () => RepositoryWorkingCopySnapshot;
  getSaveToken: () => RepositorySaveToken;
  isSaveLineageCurrent: (token: RepositorySaveToken) => boolean;
  subscribe: (
    listener: (snapshot: RepositoryWorkingCopySnapshot) => void,
  ) => () => void;

  /**
   * Records a package/macro content or order change (create, edit,
   * duplicate, delete, move, reorder). Always dirties the working copy.
   */
  applyContentChange: (repository: Repository) => void;

  /**
   * Records a completed repository import. Per SPEC_V2 §8.8, import replaces
   * the working copy content and dirties it — it does not upload a snapshot.
   */
  applyImport: (repository: Repository) => void;

  /**
   * Clears dirty state after a snapshot upload succeeds with `201 Created`.
   * The saved repository becomes the new baseline and working copy.
   */
  markSaved: (
    savedRepository: Repository,
    token?: RepositorySaveToken,
  ) => "current" | "edited" | "superseded";

  /**
   * Deliberately discards unsaved edits, reverting the working copy to the
   * current baseline. Dirty becomes false.
   */
  discardChanges: () => void;

  /**
   * Deliberately replaces both baseline and working copy (for example,
   * loading a different stored snapshot). Dirty becomes false: this is a
   * fresh, already-persisted baseline, not an edit.
   */
  replaceWorkingCopy: (repository: Repository) => void;
}

export function createRepositoryWorkingCopyStore(
  initial: Repository,
): RepositoryWorkingCopyStore {
  let baseline = initial;
  let working = initial;
  let dirty = false;
  let revision = 0;
  let lineage = 0;
  const listeners = new Set<
    (snapshot: RepositoryWorkingCopySnapshot) => void
  >();

  function snapshot(): RepositoryWorkingCopySnapshot {
    return { repository: working, dirty };
  }

  function notify(): void {
    const current = snapshot();
    listeners.forEach((listener) => {
      listener(current);
    });
  }

  function setWorking(repository: Repository, nextDirty: boolean): void {
    working = repository;
    dirty = nextDirty;
    notify();
  }

  return {
    getRepository: () => working,
    getBaseline: () => baseline,
    getIsDirty: () => dirty,
    getSnapshot: snapshot,
    getSaveToken: () => ({ revision, lineage }),
    isSaveLineageCurrent: (token) => token.lineage === lineage,
    subscribe: (listener) => {
      listeners.add(listener);
      return () => {
        listeners.delete(listener);
      };
    },
    applyContentChange: (repository) => {
      revision += 1;
      setWorking(repository, true);
    },
    applyImport: (repository) => {
      revision += 1;
      setWorking(repository, true);
    },
    markSaved: (savedRepository, token = { revision, lineage }) => {
      if (token.lineage !== lineage) {
        return "superseded";
      }
      baseline = savedRepository;
      if (token.revision === revision) {
        working = savedRepository;
        dirty = false;
        revision += 1;
        notify();
        return "current";
      }
      dirty = true;
      revision += 1;
      notify();
      return "edited";
    },
    discardChanges: () => {
      revision += 1;
      lineage += 1;
      working = baseline;
      dirty = false;
      notify();
    },
    replaceWorkingCopy: (repository) => {
      revision += 1;
      lineage += 1;
      baseline = repository;
      working = repository;
      dirty = false;
      notify();
    },
  };
}
