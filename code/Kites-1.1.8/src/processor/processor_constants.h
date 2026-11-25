#pragma once
#include <cstdint>
constexpr uint32_t NOP = 0x00000013;
constexpr uint32_t EBREAK = 0x00100073; // ends the program: older instructions finish, younger ones do not run
constexpr uint64_t INVALID_PC = UINT64_MAX;


