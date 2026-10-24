# SPDX-License-Identifier: MIT
set(CURL_INCLUDE_DIR "${PORT_ROOT}/.local/client-tls/sources/curl-8.23.0/include")
set(CURL_INCLUDE_DIRS "${CURL_INCLUDE_DIR}")
set(CURL_LIBRARY "${PORT_ROOT}/client-deps/lib/libcurl.a")
set(CURL_LIBRARIES "${CURL_LIBRARY}")
set(CURL_VERSION_STRING "8.23.0")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CURL REQUIRED_VARS CURL_INCLUDE_DIR CURL_LIBRARY VERSION_VAR CURL_VERSION_STRING)
