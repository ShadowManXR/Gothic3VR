#include "meta_bridge/Pipe.h"
#include <functional>
#include <iostream>
#include <thread>

using namespace diagnostic::meta_bridge;
namespace {
void Check(bool value, char const* message) {
    if (!value) throw std::runtime_error(message);
}
struct OwnProcess {
    HANDLE handle = OpenProcess(SYNCHRONIZE, FALSE, GetCurrentProcessId());
    OwnProcess() { Check(handle != nullptr, "Cannot open test process handle"); }
    ~OwnProcess() { CloseHandle(handle); }
};

struct Server {
    HANDLE release = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::wstring name = L"\\\\.\\pipe\\G3VR_pipe_test_" + std::to_wstring(GetCurrentProcessId()) +
                        L"_" + std::to_wstring(sequence++);
    std::exception_ptr error;
    std::thread worker;
    inline static unsigned sequence = 0;

    explicit Server(std::function<void(Pipe&, HANDLE)> action) {
        if (!release) throw std::runtime_error("Cannot create test event");
        worker = std::thread([this, action] {
            try { Pipe pipe; pipe.Serve(name); action(pipe, release); }
            catch (...) { error = std::current_exception(); }
        });
    }
    void Finish() {
        SetEvent(release);
        worker.join();
        if (error) std::rethrow_exception(error);
    }
    ~Server() {
        SetEvent(release);
        if (worker.joinable()) { CancelSynchronousIo(worker.native_handle()); worker.join(); }
        CloseHandle(release);
    }
};

void Echo(Pipe& pipe) {
    auto request = pipe.Receive();
    Writer result;
    result.bytes = std::move(request.second.bytes);
    pipe.Send(request.first, result);
}

void Stall(Command command, DWORD overrideTimeout, ULONGLONG minimum, ULONGLONG maximum) {
    Server server([](Pipe& pipe, HANDLE release) { pipe.Receive(); WaitForSingleObject(release, INFINITE); });
    Pipe client;
    OwnProcess process;
    client.Connect(server.name, process.handle);
    auto start = GetTickCount64();
    bool timedOut = false;
    try { client.Call(command, {}, overrideTimeout); }
    catch (std::exception const& e) {
        timedOut = std::string(e.what()).find("timed out") != std::string::npos;
        std::cout << "Lifecycle " << uint32_t(command) << ": " << e.what() << '\n';
    }
    auto elapsed = GetTickCount64() - start;
    std::cout << "Elapsed: " << elapsed << " ms\n";
    Check(timedOut && elapsed >= minimum && elapsed < maximum, "Lifecycle request was not bounded");
    server.Finish();
    bool closed = false;
    try { client.Call(Command::InstanceProperties); }
    catch (...) { closed = true; }
    Check(closed, "Timed-out connection remained reusable");
}
}

int main() {
    try {
        // Exercise the real defaults, not only the test timeout override.
        Stall(Command::Hello, 0, 14000, 20000);
        Stall(Command::DestroyInstance, 0, 1800, 6000);
        Stall(Command::Hello, 100, 70, 3000);

        Server server([](Pipe& pipe, HANDLE) {
            Echo(pipe); // Hello.
            auto request = pipe.Receive();
            Sleep(2300); // A normal frame wait must not inherit the teardown timeout.
            Writer result;
            pipe.Send(request.first, result);
            Echo(pipe); // Larger than the pipe buffer; exercises partial transfer.
        });
        Pipe client;
        OwnProcess process;
        client.Connect(server.name, process.handle);
        client.Call(Command::Hello).Complete();
        auto start = GetTickCount64();
        client.Call(Command::WaitFrame).Complete();
        Check(GetTickCount64() - start >= 2200, "Frame wait did not complete normally");
        Writer payload;
        payload.bytes.assign(256 * 1024, 0x5a);
        auto response = client.Call(Command::InstanceProperties, payload);
        Check(response.bytes == payload.bytes, "Partial-transfer payload corrupted");
        server.Finish();

        HANDLE peerExit = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        Check(peerExit != nullptr, "Cannot create peer-exit event");
        {
            Server exiting([peerExit](Pipe& pipe, HANDLE release) {
                pipe.Receive(); SetEvent(peerExit); WaitForSingleObject(release, INFINITE);
            });
            Pipe peerClient;
            peerClient.Connect(exiting.name, peerExit);
            bool failed = false;
            start = GetTickCount64();
            try { peerClient.Call(Command::WaitFrame); }
            catch (...) { failed = true; }
            Check(failed && GetTickCount64() - start < 3000, "Peer exit did not cancel pending I/O");
            exiting.Finish();
        }
        CloseHandle(peerExit);
        std::cout << "PASS Meta lifecycle deadlines, cancellation, frame wait and partial transfers\n";
        return 0;
    } catch (std::exception const& e) { std::cerr << e.what() << '\n'; return 1; }
}
