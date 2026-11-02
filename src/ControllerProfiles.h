#pragma once
#include <algorithm>
#include <string>
#include <vector>
namespace diagnostic {
enum class ControllerLayout {Touch,Index,Frame,Generic};
struct ControllerProfile {char const* path;char const* extension;ControllerLayout layout;};
// Native declarations are compatibility candidates, not hardware certification.
// No system buttons or fabricated input paths are suggested.
inline constexpr ControllerProfile ControllerProfiles[]{
    {"/interaction_profiles/oculus/touch_controller","",ControllerLayout::Touch},
    {"/interaction_profiles/valve/index_controller","",ControllerLayout::Index},
    {"/interaction_profiles/bytedance/pico4_controller","XR_BD_controller_interaction",ControllerLayout::Touch},
    {"/interaction_profiles/bytedance/pico_neo3_controller","XR_BD_controller_interaction",ControllerLayout::Touch},
    {"/interaction_profiles/valve/frame_controller_valve","XR_VALVE_frame_controller_interaction",ControllerLayout::Frame},
    {"/interaction_profiles/khr/generic_controller","XR_KHR_generic_controller",ControllerLayout::Generic}
};
inline bool ControllerProfileEnabled(ControllerProfile const& profile,std::vector<std::string> const& enabled) {
    return !*profile.extension||std::find(enabled.begin(),enabled.end(),profile.extension)!=enabled.end();
}
inline std::vector<std::string> SupportedControllerExtensions(std::vector<std::string> const& available) {
    std::vector<std::string> out;
    for(auto const& p:ControllerProfiles)if(*p.extension&&ControllerProfileEnabled(p,available)&&std::find(out.begin(),out.end(),p.extension)==out.end())out.emplace_back(p.extension);
    return out;
}
}
