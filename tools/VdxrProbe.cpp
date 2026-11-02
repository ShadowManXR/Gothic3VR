#include "OpenXrClient.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int __cdecl wmain(int argc, wchar_t** argv)
{
    bool check = argc == 3 && wcscmp(argv[1], L"--check-loader") == 0;
    bool run = argc == 4 && (wcscmp(argv[1], L"--seconds") == 0 || wcscmp(argv[1], L"--seconds-no-frames") == 0);
    if (!check && !run) { std::fputs("Usage: --check-loader DLL | --seconds N DLL\n", stderr); return 1; }
    diagnostic::OpenXrClient client;
    if (!client.Load(argv[check ? 2 : 3]))
    { std::printf("{\"type\":\"loader_error\",\"win32_error\":%lu}\n", client.LoaderError()); return 1; }
    std::puts("{\"type\":\"loader_ready\",\"architecture\":\"x86\",\"interface\":\"OpenXR 1.0\",\"runtime\":\"VDXR\"}");
    std::fflush(stdout);
    if (check) return 0;
    client.EnableFramePump(wcscmp(argv[1], L"--seconds") == 0);
    int seconds = _wtoi(argv[2]);
    if (seconds < 1 || seconds > 120) return 1;
    uint64_t started = GetTickCount64(), nextRetry = 0;
    bool connected = false;
    int previousError = -1;
    unsigned valid = 0;
    while (GetTickCount64() - started < static_cast<uint64_t>(seconds) * 1000)
    {
        uint64_t elapsed = GetTickCount64() - started;
        if (!connected && elapsed >= nextRetry)
        {
            connected = client.Connect();
            if (connected) std::puts("{\"type\":\"runtime_connected\",\"application\":\"headless\"}");
            else if (client.InitError() != previousError)
            {
                std::printf("{\"type\":\"runtime_waiting\",\"openxr_error\":%d}\n", client.InitError());
                previousError = client.InitError();
            }
            std::printf("{\"type\":\"runtime_capabilities\",\"headless\":%s,\"runtime\":\"%s\"}\n", client.HeadlessSupported() ? "true" : "false", client.RuntimeName());
            nextRetry = elapsed + 5000;
        }
        diagnostic::HeadPose pose;
        if (connected)
        {
            connected = client.Read(pose);
            std::printf("{\"type\":\"pose\",\"elapsed_ms\":%llu,\"connected\":%s,\"valid\":%s,\"tracking_result\":%d",
                        elapsed, pose.deviceConnected ? "true" : "false", pose.valid ? "true" : "false", pose.trackingResult);
            if (pose.valid)
            {
                ++valid;
                std::printf(",\"position_m\":[%.6g,%.6g,%.6g],\"quaternion_wxyz\":[%.6g,%.6g,%.6g,%.6g]",
                    pose.position[0],pose.position[1],pose.position[2],pose.quaternion[0],pose.quaternion[1],pose.quaternion[2],pose.quaternion[3]);
            }
            std::puts("}");
        }
        std::fflush(stdout);
        Sleep(100);
    }
    std::printf("{\"type\":\"summary\",\"valid_pose_samples\":%u}\n", valid);
    return valid ? 0 : 2;
}
