#pragma once

#include <binaryninjaapi.h>

#include <cstddef>
#include <exception>
#include <functional>

namespace ObjectiveNinja {

/**
 * Utilities for C++ exceptions.
 */
class ExceptionUtils {
public:
    /**
     * Perform an action for each nesting level of an exception
     */
    static void forNested(std::exception_ptr eptr,
        std::function<void(const std::exception&, std::size_t level)> action);

    /**
     * Perform an action for each nesting level of the current exception
     */
    static void forCurrentNested(
        std::function<void(const std::exception&, std::size_t level)> action);

    /**
     * Perform an action for each nesting level of the current exception
     */
    static std::function<void(const std::exception&, std::size_t level)> logDebugAction(
        BinaryNinja::Logger& log, std::size_t levelOffset);
};

}
