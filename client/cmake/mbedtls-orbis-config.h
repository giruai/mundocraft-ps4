#pragma once

/* PS4 entropy is supplied by the port's native CSPRNG adapter. */
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
