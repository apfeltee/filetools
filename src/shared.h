
#pragma once
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <array>
#include <cmath>
#include <cctype>

namespace Shared
{
    template<typename Type, size_t size>
    constexpr size_t arraySize(Type(&)[size])
    {
        return size;
    }

    std::string sizeToReadable(double len, int precision=0);
}
