// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef PATH_UTIL_HPP
#define PATH_UTIL_HPP

#include <lib/data/string.hpp>

// Splits a file/dir path into tokens (returns every dir/file going into the path in an array given by the user and stores count)
data::string* split_path_tokens(data::string path, int& count);

#endif // PATH_UTIL_HPP
