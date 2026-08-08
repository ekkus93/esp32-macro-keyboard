import { describe, expect, test } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import { createEmptyRepository } from "../src/v2/repositoryValidation";
import type { Repository } from "../src/v2/repository";

const canonical = canonicalRepository as Repository;

function withRenamedPackage(repository: Repository, name: string): Repository {
  const first = repository.packages[0];
  if (first === undefined) {
    throw new Error("fixture has no package");
  }
  return {
    ...repository,
    packages: [{ ...first, name }, ...repository.packages.slice(1)],
  };
}

describe("v2 repository working copy store", () => {
  test("starts clean with the initial repository as both baseline and working copy", () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    expect(store.getRepository()).toBe(canonical);
    expect(store.getBaseline()).toBe(canonical);
    expect(store.getIsDirty()).toBe(false);
  });

  test("applyContentChange dirties the working copy without touching the baseline", () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    const edited = withRenamedPackage(canonical, "Renamed package");
    store.applyContentChange(edited);
    expect(store.getRepository()).toEqual(edited);
    expect(store.getIsDirty()).toBe(true);
    expect(store.getBaseline()).toBe(canonical);
  });

  test("applyImport dirties the working copy, matching content changes", () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyImport(canonical);
    expect(store.getRepository()).toEqual(canonical);
    expect(store.getIsDirty()).toBe(true);
  });

  test("markSaved clears dirty and moves the baseline forward", () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    const edited = withRenamedPackage(canonical, "Saved package");
    store.applyContentChange(edited);
    expect(store.getIsDirty()).toBe(true);

    store.markSaved(edited);
    expect(store.getIsDirty()).toBe(false);
    expect(store.getRepository()).toEqual(edited);
    expect(store.getBaseline()).toEqual(edited);
  });

  test("a failed save (no markSaved call) leaves the working copy dirty and unchanged", () => {
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    const edited = withRenamedPackage(canonical, "Unsaved package");
    store.applyContentChange(edited);

    // Simulate a save attempt that fails: nothing calls markSaved().
    expect(store.getIsDirty()).toBe(true);
    expect(store.getRepository()).toEqual(edited);
    expect(store.getBaseline()).toEqual(createEmptyRepository());
  });

  test("discardChanges reverts the working copy to the baseline and clears dirty", () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    store.applyContentChange(withRenamedPackage(canonical, "Discard me"));
    expect(store.getIsDirty()).toBe(true);

    store.discardChanges();
    expect(store.getIsDirty()).toBe(false);
    expect(store.getRepository()).toEqual(canonical);
    expect(store.getBaseline()).toEqual(canonical);
  });

  test("replaceWorkingCopy (loading another snapshot) is a clean, non-dirtying replacement", () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    store.applyContentChange(withRenamedPackage(canonical, "Will be replaced"));
    expect(store.getIsDirty()).toBe(true);

    const replacement = createEmptyRepository();
    store.replaceWorkingCopy(replacement);
    expect(store.getIsDirty()).toBe(false);
    expect(store.getRepository()).toEqual(replacement);
    expect(store.getBaseline()).toEqual(replacement);
  });

  test("exposes no method for package selection, sends, cancellation, snapshot deletion, or UI preferences", () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    const forbidden = [
      "selectPackage",
      "send",
      "sendMacro",
      "cancel",
      "cancelSend",
      "deleteSnapshot",
      "setPreference",
      "updatePreference",
    ];
    forbidden.forEach((name) => {
      expect(name in store).toBe(false);
    });
  });

  test("a dirty working copy survives across simulated in-tab reauthentication", () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    const edited = withRenamedPackage(canonical, "Still here after reauth");
    store.applyContentChange(edited);

    // Nothing in this module's API is triggered by authentication events;
    // simulating "reauthentication" here means simply doing nothing to the
    // store, which is exactly what must happen in the real React provider.
    expect(store.getRepository()).toEqual(edited);
    expect(store.getIsDirty()).toBe(true);
  });

  test("subscribers are notified of every state transition", () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    const seen: boolean[] = [];
    const unsubscribe = store.subscribe((snapshot) => {
      seen.push(snapshot.dirty);
    });

    store.applyContentChange(withRenamedPackage(canonical, "First edit"));
    store.markSaved(withRenamedPackage(canonical, "First edit"));
    store.discardChanges();

    expect(seen).toEqual([true, false, false]);
    unsubscribe();

    store.applyContentChange(canonical);
    expect(seen).toEqual([true, false, false]);
  });

  test("getSnapshot returns a consistent repository/dirty pair", () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    store.applyContentChange(withRenamedPackage(canonical, "Snapshot check"));
    const snapshot = store.getSnapshot();
    expect(snapshot).toEqual({
      repository: withRenamedPackage(canonical, "Snapshot check"),
      dirty: true,
    });
  });
});
