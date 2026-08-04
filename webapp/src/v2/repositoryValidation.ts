import { compileMacro } from "./macroCompiler";
import {
  validateRepository,
  type Repository,
  type RepositoryValidationIssue,
  type RepositoryValidationResult,
} from "./repository";

function grammarError(source: string): string | null {
  const result = compileMacro(source, { keyPressMs: 0, interKeyMs: 0 });
  return result.ok ? null : result.error.message;
}

export function validateRepositoryForUse(
  value: unknown,
): RepositoryValidationResult {
  const structural = validateRepository(value, grammarError);
  if (!structural.ok) {
    return structural;
  }

  const issues: RepositoryValidationIssue[] = [];
  structural.value.packages.forEach((pkg, packageIndex) => {
    pkg.macros.forEach((macro, macroIndex) => {
      const result = compileMacro(macro.source, {
        keyPressMs: macro.keyPressMs,
        interKeyMs: macro.interKeyMs,
      });
      if (!result.ok) {
        issues.push({
          path: `$.packages[${String(packageIndex)}].macros[${String(macroIndex)}].source`,
          code: result.error.code,
          message: result.error.message,
        });
      }
    });
  });

  return issues.length === 0 ? structural : { ok: false, issues };
}

export function createEmptyRepository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [],
  };
}
