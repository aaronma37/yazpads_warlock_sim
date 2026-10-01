// Browser APIs can remain pending (notably adapter/device startup). Bound each
// stage and retain rejection handlers after timeout so late failures are handled.
export function runStage(stage, operation, {
  signal, timeoutMs = 30000, onProgress = () => {}, onLateValue = () => {},
} = {}) {
  return new Promise((resolve, reject) => {
    let settled = false;
    let timer;
    const cleanup = () => {
      clearTimeout(timer);
      signal?.removeEventListener('abort', abort);
    };
    const fail = error => {
      if (settled) return;
      settled = true;
      cleanup();
      reject(error);
    };
    const abort = () => fail(signal.reason ?? new DOMException('Stopped', 'AbortError'));
    if (signal?.aborted) { abort(); return; }
    signal?.addEventListener('abort', abort, {once: true});
    timer = setTimeout(() => {
      const error = new Error(`${stage} timed out after ${timeoutMs / 1000}s. The browser did not complete this stage. Reload and check browser GPU support if this persists.`);
      error.name = 'TimeoutError';
      fail(error);
    }, timeoutMs);
    Promise.resolve().then(() => {
      if (settled) return;
      onProgress(stage);
      return operation();
    }).then(value => {
      if (settled) { if (value !== undefined) onLateValue(value); return; }
      settled = true;
      cleanup();
      resolve(value);
    }, error => fail(error)).catch(() => {});
  });
}
