#include "ExceptionUtils.h"

#include <cstddef>
#include <exception>
#include <functional>

namespace ObjectiveNinja {

void ExceptionUtils::forNested(std::exception_ptr eptr,
    std::function<void(const std::exception&, std::size_t level)> action)
{
    for (auto level = std::size_t {0}; eptr; ++level) {
        try {
            std::rethrow_exception(eptr);
        } catch (const std::exception& e) {
            action(e, level);
            try {
                std::rethrow_exception(eptr);
            } catch (const std::nested_exception& e_nested) {
                eptr = e_nested.nested_ptr();
                continue;
            } catch (...) {}
        } catch (...) {}
        break;
    }
}

void ExceptionUtils::forCurrentNested(
    std::function<void(const std::exception&, std::size_t level)> action)
{
    return ExceptionUtils::forNested(std::current_exception(), std::move(action));
}


std::function<void(const std::exception&, std::size_t level)> ExceptionUtils::logDebugAction(
    BinaryNinja::Logger& log, std::size_t levelOffset)
{
    return [&](const std::exception& e, std::size_t level) {
        log.LogDebug("%s%s", std::string((level + levelOffset) * 2, ' ').c_str(), e.what());
    };
}

}
