
/* must be first header, due to msvc stuff */
#include "glue.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <functional>
#include <string>
#include <cstdio>
#if defined(_WIN32)
    #include <io.h>
    #include <fcntl.h>
#endif
#include "shared.h"
#include "find.hpp"
#include "optionparser.hpp"

struct Config
{
    bool sort = false;
    bool readstdin = false;
    bool recursive = false;
    size_t max_depth = 0;
    std::optional<std::string> filepath = {};
};

struct Program
{
    struct Item
    {
        size_t sizebytes;
        std::filesystem::path path;
    };

    Config cfg;
    std::vector<std::string> dirs;
    std::vector<Item> items;

    Program(Config c, const std::vector<std::string>& d): cfg(c), dirs(d)
    {
    }

    void printItem(const Item& it)
    {
        std::string sizestr = Shared::sizeToReadable(it.sizebytes, 2);
        std::cout << sizestr << "\t" << it.path.string() << std::endl;
        std::cout << std::flush;
    }

    void printSorted()
    {
        std::sort(items.begin(), items.end(), [](const Item& lhs, const Item& rhs)
        {
            return (lhs.sizebytes < rhs.sizebytes);
        });
        for(auto& it: items)
        {
            printItem(it);
        }
    }

    void handleItem(const std::string& fs)
    {
        if(std::filesystem::is_regular_file(fs))
        {
            Item it;
            it.path = std::filesystem::path(fs);
            it.sizebytes = std::filesystem::file_size(it.path);
            if(cfg.sort)
            {
                items.push_back(it);
            }
            else
            {
                printItem(it);
            }
        }
        else
        {
            std::cerr << "not a file: " << fs << std::endl;
        }
    }

    template<typename StreamT>
    bool readStream(StreamT& fh)
    {
        std::string line;
        line.reserve(1024 * 8);
        while(std::getline(fh, line))
        {
            handleItem(line);
            line.clear();
        }
        return true;
    }

    bool readStdin()
    {
        return readStream(std::cin);
    }

    bool readFile()
    {
        std::fstream fh(cfg.filepath.value(), std::ios::in | std::ios::binary);
        if(fh.good())
        {
            readStream(fh);
        }
        return false;
    }

    bool readDir(const std::string& dirn)
    {
        size_t md;
        Find::Finder fi(dirn);
        md = 1;
        if(cfg.max_depth > 0)
        {
            md = cfg.max_depth;
        }
        if(!cfg.recursive)
        {
            fi.setMaxDepth(md);
        }
        fi.onException([&](const std::exception& ex, const std::string& orig, const std::filesystem::path& p)
        {
            std::string exmsg;
            exmsg = ex.what();
            exmsg.erase(std::remove(exmsg.begin(), exmsg.end(), '\r'), exmsg.end());
            exmsg.erase(std::remove(exmsg.begin(), exmsg.end(), '\n'), exmsg.end());
            std::cerr << "ERROR: in '" << orig << "': path \"" << p.string() << "\": " << exmsg << std::endl;
        });
        fi.skipItemIf([&](const std::filesystem::path& checkthis, bool isdir, bool isfile)
        {
            (void)checkthis;
            (void)isfile;
            return isdir;
        });
        fi.walk([&](const std::filesystem::path& path)
        {
            handleItem(path);
        });
        return true;
    }

    bool main()
    {
        if(cfg.readstdin)
        {
            readStdin();
        }
        else if(cfg.filepath)
        {
            if(!readFile())
            {
                return false;
            }
        }
        else
        {
            if(dirs.size() > 0)
            {
                for(auto& dir: dirs)
                {
                    readDir(dir);
                }
            }
            else
            {
                readDir(".");
            }
        }
        return true;
    }
};

int main(int argc, char* argv[])
{
    Config cfg;
    OptionParser prs;
    prs.on({"-s", "--sort"}, "sort sizes", [&]
    {
        cfg.sort = true;
    });
    prs.on({"-i", "--stdin"}, "read filepaths from stdin", [&]
    {
        cfg.readstdin = true;
    });
    prs.on({"-f<file>", "--file=<file>"}, "read filepaths from <file>", [&](auto& v)
    {
        cfg.filepath = v.str();
    });
    prs.on({"-d<n>", "--depth=<n>"}, "set maximum depth", [&](auto& v)
    {
        cfg.max_depth = std::stoi(v.str());
    });
    prs.on({"-r", "--recursive"}, "recurse directories", [&]
    {
        cfg.recursive = true;
    });
    try
    {
        prs.parse(argc, argv);
        Program pg(cfg, prs.positional());
        if(pg.main())
        {
            if(cfg.sort)
            {
                pg.printSorted();
            }
            return 0;
        }
        return 1;
    }
    catch(std::runtime_error& e)
    {
        std::cerr << "exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
