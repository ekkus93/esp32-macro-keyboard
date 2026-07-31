from pathlib import Path

path = Path("webapp/tests/management-screens.test.tsx")
text = path.read_text()
old = '''    Object.defineProperty(file, "text", {
      configurable: true,
      value: async () => packageText,
    });'''
new = '''    Object.defineProperty(file, "text", {
      configurable: true,
      value: () => Promise.resolve(packageText),
    });'''
if text.count(old) != 1:
    raise SystemExit("replacement file-text test anchor changed")
text = text.replace(old, new, 1)
old = '''    expect(JSON.parse(String(call?.body))).toEqual({
      targetSetId: macroSet.id,
      expectedRevision: macroSet.revision,
      package: packageDocument,
    });'''
new = '''    const requestBody = call?.body;
    expect(typeof requestBody).toBe("string");
    if (typeof requestBody !== "string") {
      throw new Error("Replacement request body was not serialized JSON.");
    }
    expect(JSON.parse(requestBody)).toEqual({
      targetSetId: macroSet.id,
      expectedRevision: macroSet.revision,
      package: packageDocument,
    });'''
if text.count(old) != 1:
    raise SystemExit("replacement request-body test anchor changed")
path.write_text(text.replace(old, new, 1))
