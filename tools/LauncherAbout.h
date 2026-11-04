// Gothic 3 VR — project created and maintained by PeCOS2k5.
// SPDX-License-Identifier: GPL-3.0-or-later
// Original and third-party credits: AUTHORS.md and THIRD-PARTY-NOTICES.md.
#pragma once
#include "ProjectIdentity.h"

HRESULT CALLBACK AboutCallback(HWND,UINT message,WPARAM,LPARAM value,LONG_PTR){
    if(message==TDN_HYPERLINK_CLICKED){
        auto* url=reinterpret_cast<wchar_t const*>(value);
        for(auto* allowed:{g3vr::identity::Repository,g3vr::identity::Releases,g3vr::identity::Nexus,
                          g3vr::identity::License,g3vr::identity::Credits})
            if(url&&!wcscmp(url,allowed)){ShellExecuteW(window,L"open",allowed,nullptr,nullptr,SW_SHOWNORMAL);break;}
    }
    return S_OK;
}
void OpenAbout(){
    TASKDIALOGCONFIG dialog{sizeof(dialog)};
    dialog.hwndParent=window;dialog.hInstance=instance;
    dialog.dwFlags=TDF_ENABLE_HYPERLINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_SIZE_TO_CONTENT;
    dialog.dwCommonButtons=TDCBF_CLOSE_BUTTON;
    dialog.pszWindowTitle=L"About Gothic 3 VR";
    dialog.pszMainInstruction=L"Gothic 3 VR by PeCOS2k5";
    dialog.pszContent=L"Created and maintained by PeCOS2k5.\n\n"
        L"<a href=\"https://github.com/PeCOS2k5/Gothic3VR/releases\">Official downloads</a>   "
        L"<a href=\"https://www.nexusmods.com/gothic3/mods/115\">Nexus Mods</a>\n"
        L"<a href=\"https://github.com/PeCOS2k5/Gothic3VR\">Official repository</a>   "
        L"<a href=\"https://github.com/PeCOS2k5/Gothic3VR/blob/main/AUTHORS.md\">Authors and credits</a>\n\n"
        L"Project code: <a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU GPL v3 or later</a>. "
        L"You may modify and redistribute the code under that license. No warranty.\n\n"
        L"Gothic 3 and its original content belong to their respective rights holders. "
        L"This is an unofficial VR modification.";
    dialog.pszExpandedInformation=L"Gothic 3 SDK: Georgeto and contributors.\n"
        L"OpenXR/Vulkan: Khronos and contributors. OpenVR: Valve and contributors.\n"
        L"DXVK: Philip Rebohle and contributors. SMAA: iryoku contributors.\n"
        L"NIS/FXAA: NVIDIA. CAS: AMD and contributors.\n\n"
        L"Third-party components, native patches, optional mods, artwork and music "
        L"retain their applicable licenses and credits. See the accompanying notices.\n\n"
        L"Independently maintained versions must not be presented as official PeCOS2k5 releases.";
    dialog.pszExpandedControlText=L"Third-party credits";
    dialog.pszCollapsedControlText=L"Hide third-party credits";
    dialog.pfCallback=AboutCallback;
    if(FAILED(TaskDialogIndirect(&dialog,nullptr,nullptr,nullptr)))
        MessageBoxW(window,L"Gothic 3 VR by PeCOS2k5\n\nProject code: GPL-3.0-or-later. No warranty.\n"
            L"Official project: https://github.com/PeCOS2k5/Gothic3VR\n"
            L"Third-party credits are in AUTHORS.md and THIRD-PARTY-NOTICES.md.",L"About Gothic 3 VR",MB_OK|MB_ICONINFORMATION);
}
