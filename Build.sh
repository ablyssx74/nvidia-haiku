#!/bin/bash
set -e

baseDir="$PWD"
buildBaseDir="$PWD/build.$(getarch)"
installDir="$PWD/install.$(getarch)"

# Submodules (open-gpu-kernel-modules, mesa-nvk, mesa-zink, VideoStreamsWsi)
# ship as empty directories until initialized -- do it here so a fresh clone
# doesn't fail deep into the build with confusing "no such file" errors from
# inside one of them instead.
git submodule update --init --recursive

function buildProject {
	projectName="$1"
	shift
	buildDir="$buildBaseDir/$projectName"

	# Projects live directly at the repo root (e.g. ./accelerant), not under
	# a source/ subdirectory.
	cd "$baseDir/$projectName"

	meson setup "$buildDir" \
		-Dpkg_config_path="$installDir/develop/lib/pkgconfig" \
		-Dprefix="$installDir" "$@"

	ninja -C "$buildDir"
	ninja -C "$buildDir" install
}

buildProject nvrm_sdk
buildProject nvrm_cpp_sdk
buildProject nvidia_gsp
buildProject accelerant
# NvKmsTest has no corresponding project directory in this repo (or
# upstream X547/nvidia-haiku, as of this fix) -- nothing to build here yet.
buildProject VideoStreamsWsi
buildProject mesa-nvk \
	-Dpkg_config_path="/boot/data/packages/llvm-project/build-libclc.x86_64/install/share/pkgconfig:/boot/data/packages/SPIRV-LLVM-Translator/build.x86_64/install/lib/pkgconfig:/boot/data/packages/Vulkan/SPIRV-Tools/build.x86_64/install/lib/pkgconfig" \
	-Dgallium-drivers= \
	-Dvulkan-drivers=nouveau \
	-Dplatforms=wayland \
	-Dgallium-rusticl=false \
	-Degl=disabled \
	-Dglvnd=disabled \
	-Dglx=disabled \
	-Ddisplay-info=disabled
buildProject mesa-zink \
	-Dpkg_config_path="/boot/data/packages/libglvnd/build.x86_64/install/develop/lib/pkgconfig:/boot/data/packages/Vulkan/SPIRV-Tools/build.x86_64/install/lib/pkgconfig" \
	-Dplatforms=haiku \
	-Dgallium-opencl=disabled \
	-Dgallium-rusticl=false \
	-Dgallium-drivers=zink \
	-Dglx=disabled \
	-Dglvnd=enabled \
	-Degl=enabled \
	-Dvulkan-drivers= \
	-Dllvm=disabled \
	-Dshader-cache=enabled
