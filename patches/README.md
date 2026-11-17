# Patches for the mesa-nvk submodule (X547/mesa, branch mesa-nvk-r2)

`mesa-nvk-nvrm-gpu-error-and-sparse.patch`, apply with `git -C mesa-nvk apply ../patches/mesa-nvk-nvrm-gpu-error-and-sparse.patch`:

- `nvkmd_nvrm_va.c`: ranges allocated with `NVKMD_VA_SPARSE` get `NVOS32_ALLOC_FLAGS_SPARSE`, so an access to a part that nothing is bound to
  reads zeros and drops writes instead of faulting. Without it the first copy-engine write to an unbound tile of a D3D12 reserved texture was a
  GPU MMU fault (error 31) that killed the channel (Halo Infinite).
- `nvkmd_nvrm_ctx.c`: the flush loop looks at the channel's error notifier (status != 0 and info32 is the RM error code), gives up after
  `NVK_NVRM_TIMEOUT_MS` (default 30000, 0 = never), prints the GPU state and returns `VK_ERROR_DEVICE_LOST` instead of polling for ever.
- `nvkmd_nvrm_ctx.c` (more): one mutex per context around exec and flush (two threads in one context lost submissions), and on a fault the
  GP-ring segment that contains the GPU's fetch address is printed.
- `nvk_queue.c`: on `VK_ERROR_DEVICE_LOST` the last four pushes of the submission are printed as hex words (`NVK: push[i][d] = ...`), to be
  decoded with `nv_push_dump file.bin TURING`; `NVK_NVRM_NO_FAULT_DUMP` turns it off.
- `nvRmApi.c`: one process-wide lock around every RM ioctl (`NVK_NVRM_NO_SERIALIZE=1` turns it off). It did not cure the Halo Infinite fault.
- `nvkmd_nvrm_va.c`: a ring of the last 16384 address-range events (alloc, free, bind, unbind) printed on a fault, a count of the live
  system-memory ranges by size (printed every 512 MB of growth and on a fault) and `NVK_NVRM_VA_LOG=1` to print every alloc and bind.
  These are diagnostics for the Halo Infinite GPU fault (error 31, cause still unknown; Halo is paused).
- `nvkmd_nvrm_mem.c`: `vkGetMemoryFdKHR()` and importing a file descriptor now work for memory that was exported in the same process. Haiku has no dma-buf, so the descriptor is a fresh unlinked file and the process keeps a table of which memory object each one stands for (identity: device and inode); an import of such a descriptor in the same process returns that memory, one from another process is refused as before. Wine's shared Direct3D resources (DXVK for Unity games: Frontline 1942) call the export right after every `vkAllocateMemory()` of a shared resource, and until now that failed, so DXVK's first allocation failed and the game crashed in DXVK with a null pointer.
- `nvkmd_nvrm_ctx.c` (tracing, off by default): `NVK_NVRM_TRACE=1` prints one line on stderr for every flush, doorbell and destroy step, and one per round of the wait loop, with the time of day, thread, context, GPGet/GPPut and the semaphore value, plus the raw words of the GP ring entries the GPU still has to fetch. Made to see what a channel that stops fetching work was doing (Firefox with Zink: a channel's GPGet stays short of GPPut although the push buffer is idle and the doorbell was rung; the cause is not found). On a timeout without a channel error (and with the trace on), `nvkmd_nvrm_dump_va_events_for()` in `nvkmd_nvrm_va.c` prints the address range events that touched each pending entry's push buffer.
- `nvkmd_nvrm_ctx.c` (test only): `NVK_NVRM_TEST_WEDGE=N` skips the doorbell for flush number N of a context, so the GPU never fetches what was just queued. It makes the "GPGet short of GPPut" state on demand, to see what a process ending in that state does to the machine (nothing: a clean exit and an abort() both left the machine alone).

`mesa-nvk-haiku-driver-build-sha.patch`, apply with `git -C mesa-nvk apply ../patches/mesa-nvk-haiku-driver-build-sha.patch`:

- `nvk_instance.c`: `instance->driver_build_sha` is now set on Haiku. The build-id lookup is skipped there (`#ifndef __HAIKU__`) and the field was never written, so it held whatever the allocator had in that memory: different for every process. The field names the shader cache directory (`mesa_shader_cache_sf/<sha>/nvk_0166`) and goes into `pipelineCacheUUID` (so zink's cache id and the programs' own pipeline caches changed too). No process ever found the cache of an earlier one: every start-up compiled all its shaders again and every Splitgate session left another 90 MB directory. The value is now a SHA-1 of the driver file's path, size and modification time (a new value when the driver is replaced). Test: 100 Direct3D 12 pipelines through Wine took 448 ms the first time and about 40 ms in the next process; a game's second start wrote nothing new to the cache.
