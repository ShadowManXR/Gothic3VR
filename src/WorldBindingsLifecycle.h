#pragma once
#include <cstdint>
namespace diagnostic {
// Native callbacks can re-enter OnProcess while a world is being stopped.
// Block entity work before native teardown, then allow a later loaded world.
class WorldBindingsLifecycle {
    unsigned stops=0;bool closing=false;
public:
    void BeginStop(){++stops;}
    void EndStop(){if(stops)--stops;}
    void BeginShutdown(){closing=true;}
    bool Blocked()const{return closing||stops!=0;}
    bool EntityUpdates(bool gameRunning)const{return gameRunning&&!Blocked();}
};
constexpr unsigned SessionStopRva=0x1741c0;
constexpr unsigned SessionStopThreadImportRva=0x24c8c4;
constexpr unsigned WorkspaceCallbackRva=0x4160;
}
