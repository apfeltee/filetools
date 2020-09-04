
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

#include "find.hpp"
#include "optionparser.hpp"

#if defined(_MSC_VER)
    #define fileno _fileno
    #define isatty _isatty
#endif

#if defined(_MAX_PATH)
    #define CFILES_MAXPATHLEN _MAX_PATH
#elif defined(PATH_MAX)
    #define CFILES_MAXPATHLEN PATH_MAX
#else
    /*#error please add definition for CFILES_MAXPATHLEN!*/
    /* going with some half-way sensible default here ... */
    #define CFILES_MAXPATHLEN (128+1)
#endif

enum class SortKind
{
    // extension, i.e., "foobar.xyz" -> ".xyz"
    Extension,

    // the stem, i.e, "foobar.xyz" -> "foobar"
    Stem,

    // the whole filename, i.e., "foobar.xyz" -> "foobar.xyz" (duh!)
    // useful for directories that contain many files with the same name
    Filename,
};

struct Config
{
    // what to sort for - default is SortKind::Extension, i.e., file extensions.
    SortKind sortkind = SortKind::Extension;

    // whether to store file extension/stem/name case-insensitively
    bool icase = false;

    // whether to read from standard input.
    // this is handled by '-i' - default behaviour is to read the current directory
    bool readstdin = false;

    // whether to read input from file(s) that contain filenames and/or file paths
    // this is handled via '-f'
    bool readlistings = false;

    // whether to ignore files that lack a file extension
    bool reject_noext = false;

    // whether to sort values by count. defaults to true
    bool sortvals = true;

    // if '-o' is specified, then the file handle must be closed (deleted, actually)
    bool mustclose = false;

    // whether to output in reverse order
    bool revoutput = false;

    // whether to merely collect and print file extensions - no ordering, no counting
    bool collectonly = false;

    // whether to be verbose
    bool verbose = false;

    // maximum depth (this doesn't quite seem to work correctly - see find.hpp)
    size_t maxdepth = 0;

    // where the output is written to. default is std::cout; handled by '-o' flag
    std::ostream* outstream;

    // list of dirs to prune
    std::vector<std::filesystem::path> pruneme = {};
};

class ExtList
{
    public:
        template<typename T>
        using ContainerType = std::deque<T>;

        struct Item
        {
            std::string ext;
            size_t count;
            size_t hash;
        };

    private:
        ContainerType<size_t> m_seen;
        ContainerType<Item> m_items;
        std::hash<std::string> m_hashfn;

    public:
        ExtList()
        {
        }

        auto begin()
        {
            return m_items.begin();
        }

        auto end()
        {
            return m_items.end();
        }

        auto rbegin()
        {
            return m_items.rbegin();
        }

        auto rend()
        {
            return m_items.rend();
        }

        auto byhash(size_t hash)
        {
            return std::find_if(m_items.begin(), m_items.end(),  [&](const Item& item) 
            { 
                return (item.hash == hash);
            });
        }

        bool contains(size_t hash, size_t& idx)
        {
            auto it = std::find(m_seen.begin(), m_seen.end(), hash);
            if(it != m_seen.end())
            {
                idx = std::distance(m_seen.begin(), it);
                return true;
            }
            return false;
        }

        void increase(const std::string& ext)
        {
            size_t idx;
            size_t hash;
            hash = m_hashfn(ext);
            if(contains(hash, idx))
            {
                m_items[idx].count++;
            }
            else
            {
                m_seen.push_back(hash);
                m_items.push_back(Item{ext, 1, hash});
            }
        }
};

class CountFiles
{
    private:
        ExtList m_map;
        Config& m_options;
        size_t m_padding = 5;

    private:
        void checkPadding(size_t slen)
        {
            if(slen > m_padding)
            {
                m_padding = slen;
            }
        }

        // this function will attempt to remove '\r\n'.
        // this only applies to listing files.
        // it will effectively do nothing if the string does not contain a '\r'
        void fixCR(std::string& str)
        {
            if(!str.empty() && (str[str.size() - 1] == '\r'))
            {
                str.erase(str.size() - 1);
            }
        }

        void shortenpath(std::string& rawpath)
        {
            size_t plen;
            while(true)
            {
                plen = rawpath.size();
                rawpath = rawpath.substr(10, plen);
                if(rawpath.size() < CFILES_MAXPATHLEN)
                {
                    break;
                }
            }
        }

        void push(const std::string& val)
        {
            m_map.increase(val);
        }

    public:
        CountFiles(Config& opts): m_options(opts)
        {
        }

        std::ostream& out()
        {
            return *(m_options.outstream);
        }

        template<typename... Args>
        void verbose(const char* fmt, Args&&... args)
        {
            if(m_options.verbose)
            {
                std::fprintf(stderr, "[v] ");
                std::fprintf(stderr, fmt, args...);
                std::fprintf(stderr, "\n");
            }
        }

        // this function is where post-processing (like turning strings lowercase)
        // happens. new options and/or functionality that directly operate
        // on the input string should be added here.
        void increase(const std::string& val)
        {
            std::string copy;
            checkPadding(val.size());
            if(m_options.icase)
            {
                copy = val;
                std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
                push(copy);
            }
            else
            {
                push(val);
            }
        }

        void modeExtension(const std::filesystem::path& item)
        {
            std::string strext;
            std::string bnamestr;
            std::filesystem::path bname;
            bname = item.filename();
            bnamestr = bname.string();
            /*
            * if the item path is something like "foo/bar/", then
            * bname is just an empty string, and doesn't contain anything to work with.
            * theoretically, this shouldn't happen, though.
            */
            if(!bnamestr.empty())
            {
                strext = bname.extension().string();
                /*
                * in some super funky cases, the extension might be something
                * like "." (i.e., "foo."). i don't who or why someone would
                * name a file like this, but still.
                */
                if(strext.size() > 1)
                {
                    increase(strext);
                }
                else
                {
                    if(!m_options.reject_noext)
                    {
                        increase(bname.string());
                    }
                }
            }
        }

        void modeStem(const std::filesystem::path& item)
        {
            std::string stemstr;
            std::filesystem::path stem;
            stem = item.stem();
            stemstr = stem.string();
            increase(stemstr);
        }

        /*
        void modeFilesize(const std::filesystem::path& item)
        {
            
        }
        */

        void handleItem(const std::filesystem::path& item)
        {
            switch(m_options.sortkind)
            {
                case SortKind::Extension:
                    return modeExtension(item);
                case SortKind::Stem:
                    return modeStem(item);
                /*
                case SortKind::Filesize:
                    return modeFilesize(item);
                */
                default:
                    std::cerr << "unimplemented sort kind" << std::endl;
                    std::exit(1);
                    break;
            }
        }

        void walkFilestream(std::istream& infh)
        {
            std::string line;
            line.reserve(1024 * 8);
            while(std::getline(infh, line))
            {
                //fixCR(line);
                if(line.size() >= CFILES_MAXPATHLEN)
                {
                    //shortenpath(line);
                }
                verbose("line: %s", line.c_str());
                try
                {
                    handleItem(line);
                }
                catch(std::exception& ex)
                {
                    std::cerr << "in walkFilestream: " << ex.what() << "(line: \"" << line << "\")" << std::endl;
                }
            }
        }

        void walkDirectory(const std::string& dir)
        {
            Find::Finder fi(dir);
            fi.setMaxDepth(m_options.maxdepth);
            fi.onException([&](const std::exception& ex, const std::string& orig, const std::filesystem::path& p)
            {
                std::string exmsg;
                exmsg = ex.what();
                // FormatMessage() includes CRLF for some reason, so remove that
                exmsg.erase(std::remove(exmsg.begin(), exmsg.end(), '\r'), exmsg.end());
                exmsg.erase(std::remove(exmsg.begin(), exmsg.end(), '\n'), exmsg.end());
                std::cerr << "ERROR: in '" << orig << "': path \"" << p.string() << "\": " << exmsg << std::endl;
            });
            fi.skipItemIf([&](const std::filesystem::path& checkthis, bool isdir, bool isfile)
            {
                (void)isfile;
                if(isdir)
                {
                    verbose("current path: %s", checkthis.string().c_str());
                }
                return (isdir);
            });

            fi.pruneIf([&](const std::filesystem::path& checkthis)
            {
                for(auto& prunethis: m_options.pruneme)
                {
                    if(prunethis.empty())
                    {
                        continue;
                    }
                    //std::cerr << "checkthis.parent_path() = " << checkthis.parent_path() << std::endl;
                    //std::cerr << "prunethis.parent_path() = " << prunethis.parent_path().string() << std::endl;
                    // todo: add option to ignore case maybe?
                    // first, check if the paths match as-is ...
                    if(checkthis == prunethis)
                    {
                        return true;
                    }
                    // then, check if filename (in case of directories, the dirname) is equal ...
                    if(checkthis.has_parent_path() && ((!prunethis.empty()) && prunethis.has_parent_path()))
                    {

                        if(checkthis.parent_path() == prunethis.parent_path())
                        {
                            return true;
                        }
                    }
                    // lastly, do a lexical check
                    return ((checkthis.compare(prunethis)) >= 0);
                }
                return false;
            });

            fi.walk([&](const std::filesystem::path& path)
            {
                handleItem(path);
            });
        }

        void sort()
        {
            std::sort(m_map.begin(), m_map.end(), [](const ExtList::Item& lhs, const ExtList::Item& rhs)
            {
                return (lhs.count < rhs.count);
            });
        }

        ExtList get()
        {
            return m_map;
        }

        void printVals(const std::string& ext, const size_t& count)
        {
            std::stringstream buf;
            size_t realpad;
            if(m_options.collectonly)
            {
                out() << ext << '\n';
            }
            else
            {
                realpad = (m_padding + 2);
                out() << std::setw(realpad) << ext << " " << count << '\n';
            }
        }

        void printOutput()
        {
            if(m_options.sortvals && (!m_options.collectonly))
            {
                sort();
            }
            if(m_options.revoutput && (!m_options.collectonly))
            {
                for(auto it=m_map.rbegin(); it!=m_map.rend(); it++)
                {
                    printVals(it->ext, it->count);
                }
            }
            else
            {
                for(auto it=m_map.begin(); it!=m_map.end(); it++)
                {
                    printVals(it->ext, it->count);
                }
            }
        }
};

/*
* for some reason, isatty() seems to not work... sometimes.
* haven't been able to figure out why, yet.
* works so far with msvc, clang (for msvc), and gcc.
*/
static bool have_filepipe()
{
    return (isatty(fileno(stdin)) == 0);
}

int main(int argc, char* argv[])
{
    // disables the CRLF bullshit on windows:
    // without it, printing to console makes windows
    // replace LF ("\n") with CRLF ("\r\n"), which
    // messes with cygwin tools
    #if defined(_MSVC) || defined(_WIN32)
        _setmode(1, _O_BINARY);
    #endif

    OptionParser prs;
    Config opts;
    std::fstream* fhptr;
    opts.outstream = &std::cout;
    prs.on({"-i", "--stdin"}, "read input from stdin", [&]
    {
        opts.readstdin = true;
    });
    prs.on({"-r", "--reverse"}, "reverse output", [&]
    {
        opts.revoutput = true;
    });
    prs.on({"-c", "--nocase"}, "turn extensions/names to lowercase", [&]
    {
        opts.icase = true;
    });
    prs.on({"-n", "--nosort"}, "do not sort", [&]
    {
        opts.sortvals = false;
    });
    prs.on({"-d?", "--maxdepth=?"}, "set maximum depth (default is 0; meaning unlimited)", [&](const auto& v)
    {
        opts.maxdepth = v.template as<size_t>();
    });
    prs.on({"-f", "--listing"}, "interpret arguments as a list of files containing paths", [&]
    {
        opts.readlistings = true;
    });
    prs.on({"-e", "--rnoext"}, "skip files that have no extension", [&]
    {
        opts.reject_noext = true;
    });
    prs.on({"-p?", "--prune=?"}, "do not enter the specified directory (prune directory)", [&](const auto& v)
    {
        
        /*
        * outstandingly retarded hack incoming:
        * GNU libstdc++fs is broken enough to attempt to post-parse a path past (!!!) being defined...
        * hence, say, `std::filesystem::path("./foo")` actually parses ::parent_path() as "." ...
        * so this unbelievably awful hack "fixes" this issue by appending a faux path, through which
        * ::parent_path() now points to the actual directory name.
        *
        * sidenote: the C++ committee deserves more than plenty of hate for designing this
        * ambigious garbage in the first place.
        */
        #if defined(__CYGWIN__) || defined(__GNUC__)
            std::filesystem::path dumb("./");
            dumb /= v.str();
            dumb /= "/0";
        #else
            std::filesystem::path dumb(v.str());
        #endif
        //std::cerr << "defined pruning: " << dumb << std::endl;
        opts.pruneme.push_back(dumb);
    });
    prs.on({"-o?", "--output=?"}, "write output to file (default: write to stdout)", [&](const auto& v)
    {
        auto s = v.str();
        fhptr = new std::fstream(s, std::ios::out | std::ios::binary);
        if(!fhptr->good())
        {
            std::cerr << "failed to open '" << s << "' for writing" << '\n';
            std::exit(1);
        }
        opts.outstream = fhptr;
        opts.mustclose = true;
    });
    prs.on({"-m?", "--mode=?"}, "which sort kind to use ('e': extension, 's': stem, 'f': filename. default: 'e')", [&](const auto& v)
    {
        char modech;
        auto s = v.str();
        modech = std::tolower(s[0]);
        switch(modech)
        {
            case 'x':
            case 'e':
                opts.sortkind = SortKind::Extension;
                break;
            case 'f': // 'filename'
            case 'b': // 'basename'
                opts.sortkind = SortKind::Filename;
                break;
            case 'n': // 'name'
            case 's': // 'stem'
                opts.sortkind = SortKind::Stem;
                break;
            default:
                std::cerr << "unknown mode '" << modech << "'" << std::endl;
                std::exit(1);
                break;
        }
    });
    prs.on({"-x", "--collect"}, "collect file modes (extension or otherwise) only, does not print amount", [&]
    {
        opts.collectonly = true;
    });
    prs.on({"-v", "--verbose"}, "enable verbose messages", [&]
    {
        opts.verbose = true;
    });
    try
    {
        prs.parse(argc, argv);
    }
    catch(std::exception& e)
    {
        std::cerr << "error: " << e.what() << '\n';
    }
    //return 0;
    CountFiles cf(opts);
    if((!opts.readstdin) && (prs.size() == 0))
    {
        cf.walkDirectory(".");
    }
    else
    {
        if(opts.readstdin)
        {
            //std::cerr << "reading from stdin" << std::endl;
            if(have_filepipe())
            {
                cf.walkFilestream(std::cin);
            }
            else
            {
                std::cerr << "error: no file piped" << '\n';
                return 1;
            }
        }
        else if(opts.readlistings)
        {
            auto files = prs.positional();
            for(const auto& file: files)
            {
                std::fstream fh(file, std::ios::in | std::ios::binary);
                if(fh.good())
                {
                    //std::cerr << "reading paths from \"" << file << "\" ..." << '\n';
                    cf.walkFilestream(fh);
                }
                else
                {
                    std::cerr << "failed to open \"" << file << "\" for reading" << '\n';
                }
            }
        }
        else
        {
            auto dirs = prs.positional();
            for(const auto& dir: dirs)
            {
                cf.walkDirectory(dir);
            }
        }
    }
    cf.printOutput();
    //std::cerr << "after printOutput" << std::endl;
    if(opts.mustclose)
    {
        delete fhptr;
    }
    return 0;
}
