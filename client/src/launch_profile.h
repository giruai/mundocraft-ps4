// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
namespace ps4port {
struct LaunchProfile { bool lan=false;std::string address;std::string name="PS4Player";uint16_t port=30000; };
bool parse_profile(std::string_view text,LaunchProfile &result,std::string &error);
}
