#include "tui/screen_layout.hpp"

#include <algorithm>

namespace ss::tui {

ScreenLayout compute_layout(int width, int height) {
    return {.visible_rows = std::max(1, height - service_rows),
            .visible_cols = std::max(1, (width - row_header_width) / column_width)};
}

}  // namespace ss::tui
