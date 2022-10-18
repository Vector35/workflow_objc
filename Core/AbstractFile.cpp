/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "AbstractFile.h"

#include <exception>
#include <ios>
#include <sstream>

namespace ObjectiveNinja {

uint8_t AbstractFile::readByte(uint64_t offset) try
{
    seek(offset);
    return readByte();
} catch (...) {
    auto msg = std::ostringstream("AbstractFile::readByte(", std::ios_base::app);
    msg << std::hex << std::showbase << offset << ") failed";
    std::throw_with_nested(std::runtime_error(msg.str()));
}

uint16_t AbstractFile::readShort(uint64_t offset) try
{
    seek(offset);
    return readShort();
} catch (...) {
    auto msg = std::ostringstream("AbstractFile::readShort(", std::ios_base::app);
    msg << std::hex << std::showbase << offset << ") failed";
    std::throw_with_nested(std::runtime_error(msg.str()));
}

uint32_t AbstractFile::readInt(uint64_t offset) try
{
    seek(offset);
    return readInt();
} catch (...) {
    auto msg = std::ostringstream("AbstractFile::readInt(", std::ios_base::app);
    msg << std::hex << std::showbase << offset << ") failed";
    std::throw_with_nested(std::runtime_error(msg.str()));
}

uint64_t AbstractFile::readLong(uint64_t offset) try
{
    seek(offset);
    return readLong();
} catch (...) {
    auto msg = std::ostringstream("AbstractFile::readLong(", std::ios_base::app);
    msg << std::hex << std::showbase << offset << ") failed";
    std::throw_with_nested(std::runtime_error(msg.str()));
}

std::string AbstractFile::readString(size_t maxLength) try
{
    std::string result;

    while (maxLength == 0 || result.size() <= maxLength) {
        char c = static_cast<char>(readByte());

        if (c == 0)
            break;

        result += c;
    }

    return result;
} catch (...) {
    auto msg = std::ostringstream("AbstractFile::readString(", std::ios_base::app);
    msg << std::hex << std::showbase << maxLength << ") failed";
    std::throw_with_nested(std::runtime_error(msg.str()));
}

std::string AbstractFile::readStringAt(uint64_t address, size_t maxLength) try
{
    seek(address);
    return readString(maxLength);
} catch (...) {
    auto msg = std::ostringstream("AbstractFile::readStringAt(", std::ios_base::app);
    msg << std::hex << std::showbase << address << ", ...) failed";
    std::throw_with_nested(std::runtime_error(msg.str()));
}

}
