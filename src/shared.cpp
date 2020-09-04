
#include "shared.h"

namespace Shared
{
    std::string sizeToReadable(double len, int precision)
    {
        static const char sizes[] = {'B', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 0};
        int asz;
        int order;
        std::stringstream rs;
        order = 0;
        asz = Shared::arraySize(sizes);
        while((len >= 1024) && (order < (asz - 1)))
        {
            order++;
            len = len / 1024;
        }
        if(precision > 0)
        {
            rs << std::fixed << std::showpoint << std::setprecision(precision);
        }
        rs << len << sizes[order];
        return rs.str();
    }
}
