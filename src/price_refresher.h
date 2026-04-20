#ifndef PRICE_REFRESHER_H
#define PRICE_REFRESHER_H

#include <cstdint>
#include <string>

// Spawns refresh_prices.py as a child process and polls its exit each frame.
// No threads — uses non-blocking OS process APIs.
class PriceRefresher {
public:
    enum class State { Idle, Running, Succeeded, Failed };

    PriceRefresher();
    ~PriceRefresher();

    PriceRefresher(const PriceRefresher&) = delete;
    PriceRefresher& operator=(const PriceRefresher&) = delete;

    // Start a refresh for the given CSV file. No-op if already running or path empty.
    // Returns true if the process was successfully launched.
    bool start(const std::string& csv_path);

    // Non-blocking poll — call once per frame. Transitions state when the child exits.
    void poll();

    State              state()        const { return state_; }
    bool               is_busy()      const { return state_ == State::Running; }
    const std::string& last_message() const { return last_message_; }

private:
    void cleanup_process();

    State       state_            = State::Idle;
    std::string last_message_;
    std::string status_file_path_;

#ifdef _WIN32
    // Stores a HANDLE as intptr_t so windows.h doesn't bleed into headers.
    // -1 == INVALID_HANDLE_VALUE on both 32/64-bit Windows.
    intptr_t process_handle_ = -1;
#else
    int child_pid_ = -1;
#endif
};

#endif
