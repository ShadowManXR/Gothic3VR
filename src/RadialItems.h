#pragma once
#include <array>
#include <cstdint>
#include <memory>

namespace diagnostic {
inline constexpr unsigned RadialIconPixels=64;
using RadialIcon=std::array<uint32_t,RadialIconPixels*RadialIconPixels>;
struct RadialItem {
    int amount=0;
    std::shared_ptr<RadialIcon const> icon;
    bool operator==(RadialItem const& other)const{return amount==other.amount&&icon==other.icon;}
};
using RadialItems=std::array<RadialItem,10>;
// Inventory hotkeys use the printed digit, not a zero-based position. Native
// OnPlayerUseQuickSlot passes 1..9,0 unchanged to FindStackIndexByQuickSlot.
// The wheel and ControllerCommands are ordered 1..9,0 clockwise from the top.
inline constexpr unsigned RadialHotkey(unsigned sector){return (sector+1)%10;}
}
