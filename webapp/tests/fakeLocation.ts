function normalizeHash(hash: string): string {
  if (hash.length === 0) {
    return "";
  }
  return hash.startsWith("#") ? hash : `#${hash}`;
}

export function setHashSilently(hash: string): void {
  const normalized = normalizeHash(hash);
  const next = `${window.location.pathname}${window.location.search}${normalized}`;
  window.history.replaceState(null, "", next);
}

export function navigateHash(hash: string): void {
  setHashSilently(hash);
  window.dispatchEvent(new HashChangeEvent("hashchange"));
}

export function resetFakeLocation(): void {
  setHashSilently("");
}
