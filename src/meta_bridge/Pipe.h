#pragma once
#include "Protocol.h"
#include <utility>

namespace diagnostic::meta_bridge {
class Pipe {
    HANDLE handle = INVALID_HANDLE_VALUE;
    HANDLE completion = nullptr;
    HANDLE peer = nullptr; // Borrowed child-process handle; owned by Client.
    uint32_t serial = 0;
    ULONGLONG deadline = 0;

    DWORD Remaining() const {
        if (!deadline) return INFINITE;
        auto now = GetTickCount64();
        return now >= deadline ? 0 : static_cast<DWORD>(deadline - now);
    }

    void Transfer(void* data, uint32_t size, bool write) {
        auto p = static_cast<uint8_t*>(data);
        while (size) {
            if (deadline && !Remaining()) throw std::runtime_error("Meta bridge request timed out");
            DWORD count = 0;
            BOOL ok;
            if (completion) {
                OVERLAPPED operation{};
                operation.hEvent = completion;
                ResetEvent(completion);
                ok = write ? WriteFile(handle, p, size, &count, &operation)
                           : ReadFile(handle, p, size, &count, &operation);
                if (!ok && GetLastError() == ERROR_IO_PENDING) {
                    HANDLE waits[] = {completion, peer};
                    auto result = WaitForMultipleObjects(peer ? 2 : 1, waits, FALSE, Remaining());
                    auto waitError = result == WAIT_FAILED ? GetLastError() : ERROR_SUCCESS;
                    if (result != WAIT_OBJECT_0) {
                        // The OVERLAPPED and buffer must outlive cancellation.
                        CancelIoEx(handle, &operation);
                        GetOverlappedResult(handle, &operation, &count, TRUE);
                        if (result == WAIT_TIMEOUT) throw std::runtime_error("Meta bridge request timed out");
                        if (result == WAIT_FAILED)
                            throw std::runtime_error("Meta bridge I/O wait failed: " + std::to_string(waitError));
                        throw std::runtime_error("Meta bridge host exited");
                    }
                    ok = GetOverlappedResult(handle, &operation, &count, FALSE);
                }
            } else {
                // The host keeps its existing synchronous server endpoint.
                ok = write ? WriteFile(handle, p, size, &count, nullptr)
                           : ReadFile(handle, p, size, &count, nullptr);
            }
            if (!ok || !count) throw std::runtime_error("Meta bridge process disconnected");
            p += count;
            size -= count;
        }
    }

public:
    Pipe() = default;
    ~Pipe() { Reset(); }
    Pipe(Pipe const&) = delete;
    Pipe& operator=(Pipe const&) = delete;
    uint32_t Calls() const { return serial; }

    void Reset() {
        if (handle != INVALID_HANDLE_VALUE) { CloseHandle(handle); handle = INVALID_HANDLE_VALUE; }
        if (completion) { CloseHandle(completion); completion = nullptr; }
        peer = nullptr;
        deadline = 0;
    }

    void Connect(std::wstring const& name, HANDLE child) {
        Reset();
        auto until = GetTickCount64() + 15000;
        for (;;) {
            handle = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                 OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
            if (handle != INVALID_HANDLE_VALUE) {
                completion = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                if (!completion) { Reset(); throw std::runtime_error("Cannot create Meta bridge I/O event"); }
                peer = child;
                return;
            }
            if (WaitForSingleObject(child, 0) == WAIT_OBJECT_0)
                throw std::runtime_error("Meta 64-bit host exited during startup");
            if (GetTickCount64() >= until) throw std::runtime_error("Meta 64-bit host startup timed out");
            Sleep(5);
        }
    }

    void Serve(std::wstring const& name) {
        Reset();
        handle = CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
            1, 65536, 65536, 0, nullptr);
        if (handle == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create local Meta bridge pipe");
        if (!ConnectNamedPipe(handle, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED)
            throw std::runtime_error("Cannot connect local Meta bridge pipe");
    }

    void Send(PacketHeader header, Writer const& payload) {
        if (payload.bytes.size() > MaximumPacket) throw std::runtime_error("Meta bridge packet too large");
        header.bytes = uint32_t(payload.bytes.size());
        Transfer(&header, sizeof(header), true);
        if (header.bytes) Transfer(const_cast<uint8_t*>(payload.bytes.data()), header.bytes, true);
    }

    std::pair<PacketHeader, Reader> Receive() {
        PacketHeader header;
        Transfer(&header, sizeof(header), false);
        if (header.version != Version || header.bytes > MaximumPacket)
            throw std::runtime_error("Incompatible Meta bridge protocol");
        std::vector<uint8_t> bytes(header.bytes);
        if (header.bytes) Transfer(bytes.data(), header.bytes, false);
        return {header, Reader(std::move(bytes))};
    }

    Reader Call(Command command, Writer const& payload = {}, DWORD timeoutMs = 0) {
        // Lifecycle operations are bounded. In particular, xrWaitFrame keeps
        // its runtime-defined wait; it must not inherit a startup deadline.
        if (!timeoutMs) timeoutMs = command == Command::Hello ? 15000 :
                                    command == Command::DestroyInstance ? 2000 : INFINITE;
        deadline = timeoutMs == INFINITE ? 0 : GetTickCount64() + timeoutMs;
        try {
            PacketHeader h;
            h.command = uint32_t(command);
            h.serial = ++serial;
            Send(h, payload);
            auto response = Receive();
            if (response.first.command != h.command || response.first.serial != h.serial)
                throw std::runtime_error("Meta bridge response mismatch");
            deadline = 0;
            return std::move(response.second);
        } catch (...) {
            // A timed-out/partial transaction cannot be reused as a new reply.
            Reset();
            throw;
        }
    }
};
}
