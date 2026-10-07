export const API_URL =
  import.meta.env.VITE_API_URL ||
  (import.meta.env.DEV ? "http://localhost:8400" : "/api");

export async function request(path, { body, ...options } = {}) {
  const timeout = AbortSignal.timeout(20000);
  const response = await fetch(`${API_URL}${path}`, {
    ...options,
    signal: options.signal
      ? AbortSignal.any([options.signal, timeout])
      : timeout,
    ...(body !== undefined
      ? {
          body: JSON.stringify(body),
          headers: { "Content-Type": "application/json" },
        }
      : {}),
  });
  const data = await response.json();
  if (!response.ok)
    throw new Error(
      data.error?.message ||
        `Request failed (${response.status}). Please try again.`,
    );
  return data;
}
