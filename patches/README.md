# Patches for the mesa-nvk submodule (X547/mesa, branch mesa-nvk-r2)

`mesa-nvk-nvrm-gpu-error-and-sparse.patch`, apply with `git -C mesa-nvk apply ../patches/mesa-nvk-nvrm-gpu-error-and-sparse.patch`:

- `nvkmd_nvrm_va.c`: ranges allocated with `NVKMD_VA_SPARSE` get `NVOS32_ALLOC_FLAGS_SPARSE`, so an access to a part that nothing is bound to
  reads zeros and drops writes instead of faulting. Without it the first copy-engine write to an unbound tile of a D3D12 reserved texture was a
  GPU MMU fault (error 31) that killed the channel (Halo Infinite).
- `nvkmd_nvrm_ctx.c`: the flush loop looks at the channel's error notifier (status != 0 and info32 is the RM error code), gives up after
  `NVK_NVRM_TIMEOUT_MS` (default 30000, 0 = never), prints the GPU state and returns `VK_ERROR_DEVICE_LOST` instead of polling for ever.
