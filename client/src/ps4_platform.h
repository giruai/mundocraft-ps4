// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
namespace ps4port {
bool prepare_random();
bool secure_random(void *output, std::size_t size);
void log(const char *format, ...);
}
