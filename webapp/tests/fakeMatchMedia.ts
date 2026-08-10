import { vi } from "vitest";

/**
 * A minimal, controllable `window.matchMedia` fake. jsdom does not
 * implement `matchMedia` at all, so any code under test that reads it
 * (`useLandscapePhoneBlock`, TODO_V2 V2-131) needs this installed first.
 * Each distinct query string gets its own independent `matches` state and
 * `change` listener set, matching real `MediaQueryList` semantics closely
 * enough for these tests: `set(query, matches)` updates state and notifies
 * every listener registered via `addEventListener("change", ...)`.
 */
export interface FakeMatchMedia {
  set: (query: string, matches: boolean) => void;
}

type ChangeListener = (event: MediaQueryListEvent) => void;

interface FakeEntry {
  matches: boolean;
  listeners: Set<ChangeListener>;
}

interface FakeMediaQueryList {
  readonly matches: boolean;
  readonly media: string;
  addEventListener: (type: "change", listener: ChangeListener) => void;
  removeEventListener: (type: "change", listener: ChangeListener) => void;
}

export function installFakeMatchMedia(): FakeMatchMedia {
  const entries = new Map<string, FakeEntry>();

  const entryFor = (query: string): FakeEntry => {
    let entry = entries.get(query);
    if (entry === undefined) {
      entry = { matches: false, listeners: new Set() };
      entries.set(query, entry);
    }
    return entry;
  };

  const matchMedia = (query: string): MediaQueryList => {
    const entry = entryFor(query);
    const list: FakeMediaQueryList = {
      get matches() {
        return entry.matches;
      },
      media: query,
      addEventListener: (_type, listener) => {
        entry.listeners.add(listener);
      },
      removeEventListener: (_type, listener) => {
        entry.listeners.delete(listener);
      },
    };
    return list as unknown as MediaQueryList;
  };

  vi.stubGlobal("matchMedia", matchMedia);

  return {
    set: (query, matches) => {
      const entry = entryFor(query);
      if (entry.matches === matches) {
        return;
      }
      entry.matches = matches;
      const event = { matches } as MediaQueryListEvent;
      for (const listener of entry.listeners) {
        listener(event);
      }
    },
  };
}
