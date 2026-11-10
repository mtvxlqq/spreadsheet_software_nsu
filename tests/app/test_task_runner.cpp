#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <latch>
#include <memory>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>

#include <gtest/gtest.h>

#include "app/action_registry.hpp"
#include "app/keymap.hpp"
#include "app/register_actions.hpp"
#include "app/session.hpp"
#include "app/session_observer.hpp"
#include "app/task_runner.hpp"
#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/errors.hpp"
#include "core/function_registry.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/number_format_registry.hpp"
#include "core/value.hpp"
#include "core/value_parser.hpp"
#include "support/manual_dispatcher.hpp"

namespace fs = std::filesystem;

using ss::app::Session;
using ss::app::Status;
using ss::app::TaskRunner;
using ss::core::CellAddress;
using ss::test::ManualDispatcher;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

// крутится, пока не попросят остановиться
void wait_for_stop(const std::stop_token& stop) {
    while (!stop.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

class CountingObserver : public ss::app::SessionObserver {
public:
    void on_session_changed() override {
        ++calls;
    }

    int calls = 0;
};

}  // namespace

TEST(TaskRunnerTest, ContinuationRunsOnlyOnDrainInMainThread) {
    ManualDispatcher dispatcher;
    TaskRunner runner(dispatcher);
    const std::thread::id main_thread = std::this_thread::get_id();
    std::thread::id work_thread;
    std::thread::id continuation_thread;

    runner.submit([&](std::stop_token) -> TaskRunner::Continuation {
        work_thread = std::this_thread::get_id();
        return [&] { continuation_thread = std::this_thread::get_id(); };
    });
    ASSERT_TRUE(dispatcher.wait_for_task());
    EXPECT_EQ(continuation_thread, std::thread::id{});  // до drain ничего

    EXPECT_EQ(dispatcher.drain(), 1U);
    EXPECT_NE(work_thread, main_thread);
    EXPECT_EQ(continuation_thread, main_thread);
}

TEST(TaskRunnerTest, ExceptionFromWorkComesToMainThread) {
    ManualDispatcher dispatcher;
    TaskRunner runner(dispatcher);
    runner.submit([](std::stop_token) -> TaskRunner::Continuation {
        throw ss::core::Error("сломалось в фоне");
    });
    ASSERT_TRUE(dispatcher.wait_for_task());
    EXPECT_THROW(dispatcher.drain(), ss::core::Error);
}

TEST(TaskRunnerTest, EmptyContinuationIsNotPosted) {
    ManualDispatcher dispatcher;
    {
        TaskRunner runner(dispatcher);
        runner.submit([](std::stop_token) { return TaskRunner::Continuation{}; });
    }
    EXPECT_EQ(dispatcher.pending(), 0U);
}

TEST(TaskRunnerTest, DestructorStopsAndWaitsForThreads) {
    ManualDispatcher dispatcher;
    std::atomic<bool> finished = false;
    std::latch started(1);
    {
        TaskRunner runner(dispatcher);
        runner.submit([&](std::stop_token stop) -> TaskRunner::Continuation {
            started.count_down();
            wait_for_stop(stop);
            finished = true;
            return [] {};
        });
        started.wait();
    }
    EXPECT_TRUE(finished);
    // после остановки продолжение не отправляется
    EXPECT_EQ(dispatcher.pending(), 0U);
}

TEST(TaskRunnerTest, ManyTasks) {
    ManualDispatcher dispatcher;
    TaskRunner runner(dispatcher);
    int done = 0;
    for (int i = 0; i < 20; ++i) {
        runner.submit([&done](std::stop_token) -> TaskRunner::Continuation {
            return [&done] { ++done; };
        });
    }
    while (done < 20) {
        ASSERT_TRUE(dispatcher.wait_for_task());
        dispatcher.drain();
    }
    EXPECT_EQ(done, 20);
}

namespace {

class BackgroundLoadTest : public ::testing::Test {
protected:
    BackgroundLoadTest() {
        ss::core::register_builtins(functions);
        ss::core::register_value_parsers(value_parsers);
        ss::core::register_formats(formats);
        ss::core::register_number_formats(number_formats);
        ss::app::register_actions(actions);
        const std::string test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        dir = fs::temp_directory_path() / ("ss_background_" + test_name);
        fs::remove_all(dir);
        fs::create_directories(dir);
        session = std::make_unique<Session>(functions, value_parsers, formats, number_formats, actions,
                                            ss::app::Keymap::defaults(), dispatcher);
    }

    ~BackgroundLoadTest() override {
        session.reset();
        std::error_code ignored;
        fs::remove_all(dir, ignored);
    }

    void press(std::string_view text) {
        for (const char c : text) {
            session->handle_key(std::string(1, c));
        }
    }

    void command(std::string_view line) {
        session->handle_key(":");
        press(line);
        session->handle_key("enter");
    }

    void input_a1(std::string_view raw) {
        session->set_cursor(at("A1"));
        session->handle_key("enter");
        session->edit_line().clear();
        press(raw);
        session->handle_key("enter");
    }

    std::string write_table(std::string_view name, std::string_view text) const {
        const fs::path path = dir / name;
        std::ofstream(path, std::ios::binary) << text;
        return path.string();
    }

    std::string raw(std::string_view address) const {
        return session->document().raw(at(address));
    }

    ss::core::FunctionRegistry functions;
    ss::core::ValueParserRegistry value_parsers;
    ss::core::FormatRegistry formats;
    ss::core::NumberFormatRegistry number_formats;
    ss::app::ActionRegistry actions;
    ManualDispatcher dispatcher;
    std::unique_ptr<Session> session;
    fs::path dir;
};

}  // namespace

TEST_F(BackgroundLoadTest, DocumentChangesOnlyAfterDrain) {
    const std::string path = write_table("t.sst", "SST 1\nA1\tnew\n");
    input_a1("old");
    command("e " + path);
    EXPECT_TRUE(session->busy());
    EXPECT_EQ(raw("A1"), "old");

    ASSERT_TRUE(dispatcher.wait_for_task());
    // загрузка уже закончилась, но главный поток ещё ничего не получил
    EXPECT_EQ(raw("A1"), "old");

    dispatcher.drain();
    EXPECT_EQ(raw("A1"), "new");
    EXPECT_FALSE(session->busy());
    EXPECT_EQ(session->status().kind, Status::Kind::Info);
    EXPECT_EQ(session->current_path(), fs::path(path));
}

TEST_F(BackgroundLoadTest, BrokenFileKeepsOldDocument) {
    const std::string path = write_table("bad.sst", "SST 1\nA1\t=1+\n");
    input_a1("old");
    command("e " + path);
    ASSERT_TRUE(dispatcher.wait_for_task());
    dispatcher.drain();
    EXPECT_EQ(raw("A1"), "old");
    EXPECT_EQ(session->status().kind, Status::Kind::Error);
    EXPECT_FALSE(session->busy());
    EXPECT_TRUE(session->dirty());
}

TEST_F(BackgroundLoadTest, UnknownFormatFailsWithoutThread) {
    command("e table.xlsx");
    EXPECT_FALSE(session->busy());
    EXPECT_EQ(session->status().kind, Status::Kind::Error);
    EXPECT_EQ(dispatcher.pending(), 0U);
}

TEST_F(BackgroundLoadTest, KeysWorkWhileLoading) {
    std::latch release(1);
    session->run_in_background([&release](std::stop_token) -> std::function<void(Session&)> {
        release.wait();
        return [](Session& target) { target.set_status("готово"); };
    });
    // пока фон ждёт, интерфейс работает
    input_a1("7");
    EXPECT_EQ(raw("A1"), "7");
    EXPECT_TRUE(session->busy());

    release.count_down();
    ASSERT_TRUE(dispatcher.wait_for_task());
    dispatcher.drain();
    EXPECT_EQ(session->status().text, "готово");
    EXPECT_FALSE(session->busy());
}

TEST_F(BackgroundLoadTest, ObserversHearAboutFinishedTask) {
    CountingObserver observer;
    session->subscribe(&observer);
    session->run_in_background(
        [](std::stop_token) -> std::function<void(Session&)> { return [](Session&) {}; });
    ASSERT_TRUE(dispatcher.wait_for_task());
    dispatcher.drain();
    EXPECT_EQ(observer.calls, 1);
    session->unsubscribe(&observer);
}

TEST_F(BackgroundLoadTest, ForeignExceptionBecomesStatus) {
    session->run_in_background([](std::stop_token) -> std::function<void(Session&)> {
        throw std::runtime_error("не наша ошибка");
    });
    ASSERT_TRUE(dispatcher.wait_for_task());
    dispatcher.drain();
    EXPECT_EQ(session->status().kind, Status::Kind::Error);
    EXPECT_EQ(session->status().text, "не наша ошибка");
}

TEST_F(BackgroundLoadTest, DestroyingSessionWithRunningTask) {
    std::latch started(1);
    std::atomic<bool> stopped = false;
    session->run_in_background([&](std::stop_token stop) -> std::function<void(Session&)> {
        started.count_down();
        wait_for_stop(stop);
        stopped = true;
        return [](Session& target) { target.set_status("не должно выполниться"); };
    });
    started.wait();
    // деструктор попросит поток остановиться и дождётся его
    session.reset();
    EXPECT_TRUE(stopped);
    EXPECT_EQ(dispatcher.drain(), 0U);
}

TEST_F(BackgroundLoadTest, ContinuationAfterSessionIsGoneDoesNothing) {
    bool ran = false;
    session->run_in_background([&ran](std::stop_token) -> std::function<void(Session&)> {
        return [&ran](Session& target) {
            ran = true;
            target.set_status("сеанс уже удалён");
        };
    });
    // продолжение уже в очереди, а сеанса не станет раньше, чем его выполнят
    ASSERT_TRUE(dispatcher.wait_for_task());
    session.reset();
    // под ASan видно, что к удалённому сеансу никто не обратился
    EXPECT_EQ(dispatcher.drain(), 1U);
    EXPECT_FALSE(ran);
}
