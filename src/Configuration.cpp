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

#include <algorithm>
#include <fstream>
#include "Configuration.h"


Configuration::Configuration() : std::unordered_map<std::string, std::string>()
{
}

Configuration::~Configuration()
{
}

bool Configuration::loadConfiguration(const std::string& path, const std::string& delimiter, const char comment)
{
    std::ifstream cFile(path);
    bool retVal = cFile.is_open();
    if (retVal)
    {
        std::string line;
        clear();

        while (getline(cFile, line))
        {
            line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
            if (!line.empty() && line[0] != comment)
            {
                size_t delimiterPos = line.find(delimiter);
                insert(std::pair<std::string, std::string>(line.substr(0, delimiterPos), line.substr(delimiterPos + 1)));
            }
        }
    }
    return retVal;
}