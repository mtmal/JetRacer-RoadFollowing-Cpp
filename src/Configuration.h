////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023 Mateusz Malinowski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <unordered_map>

/** 
 * A class that extends unordered_map to add loading configuration from a file.
 */
class Configuration : public std::unordered_map<std::string, std::string>
{
public:
    /**
     * Basic constructor.
     */
    Configuration();

    /**
     * Basic destructor.
     */
    virtual ~Configuration();

    /**
     * Loads configuration from the file. This function first clears old configuration only if the file was successfully opened.
     *  @param path a path to configuration file.
     *  @param delimiter an optional delimiter for key-value pairs.
     *  @param comment an optional character which comments a line in the configuration file.
     *  @return true if the file was successfully opened.
     */
    bool loadConfiguration(const std::string& path, const std::string& delimiter = "=", const char comment = '#');
};
