
#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdexcept>

#if defined(_MSC_VER)
    #include <experimental/filesystem>
    #include <filesystem>
    namespace std
    {
        namespace filesystem
        {
            using namespace std::experimental::filesystem::v1;
        }
    }
#else
    #include <unistd.h>
    #include <boost/filesystem.hpp>
    namespace std
    {
        namespace filesystem
        {
            using namespace boost::filesystem;
        }
    };
    #if defined(BOOST_POSIX_API)
        #warning "using boost posix-api"
    #elif defined(BOOST_WINDOWS_API)
        #warning "using boost windows-api"
    #endif
#endif

#define g(K_NAME, ...) \
    std::cout << "  " << std::setw(25) << #K_NAME << " = " << path.K_NAME(__VA_ARGS__) << std::endl;


static void inspect(const std::filesystem::path& p)
{
    auto path = std::filesystem::absolute(p);
    std::cout << "raw: " << path << std::endl;
    g(is_relative);
    g(is_absolute);
    g(has_root_path);
    g(has_root_name);
    g(has_root_directory);
    g(has_filename);
    g(has_stem);
    g(has_extension);
    g(root_name);
    g(root_directory);
    g(root_path);
    g(relative_path);
    g(parent_path);
    g(filename);
    g(stem);
    g(extension);
    std::cout << "---" << std::endl;
}

int main(int argc, char* argv[])
{
    int i;
    for(i=1; i<argc; i++)
    {
        try
        {
            inspect(argv[i]);
        }
        catch(std::exception& ex)
        {
            std::cout << "exception: " << ex.what() << std::endl;
        }
    }
    return 0;    
}


