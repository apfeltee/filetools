
#define _CRT_SECURE_NO_WARNINGS
//#define _CRT_NON_CONFORMING_SWPRINTFS
//#define _DEBUG

#include <iostream>
#include <sstream>
#include "find.hpp"

int main(int argc, char* argv[])
{
    std::string dir;
    dir = ".";
    if(argc > 1)
    {
        dir = argv[1];
    }
    Find::Finder fi(dir);
    
    fi.pruneIf([&](const std::filesystem::path& path)
    {
        auto filename = path.filename();
        return (
            (filename == "$RECYCLE.BIN") ||
            (filename == "cygwin")
        );
    });
    
    fi.skipItemIf([&](const std::filesystem::path& path, bool isdir, bool isfile)
    {
        (void)path;
        (void)isdir;
        (void)isfile;
        return (isdir == true);
    });
    /*
    fi.ignoreIf([&](const std::filesystem::path& path)
    {
        //std::cout << "ignoreIf::extension=" << path.extension() << std::endl;
        return (path.extension() == ".sh");
    });
    */
    fi.onException([&](std::runtime_error& ex, const std::string& msg, const std::filesystem::path& path)
    {
        std::cerr << "walk() exception: " << ex.what() << std::endl;
    });
    try
    {
        fi.walk([&](const std::filesystem::path& path)
        {
            auto wpath = path.string();
            std::cout << path << std::endl;
            //std::wprintf(L"%.*s\n", wpath.size(), wpath.data());
            //sfprintf<wchar_t>(std::wcout, L"%s\n", wpath);
            //std::wcout << L"+";
            //std::wcout.write(wpath.data(), wpath.size());
            //std::wcout << std::endl;
        });
    }
    catch(std::runtime_error& ex)
    {
        std::cout << "directory_iterator exception: " << ex.what() << std::endl;
    }
}
