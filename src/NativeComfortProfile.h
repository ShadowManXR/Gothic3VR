#pragma once
#include <cstdint>
namespace diagnostic {
// Game.dll CP 1.75/PUP, additionally protected by the full module SHA256.
// Complete instructions without relative operands: push esi; mov esi,ecx;
// cmp dword ptr [esi+170h],0. Native stores the same vector sent to physics.
inline constexpr unsigned char CurrentVelocityPrefix[10]{0x56,0x8b,0xf1,0x83,0xbe,0x70,0x01,0x00,0x00,0x00};
inline constexpr uint32_t CurrentVelocityRva=0xe1fd0,MouseFilterXRva=0x26690,MouseFilteredXOffset=0x218;
inline constexpr uint32_t WishedRotationRva=0x24810;
// push ebp; mov ebp,esp; and esp,-8 (complete, position independent).
inline constexpr unsigned char WishedRotationPrefix[]{0x55,0x8b,0xec,0x83,0xe4,0xf8};
inline constexpr uint32_t GroundTranslationRva=0xe2410,GroundSpeedOffset=0x158;
}
