
#include "shared.h"

static const auto UNITS = std::map<char, int64_t>{
    {'B', 1},
    {'K', 1024},
    {'M', 1048576},
    {'G', 1073741824},
    {'T', 1099511627776},
    {'P', 1125899906842624},
    {'E', 1152921504606846976},
};

struct MkSum
{
    int64_t m_bytes = 0;

    void processLine(const std::string& line)
    {
        int ch;
        size_t ofs;
        size_t efs;
        double dval;
        double byteval;
        std::string nstr;
        ofs = 0;
        // skip initial spaces
        while(std::isspace(line[ofs]))
        {
            ofs++;
        }
        // continue only if line wasn't empty
        if(ofs < line.length())
        {
            efs = 0;
            // search for digits
            while(std::isdigit(line[ofs + efs]) || (line[ofs + efs] == '.'))
            {
                efs++;
            }
            // if efs equals ofs, then there were none!
            if(efs == ofs)
            {
                fprintf(stderr, "error: line does not begin with numbers: %.*s\n", int(line.size()), line.data());
            }
            else
            {
                ch = std::toupper(line[efs]);
                nstr = line.substr(ofs, efs);
                auto unit = UNITS.find(ch);
                if(unit != UNITS.end())
                {
                    dval = std::stod(nstr);
                    byteval = (dval * unit->second);
                    #if 0
                    fprintf(stderr, "line: <<%.*s>>, nstr=<<%.*s>>, ofs=%d, efs=%d, ch=%c, dval=%f, byteval=%f\n",
                        line.size(), line.c_str(), nstr.size(), nstr.c_str(), ofs, efs, ch, dval, byteval);
                    fprintf(stderr, "m_bytes is now: %d\n", m_bytes);
                    #endif
                    m_bytes += byteval;
                }
                else
                {
                    fprintf(stderr, "error: line has unrecognized unit character '%c': %.*s\n", ch, int(line.size()), line.data());
                }
            }
        }
    }

    void readHandle(std::istream& ifs)
    {
        std::string line;
        // gcc's libstdc++ does not correctly allocate std::string for some reason
        line.reserve(1024);
        while(std::getline(ifs, line))
        {
            processLine(line);
        }
    }

    void result(const std::string& filename)
    {
        std::cout << Shared::sizeToReadable(m_bytes) << " " << filename << " (" << int64_t(m_bytes) << " bytes)" << std::endl;
    }
};

void doHandle(std::istream& ifs, const std::string& filename)
{
    MkSum mks;
    mks.readHandle(ifs);
    mks.result(filename);
}


int main(int argc, char* argv[])
{
    int i;
    int errc;
    std::string filename;
    errc = 0;
    if(argc > 1)
    {
        for(i=1; i<argc; i++)
        {
            filename = argv[i];
            std::fstream hnd(filename, std::ios::in | std::ios::binary);
            if(hnd.good())
            {
                doHandle(hnd, filename);
            }
            else
            {
                std::cerr << "failed to open \"" << filename << "\" for reading" << std::endl;
                errc += 1;
            }
        }
    }
    else
    {
        doHandle(std::cin, "<stdin>");
    }
    return ((errc > 0) ? 1 : 0);
}

