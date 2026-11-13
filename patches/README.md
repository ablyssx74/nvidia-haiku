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
