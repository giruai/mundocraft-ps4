# SPDX-License-Identifier: MIT
# Preserve the paired bundle's toolchain files; use the same overlay revision rebuilt locally.
get_filename_component(_port "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(OO_PS4_TOOLCHAIN "${_port}/orbis-sdk-v1/sdk" CACHE PATH "Paired SDK")
set(ORBIS_COMPAT_DIR "${_port}/sources/compat" CACHE PATH "Rebuilt overlay")
set(ENV{OO_PS4_TOOLCHAIN} "${OO_PS4_TOOLCHAIN}")
set(ENV{ORBIS_COMPAT_DIR} "${ORBIS_COMPAT_DIR}")
include("${_port}/orbis-sdk-v1/orbis-compat/cmake/ps4-openorbis.cmake")
# Distribution builds must not record the builder's home directory in assertions,
# macros or debug information. Preserve all ABI flags from the paired toolchain.
set(_source "$ENV{REPO_ROOT}")
set(_maps "-ffile-prefix-map=${_port}=/build/mundocraft -fmacro-prefix-map=${_port}=/build/mundocraft -fdebug-prefix-map=${_port}=/build/mundocraft")
if(_source)
 string(APPEND _maps " -ffile-prefix-map=${_source}=/src/mundocraft -fmacro-prefix-map=${_source}=/src/mundocraft -fdebug-prefix-map=${_source}=/src/mundocraft")
endif()
string(APPEND CMAKE_C_FLAGS_INIT " ${_maps} -g0")
string(APPEND CMAKE_CXX_FLAGS_INIT " ${_maps} -g0")
