# SPDX-License-Identifier: MIT
set(APP_VERSION "01.20" CACHE STRING "PS4 client APP_VER")
target_sources(luanti PRIVATE "${PS4_PORT_SOURCE}/client/src/main.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_heap_guard.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_platform.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_input.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_resolver.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_fcntl.c" "${PS4_PORT_SOURCE}/client/src/launch_profile.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_thread_atexit.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_stat.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_sqlite.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_wchar.cpp" "${PS4_PORT_SOURCE}/client/src/ps4_semaphore_check.cpp" "${PS4_PORT_SOURCE}/src/libc_threaded.c")
target_include_directories(luanti BEFORE PRIVATE "${PORT_ROOT}/sources/SDL2/include" "${PORT_ROOT}/sources/mesa/include" "${PS4_PORT_SOURCE}/client/src" "${CMAKE_BINARY_DIR}/src" "${CMAKE_SOURCE_DIR}/src")
target_compile_definitions(luanti PRIVATE LOVE_PS4_LIBC_THREADED_OFFSET=0x0c PS4_HEAP_SERIALIZE_REAL_ALLOCATOR=1)
set_source_files_properties("${PS4_PORT_SOURCE}/client/src/main.cpp" PROPERTIES COMPILE_DEFINITIONS "PS4_APP_VERSION=\"${APP_VERSION}\"")
set_source_files_properties("${PS4_PORT_SOURCE}/client/src/ps4_wchar.cpp" PROPERTIES COMPILE_OPTIONS "-fno-builtin")
set_source_files_properties("${PS4_PORT_SOURCE}/client/src/ps4_heap_guard.cpp" PROPERTIES COMPILE_OPTIONS "-fno-builtin")
file(GLOB GALLIUM "${PORT_ROOT}/mesa-build/src/gallium/targets/dri/libgallium-*.a")
if(NOT GALLIUM)
 message(FATAL_ERROR "Locked Mesa build required")
endif()
target_link_libraries(luanti
 -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=free
 -Wl,--wrap=aligned_alloc -Wl,--wrap=posix_memalign -Wl,--wrap=valloc -Wl,--wrap=malloc_usable_size
 -Wl,--allow-multiple-definition -Wl,--wrap=stat -Wl,--wrap=lstat -Wl,--start-group
 "${PORT_ROOT}/client-deps/lib/libcurl.a"
 "${PORT_ROOT}/client-deps/lib/libmbedtls.a"
 "${PORT_ROOT}/client-deps/lib/libmbedx509.a"
 "${PORT_ROOT}/client-deps/lib/libmbedcrypto.a"
 "${PORT_ROOT}/mesa-build/src/egl/libEGL.a"
 "${PORT_ROOT}/mesa-build/src/mesa/glapi/es2api/libGLESv2.a"
 "${PORT_ROOT}/mesa-build/src/mesa/glapi/es1api/libGLESv1_CM.a"
 ${GALLIUM}
 -Wl,--whole-archive "${PORT_ROOT}/mesa-build/src/amd/vulkan/libvulkan_radeon.a" -Wl,--no-whole-archive
 "${PORT_ROOT}/mesa-build/subprojects/zlib-1.3.1/libz.a" -Wl,--end-group
 -lSceNet -lSceNetCtl -lSceGnmDriver -lSceVideoOut -lScePad -lSceAudioOut -lSceUserService -lSceSysmodule -lSceSystemService)
add_custom_command(TARGET luanti POST_BUILD COMMAND bash "${PS4_PORT_SOURCE}/scripts/verify-libc-threaded.sh" "$<TARGET_FILE:luanti>" 0x0c VERBATIM)
ps4_create_eboot(luanti)
