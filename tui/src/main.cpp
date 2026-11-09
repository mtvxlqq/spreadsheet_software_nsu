#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

int main() {
    auto app = ftxui::App::Fullscreen();
    // управление только с клавиатуры
    app.TrackMouse(false);

    auto greeting = ftxui::Renderer([] {
        return ftxui::text("Табличный процессор. Для выхода нажмите букву") | ftxui::center;
    });
    auto root = ftxui::CatchEvent(greeting, [&app](const ftxui::Event& event) {
        // свои ответы на запросы терминал присылает тоже как события
        if (!event.is_character()) {
            return false;
        }
        app.Exit();
        return true;
    });

    app.Loop(root);
    return 0;
}
