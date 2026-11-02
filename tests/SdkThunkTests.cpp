#include <Engine/ge_application.h>
#include <Engine/api/dx9/ge_dx9device.h>
#include "NativeOcclusion.h"
#include <Game/ge_charactercontrol_ps.h>
#include <Engine/ge_rigidbody_ps.h>
#include <Game/script/ge_scriptadmin.h>
#include <Game/ge_cameraai_ps.h>
#include <Game/ge_session.h>
#include "VirtualHook.h"
#include <cstdio>

class ApplicationAccess : public eCApplication
{
public:
    using eCApplication::OnPreRender;
    using eCApplication::OnRenderDraw;
    using eCApplication::OnPostRender;
    using eCApplication::OnDestroyEngine;
    using eCApplication::OnDestroyWorkspace;
};
class CharacterControlAccess : public gCCharacterControl_PS
{
public: using gCCharacterControl_PS::ProcessMovements;
};
class CharacterMovementAccess : public gCCharacterMovement_PS
{
public: using gCCharacterMovement_PS::OnProcess;using gCCharacterMovement_PS::OnUpdatedWorldMatrix;
};

int __cdecl main()
{
    // Compile the actual SDK member pointers with the same Release /O2 /GL
    // and calling-convention options as the plugin. No game object is created.
    size_t slots[4]{};
    bool decoded[] = {
        diagnostic::VirtualSlot(&eCApplication::OnProcess, slots[0]),
        diagnostic::VirtualSlot(&ApplicationAccess::OnPreRender, slots[1]),
        diagnostic::VirtualSlot(&ApplicationAccess::OnRenderDraw, slots[2]),
        diagnostic::VirtualSlot(&ApplicationAccess::OnPostRender, slots[3])};
    char const* names[] = {"OnProcess", "OnPreRender", "OnRenderDraw", "OnPostRender"};
    bool success = true;
    for (unsigned i = 0; i < 4; ++i)
    {
        std::printf("%s %s SDK Release virtual thunk: slot=%zu\n",
                    decoded[i] ? "PASS" : "FAIL", names[i], slots[i]);
        success = success && decoded[i];
        for (unsigned j = 0; j < i; ++j) success = success && slots[i] != slots[j];
    }
    size_t movementSlot=0;
    bool movementDecoded=diagnostic::VirtualSlot(&CharacterControlAccess::ProcessMovements,movementSlot);
    std::printf("%s ProcessMovements SDK Release virtual thunk: slot=%zu\n",movementDecoded&&movementSlot==85?"PASS":"FAIL",movementSlot);
    size_t actionSlot=0;
    bool actionDecoded=diagnostic::VirtualSlot(&gCCharacterControl_PS::OnAction,actionSlot);
    std::printf("%s OnAction SDK Release virtual thunk: slot=%zu\n",actionDecoded&&actionSlot==47?"PASS":"FAIL",actionSlot);
    size_t processSlot=0,velocitySlot=0;
    bool processDecoded=diagnostic::VirtualSlot(&CharacterMovementAccess::OnProcess,processSlot);
    bool velocityDecoded=diagnostic::VirtualSlot(&eCRigidBody_PS::SetLinearVelocity,velocitySlot);
    std::printf("%s CharacterMovement OnProcess SDK thunk: slot=%zu\n",processDecoded&&processSlot==73?"PASS":"FAIL",processSlot);
    std::printf("%s RigidBody SetLinearVelocity SDK thunk: slot=%zu\n",velocityDecoded&&velocitySlot==92?"PASS":"FAIL",velocitySlot);
    size_t shutdownSlot=0;
    bool shutdownDecoded=diagnostic::VirtualSlot(&ApplicationAccess::OnDestroyEngine,shutdownSlot)&&shutdownSlot==165;
    std::printf("%s OnDestroyEngine SDK virtual thunk: slot=%zu\n",shutdownDecoded?"PASS":"FAIL",shutdownSlot);
    size_t workspaceSlot=0;bool workspaceDecoded=diagnostic::VirtualSlot(&ApplicationAccess::OnDestroyWorkspace,workspaceSlot)&&workspaceSlot==166;
    std::printf("%s OnDestroyWorkspace SDK virtual thunk: slot=%zu\n",workspaceDecoded?"PASS":"FAIL",workspaceSlot);
    size_t cameraPermissionSlot=0;
    bool cameraPermissionDecoded=diagnostic::VirtualSlot(&gCScriptAdmin::CallScript,cameraPermissionSlot)&&cameraPermissionSlot==47;
    std::printf("%s ScriptAdmin CallScript SDK virtual thunk: slot=%zu\n",cameraPermissionDecoded?"PASS":"FAIL",cameraPermissionSlot);
    size_t targetSlot=0;bool targetDecoded=diagnostic::VirtualSlot(&gCCameraAI_PS::SetTarget,targetSlot)&&targetSlot==85;
    std::printf("%s CameraAI SetTarget native return-heading thunk: slot=%zu\n",targetDecoded?"PASS":"FAIL",targetSlot);
    size_t modeSlot=0;bool modeDecoded=diagnostic::VirtualSlot(&gCCameraAI_PS::SetMode,modeSlot)&&modeSlot==88;
    std::printf("%s CameraAI SetMode SDK virtual thunk: slot=%zu\n",modeDecoded?"PASS":"FAIL",modeSlot);
    size_t worldSlot=0;bool worldDecoded=diagnostic::VirtualSlot(&CharacterMovementAccess::OnUpdatedWorldMatrix,worldSlot)&&worldSlot==68;
    std::printf("%s CharacterMovement OnUpdatedWorldMatrix SDK thunk: slot=%zu\n",worldDecoded?"PASS":"FAIL",worldSlot);
    size_t sessionSlot=0;bool sessionDecoded=diagnostic::VirtualSlot(&gCSession::Process,sessionSlot)&&sessionSlot<diagnostic::MaxVirtualSlots;
    std::printf("%s Session Process SDK Release virtual thunk: slot=%zu\n",sessionDecoded?"PASS":"FAIL",sessionSlot);
    size_t querySlot=0;bool queryDecoded=diagnostic::VirtualSlot(&eCDX9Device::GetOcclusionTestInfo,querySlot)&&querySlot==diagnostic::occlusion::QueryInfoSlot;
    std::printf("%s DX9 GetOcclusionTestInfo SDK Release virtual thunk: slot=%zu\n",queryDecoded?"PASS":"FAIL",querySlot);
    return targetDecoded && queryDecoded && success && sessionDecoded && workspaceDecoded && worldDecoded && modeDecoded && cameraPermissionDecoded && shutdownDecoded && movementDecoded && movementSlot==85 && actionDecoded && actionSlot==47 &&
        processDecoded && processSlot==73 && velocityDecoded && velocitySlot==92 ? 0 : 1;
}
