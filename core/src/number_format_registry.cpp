#include "core/number_format_registry.hpp"

#include <charconv>
#include <cstddef>
#include <optional>
#include <system_error>
#include <utility>

#include "core/errors.hpp"

namespace ss::core {

namespace {

// число знаков из "fixed:2"; nullopt, если после двоеточия не число от 0 до 15
std::optional<int> read_digits(std::string_view text) {
    int digits = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), digits);
    if (error != std::errc{} || end != text.data() + text.size() || digits < 0 ||
        digits > NumberFormatRegistry::max_digits) {
        return std::nullopt;
    }
    return digits;
}

}  // namespace

void NumberFormatRegistry::add(std::string name, Factory factory) {
    if (factories_.contains(name)) {
        throw Error("формат чисел " + name + " уже зарегистрирован");
    }
    factories_.emplace(std::move(name), std::move(factory));
}

std::shared_ptr<const NumberFormat> NumberFormatRegistry::create(std::string_view spec) const {
    const std::size_t colon = spec.find(':');
    if (colon == std::string_view::npos) {
        return general_;
    }
    const auto it = factories_.find(spec.substr(0, colon));
    const std::optional<int> digits = read_digits(spec.substr(colon + 1));
    if (it == factories_.end() || !digits) {
        return general_;
    }
    return it->second(*digits);
}

std::string NumberFormatRegistry::make_spec(std::string_view name, int digits) const {
    if (!factories_.contains(name)) {
        throw Error("нет такого формата чисел: " + std::string(name));
    }
    if (digits < 0 || digits > max_digits) {
        throw Error("знаков после запятой может быть от 0 до " + std::to_string(max_digits));
    }
    return std::string(name) + ":" + std::to_string(digits);
}

std::vector<std::string> NumberFormatRegistry::names() const {
    std::vector<std::string> result;
    result.reserve(factories_.size());
    for (const auto& [name, factory] : factories_) {
        result.push_back(name);
    }
    return result;
}

void register_number_formats(NumberFormatRegistry& registry) {
    registry.add("fixed", [](int digits) { return std::make_shared<const FixedFormat>(digits); });
    registry.add("percent",
                 [](int digits) { return std::make_shared<const PercentFormat>(digits); });
}

}  // namespace ss::core
