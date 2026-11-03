#pragma once

namespace ss::app {

// Обычно это TUI: по сигналу он перерисовывает экран.
class SessionObserver {
public:
    virtual ~SessionObserver() = default;
    SessionObserver(const SessionObserver&) = delete;
    SessionObserver& operator=(const SessionObserver&) = delete;
    SessionObserver(SessionObserver&&) = delete;
    SessionObserver& operator=(SessionObserver&&) = delete;

    virtual void on_session_changed() = 0;

protected:
    SessionObserver() = default;
};

}  // namespace ss::app
