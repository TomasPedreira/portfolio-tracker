#include "price_refresher.h"

#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

// ── Helpers ───────────────────────────────────────────────────────────────────

#ifdef _WIN32
static std::string get_exe_dir() {
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string dir(buf);
    const auto pos = dir.find_last_of("\\/");
    if (pos != std::string::npos) dir.resize(pos);
    return dir;
}
#endif

// Minimal parse of the JSON status file written by the script.
// Expected: {"updated": N, "failed": [...], "ts": "HH:MM"} or {"error": "..."}
static std::string parse_status(const std::string& json, bool success) {
    if (!success) {
        const auto epos = json.find("\"error\":\"");
        if (epos != std::string::npos) {
            const auto start = epos + 9;
            const auto end   = json.find('"', start);
            if (end != std::string::npos)
                return json.substr(start, end - start);
        }
        return "Refresh failed";
    }

    int updated = 0;
    {
        const auto pos = json.find("\"updated\":");
        if (pos != std::string::npos) {
            auto vpos = pos + 10;
            while (vpos < json.size() && json[vpos] == ' ') ++vpos;
            try { updated = std::stoi(json.substr(vpos)); } catch (...) {}
        }
    }

    std::string ts;
    {
        const auto pos = json.find("\"ts\":\"");
        if (pos != std::string::npos) {
            const auto start = pos + 6;
            const auto end   = json.find('"', start);
            if (end != std::string::npos) ts = json.substr(start, end - start);
        }
    }

    std::string msg = "Updated " + std::to_string(updated)
                    + " symbol" + (updated == 1 ? "" : "s");
    if (!ts.empty()) msg += " at " + ts;
    return msg;
}

// ── PriceRefresher ────────────────────────────────────────────────────────────

PriceRefresher::PriceRefresher() = default;

PriceRefresher::~PriceRefresher() {
    cleanup_process();
}

bool PriceRefresher::start(const std::string& csv_path) {
    if (state_ == State::Running || csv_path.empty()) return false;

#ifdef _WIN32
    const std::string script = get_exe_dir() + "\\scripts\\refresh_prices.py";
    const std::string cmd    = "python \"" + script + "\" \"" + csv_path + "\"";

    std::vector<char> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back('\0');

    STARTUPINFOA si{};
    si.cb          = sizeof(si);
    si.dwFlags     = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    if (!CreateProcessA(nullptr, cmd_buf.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        state_        = State::Failed;
        last_message_ = "Failed to launch Python";
        return false;
    }
    process_handle_ = reinterpret_cast<intptr_t>(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    const std::string script = "scripts/refresh_prices.py";
    const pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/env", "env", "python3", script.c_str(), csv_path.c_str(), nullptr);
        _exit(1);
    } else if (pid < 0) {
        state_        = State::Failed;
        last_message_ = "Failed to launch Python";
        return false;
    }
    child_pid_ = pid;
#endif

    status_file_path_ = csv_path + ".refresh_status";
    last_message_     = "Refreshing...";
    state_            = State::Running;
    return true;
}

void PriceRefresher::poll() {
    if (state_ != State::Running) return;

    bool exited  = false;
    bool success = false;

#ifdef _WIN32
    if (process_handle_ == -1) return;
    HANDLE h = reinterpret_cast<HANDLE>(process_handle_);
    DWORD code = 0;
    if (!GetExitCodeProcess(h, &code)) {
        cleanup_process();
        state_        = State::Failed;
        last_message_ = "Process error";
        return;
    }
    if (code == STILL_ACTIVE) return;
    exited  = true;
    success = (code == 0);
    cleanup_process();
#else
    int wstatus = 0;
    const int ret = waitpid(child_pid_, &wstatus, WNOHANG);
    if (ret == 0) return;
    child_pid_ = -1;
    exited  = (ret > 0);
    success = exited && WIFEXITED(wstatus) && (WEXITSTATUS(wstatus) == 0);
#endif

    if (!exited) {
        state_        = State::Failed;
        last_message_ = "Refresh failed";
        return;
    }

    std::string json_line;
    {
        std::ifstream f(status_file_path_);
        if (f) std::getline(f, json_line);
    }

    last_message_ = parse_status(json_line, success);
    state_        = success ? State::Succeeded : State::Failed;
}

void PriceRefresher::cleanup_process() {
#ifdef _WIN32
    if (process_handle_ != -1) {
        CloseHandle(reinterpret_cast<HANDLE>(process_handle_));
        process_handle_ = -1;
    }
#endif
}
