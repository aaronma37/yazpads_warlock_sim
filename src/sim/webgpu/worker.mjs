import {runStage} from './async.mjs';
self.onmessage = async ({data}) => {
  if (data.type !== 'run') return;
  const onProgress = message => postMessage({type: 'progress', message});
  try {
    const {runExperiment} = await runStage('Loading benchmark modules',
      () => import('./benchmark.mjs'), {onProgress});
    const report = await runExperiment({...data.options, onProgress});
    postMessage({type: 'result', report});
  } catch (error) {
    postMessage({type: 'error', message: error.message, name: error.name});
  }
};
