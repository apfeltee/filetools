
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
    bool sort = true;
    bool readstdin = false;
    bool recursive = false;
    bool printbytes = false;
    size_t max_depth = 0;
    std::optional<std::string> filepath = {};
};

struct Program
{
    struct Item
    {
        bool isdirectory = false;
        size_t sizebytes = 0;
        std::filesystem::path path;
    };

    Config cfg;
    std::vector<Item> items;

    Program(Config c): cfg(c)
    {
    }

    void printItem(const Item& it)
    {
        if(cfg.printbytes)
        {
            std::cout << it.sizebytes;
        }
        else
        {
            std::cout << Shared::sizeToReadable(it.sizebytes, 2);
        }
        std::cout << "\t" << it.path.string();
        if(it.isdirectory)
        {
            std::cout << "/";
        }
        std::cout << std::endl << std::flush;
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

    void finder(const std::filesystem::path& path, size_t maxdepth, bool onlyfiles, std::function<void(const std::filesystem::path&)> fn)
    {
        Find::Finder fi(path);
        if(maxdepth != 0)
        {
            fi.setMaxDepth(maxdepth);
        }
        fi.onException([&](const std::exception& ex, const std::string& orig, const std::filesystem::path& p)
        {
            std::string exmsg;
            exmsg = ex.what();
            exmsg.erase(std::remove(exmsg.begin(), exmsg.end(), '\r'), exmsg.end());
            exmsg.erase(std::remove(exmsg.begin(), exmsg.end(), '\n'), exmsg.end());
            std::cerr << "ERROR: in '" << orig << "': path \"" << p.string() << "\": " << exmsg << std::endl;
        });
        if(onlyfiles)
        {
            fi.skipItemIf([&](const std::filesystem::path& checkthis, bool isdir, bool isfile)
            {
                (void)checkthis;
                (void)isfile;
                return isdir;
            });
        }
        fi.walk(fn);
    }

    size_t sizeOfFile(const std::filesystem::path& path)
    {
        //std::cerr << "sizeOfFile(" << path << ")" << std::endl;
        if(std::filesystem::is_regular_file(path))
        {
            return std::filesystem::file_size(path);
        }
        return 0;
    }

    size_t sizeOfDirectory(const std::filesystem::path& dirn)
    {
        size_t sz;
        sz = 0;
        finder(dirn, 0, true, [&](const std::filesystem::path& path)
        {
            size_t r;
            r = sizeOfFile(path);
            //std::cerr << "sizeOfDirectory:path=" << path << ", size=" << r << std::endl;
            sz += r;
        });
        return sz;
    }

    void handleItem(const std::filesystem::path& fs)
    {
        Item it;
        it.path = fs;
        if(std::filesystem::is_regular_file(fs))
        {
            it.sizebytes = sizeOfFile(it.path);
        }
        else if(std::filesystem::is_directory(fs))
        {
            it.isdirectory = true;
            it.sizebytes = sizeOfDirectory(it.path);
        }
        else
        {
            std::cerr << "not a file or directory: " << fs << std::endl;
            return;
        }
        if(cfg.sort)
        {
            items.push_back(it);
        }
        else
        {
            printItem(it);
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
        for(auto& path: std::filesystem::directory_iterator(dirn))
        {
            handleItem(path);
        }
        return true;
    }

    bool main(const std::vector<std::string>& args)
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
            if(args.size() > 0)
            {
                for(auto& arg: args)
                {
                    handleItem(arg);
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
    prs.on({"-s", "--nosort"}, "do not sort sizes", [&]
    {
        cfg.sort = false;
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
    prs.on({"-b", "--bytes"}, "print bytes, instead of units", [&]
    {
        cfg.printbytes = true;
    });
    try
    {
        prs.parse(argc, argv);
        Program pg(cfg);
        if(pg.main(prs.positional()))
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
