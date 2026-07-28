import { ApiError } from "./client";

export function errorText(error: unknown): string {
  if (error instanceof ApiError) {
    return `${error.body.code}: ${error.body.message}`;
  }
  if (error instanceof Error) {
    return error.message;
  }
  return "An unknown error occurred.";
}
