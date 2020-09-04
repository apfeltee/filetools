
#include <iostream>
#include <iostream>
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
    #include <boost/filesystem.hpp>
    namespace std
    {
        namespace filesystem
        {
            using namespace boost::filesystem;
        }
    };
#endif

int main(int argc, char* argv[])
{
    int i;
    std::filesystem::directory_entry thing;
    std::cout << "argc=" << argc << std::endl;
    for(i=1; i<argc; i++)
    {
        thing = argv[i];
        std::cout << "argv[" << i << "] = " << thing << std::endl;
    }
}
