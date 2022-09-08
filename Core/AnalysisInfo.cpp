/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "AnalysisInfo.h"

#include "TypeParser.h"

#include <sstream>

namespace ObjectiveNinja {

std::vector<std::string> MethodInfo::selectorTokens() const
{
    std::stringstream r(selectorName.referenced);

    std::string token;
    std::vector<std::string> result;
    while (std::getline(r, token, ':'))
        result.push_back(token);

    return result;
}

std::vector<std::string> MethodInfo::decodedTypeTokens() const
{
    return TypeParser::parseEncodedType(type.referenced);
}

bool MethodListInfo::hasRelativeOffsets() const
{
    return flags & 0x8000;
}

bool MethodListInfo::hasDirectSelectors() const
{
    return flags & 0x4000;
}

bool PropertyListInfo::hasRelativeOffsets() const
{
    return flags & 0x8000;
}

std::string AnalysisInfo::dump() const
{
    return "<unimplemented>";
}

}
