#include <gtest/gtest.h>

// Заглушка: проверяет, что сборка, GoogleTest и ctest подключены.
// Удаляется, когда появятся настоящие тесты ядра.
TEST(Smoke, BuildAndTestInfrastructureWorks) {
    EXPECT_EQ(2 + 2, 4);
}
