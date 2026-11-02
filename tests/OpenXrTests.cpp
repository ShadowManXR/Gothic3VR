#include "OpenXrClient.h"
#include <cstdio>
int __cdecl main()
{
    diagnostic::OpenXrClient client;
    diagnostic::HeadPose pose;
    diagnostic::HeadsetInfo info;
    bool connect = !client.Connect();
    bool read = !client.Read(pose) && !pose.runtimeConnected && !pose.valid;
    bool metadata = !client.Info(info);
    std::printf("%s unloaded OpenXR cannot connect\n",connect ? "PASS":"FAIL");
    std::printf("%s absent OpenXR cannot invent pose\n",read ? "PASS":"FAIL");
    std::printf("%s absent OpenXR cannot invent headset\n",metadata ? "PASS":"FAIL");
    return connect && read && metadata ? 0:1;
}
