// The browser path loads a Priest contract explicitly, never startup Warlock refs.
export function createPriestRunController({readBuild,loadSimulation,onBusy=()=>{},onProgress=()=>{},onResult=()=>{},onStatus=()=>{}}) {
  let controller=null;
  return Object.freeze({
    isRunning:()=>controller!==null,
    cancel:()=>controller?.abort(),
    async run({detailedResults=true}={}) {
      if(controller)return;
      controller=new AbortController();const signal=controller.signal;
      try {
        const build=readBuild();onBusy(true);onStatus('Running…');
        const simulation=await loadSimulation();
        if(signal.aborted)throw new DOMException('Run cancelled.','AbortError');
        if(simulation?.id!=='priest')throw new Error('Priest simulation class identity mismatch.');
        const result=await simulation.runSimulation(build.config,{signal,detailedResults,onProgress});
        if(signal.aborted)throw new DOMException('Run cancelled.','AbortError');
        onResult(result);onStatus('Complete');return result;
      } catch(error) {onStatus(error.name==='AbortError'?'Cancelled':error.message);}
      finally {controller=null;onBusy(false);}
    }
  });
}
