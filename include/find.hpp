/*
* Copyright 2017 apfeltee (github.com/apfeltee)
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in the
* Software without restriction, including without limitation the rights to use, copy, modify,
* merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <iostream>
#include <sstream>
#include <exception>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <cwchar>

/*
* these are necessary for platforms that may not support std::filesystem.
* recent MSVC and GCC versions support std::filesystem - but
* GCC seems to still keep it in the std::experimental namespace, hence
* this wrapper.
*/

#if defined(_MSC_VER) || defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 7))
    #if defined(__GNUC__) || defined(_MSC_VER)
        #include <experimental/filesystem>
        #if !defined(__GNUC__)
            #include <filesystem>
        #endif
        namespace std
        {
            namespace filesystem
            {
                using namespace std::experimental::filesystem::v1;
            }
        }
    #else
        #include <filesystem>
    #endif
#else
    #include <boost/filesystem.hpp>
    namespace std
    {
        namespace filesystem
        {
            using namespace boost::filesystem;
        }
    }
#endif

/*
#if __has_include(<filesystem>)
    #include <filesystem>
#else
    #if __has_include(<experimental/filesystem>)
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
        }
    #endif
#endif
*/

namespace Find
{
    class Finder
    {
        public:
            using PruneIfFunc     = std::function<bool(const std::filesystem::path&)>;
            using SkipFunc        = std::function<bool(const std::filesystem::path&,bool,bool)>;
            using IgnoreFileFunc  = std::function<bool(const std::filesystem::path&)>;
            using EachFunc        = std::function<void(const std::filesystem::path&)>;
            using ExceptionFunc   = std::function<void(
                std::runtime_error&,
                const std::string&,
                const std::filesystem::path&
            )>;

            struct Config
            {
                bool use_dircache = false;
                size_t max_depth = 0;
            };

        public:
            static bool DirectoryIs(const std::filesystem::path& input, const std::filesystem::path& findme)
            {
                auto filename = input.filename();
                return filename == findme;
            }

            static bool DirectoryIs(const std::filesystem::path& input, const std::vector<std::filesystem::path>& items)
            {
                for(const auto& item: items)
                {
                    if(DirectoryIs(input, item))
                    {
                        return true;
                    }
                }
                return false;
            }

        private:
            template<typename ExcType>
            void forward_exception(ExcType& ex, const std::string& origin, const std::filesystem::path& path)
            {
                if(m_exceptionfunc)
                {
                    m_exceptionfunc(ex, origin, path);
                }
                else
                {
                    throw ex;
                }
            }

            /*
            * this function is necessary, because for some very strange reason,
            * is_symlink() may actually throw an exception ...
            * this only applies when iterating directories, though.
            */
            template<typename Type>
            bool maybe_symlink(const Type& path)
            {
                try
                {
                    return std::filesystem::is_symlink(path);
                }
                /*
                * no point in handling this exception:
                * it doesn't affect the outcome in any meaningful way.
                */
                catch(...)
                {
                }
                return false;
            }

        private:
            std::vector<std::filesystem::path> m_startdirs;
            std::vector<PruneIfFunc> m_prunefuncs;
            std::vector<SkipFunc> m_skipfuncs;
            std::vector<IgnoreFileFunc> m_ignfilefuncs;
            ExceptionFunc m_exceptionfunc;
            Config m_opts;
            size_t m_depthlevel = 1;


        protected:
            void setup(const std::vector<std::filesystem::path>& dirs)
            {
                m_startdirs = dirs;
            }

            bool skipItem(const std::filesystem::path& path, bool isdir, bool isfile)
            {
                for(const auto& fn: m_skipfuncs)
                {
                    if(!fn(path, isdir, isfile))
                    {
                        return true;
                    }
                }
                return false;
            }

            void doWalk(const std::filesystem::path& dir, const EachFunc& eachfn)
            {
                bool isdir;
                bool isfile;
                bool ispruned;
                bool emitme;
                std::error_code ecode;
                std::vector<std::filesystem::path> dircache;
                std::filesystem::directory_iterator end;
                isfile = false;
                isdir = false;
                emitme = true;
                ispruned = false;
                //#if (!defined(__CYGWIN__)) && (!defined(__CYGWIN32__))
                /*
                * this check is necessary because in certain circumstances, passing
                * a non-existant path to recursive_directory_iterator, even without error_code,
                * will not throw an exception ...
                * the solution: check if $iter equals $end, and if it does, also
                * check if $dir exists at all.
                * if those are true, manually throw an exception!
                * as std::filesystem* isn't available for cygwin (yet?), this
                * won't apply for the boost::filesystem wrapper, and in fact, won't compile
                * (due to constructor diffs - boost::system::error_code vs std::error_code).
                *
                * mind you, this is a hack meant to fix something that i really shouldn't
                * have to fix...
                */
                if(!std::filesystem::is_directory(dir, ecode))
                {
                    auto ex = std::filesystem::filesystem_error("not a directory", dir, ecode);
                    forward_exception(ex, "init", dir);
                }
                //#endif
                std::filesystem::directory_iterator iter(dir);
                while(iter != end)
                {
                    auto entry = *iter;
                    try
                    {
                        auto status = entry.status();
                        emitme = true;
                        isdir = std::filesystem::is_directory(status);
                        isfile = std::filesystem::is_regular_file(status);
                        emitme = skipItem(entry, isdir, isfile);
                        if(isdir)
                        {
                            try
                            {
                                if(maybe_symlink(entry) /* && do not follow links? */)
                                {
                                    //iter.disable_recursion_pending();
                                    ispruned = true;
                                }
                                else
                                {
                                    ispruned = false;
                                    for(const auto& prunefn: m_prunefuncs)
                                    {
                                        if(prunefn != nullptr)
                                        {
                                            if(prunefn(entry))
                                            {
                                                //iter.pop();
                                                //iter.no_push();
                                                //iter.disable_recursion_pending();
                                                ispruned = true;
                                                emitme = false;
                                            }
                                        }
                                    }
                                }
                            }
                            catch(std::runtime_error& ex)
                            {
                                forward_exception(ex, "is_symlink", entry);
                            }

                        }
                        else if(isfile)
                        {
                            ispruned = false;
                            for(const auto& filefn: m_ignfilefuncs)
                            {
                                if(filefn(entry))
                                {
                                    //std::cout << "-- filefn returned false for " << entry << std::endl; 
                                    ispruned = true;
                                    emitme = false;
                                }
                            }
                        }
                        if(emitme)
                        {
                            try
                            {
                                /*
                                * calling .path() is likely where an exception may occur.
                                * specifically, for windows, paths containing a ':' will throw exceptions ...
                                * i.e., "foo::bar.txt" will throw, and the "recovered" filename
                                * would be something like "foo-:-:bar.txt" (at least on windows).
                                * this is sadly a limitation of std::filesystem, and i don't think
                                * there's a realistic way of getting around this, save for
                                * manually parsing ...
                                */
                                eachfn(entry.path());
                            }
                            catch(std::runtime_error& ex)
                            {
                                std::string fname;
                                try
                                {
                                    /*
                                    * there is a possibility that this exception happens
                                    * *in* eachfn ...
                                    * for some reason, string() will throw, but wstring() does not.
                                    * this is for the msvcrt impl. not sure why? this hack
                                    * extracts the path somewhat-ish correctly-ish.
                                    * atm, this assumes that wstring() returns UTF-16, so
                                    * this may very well behave differently, and even oddly
                                    * on other platforms.
                                    */
                                    auto ws = entry.path().wstring();
                                    fname.append(static_cast<const char*>((const void*)ws.data()), ws.size()*2);
                                    fname.erase(std::remove(fname.begin(), fname.end(), char(0)), fname.end());
                                }
                                catch(...)
                                {
                                    fname = "[invalid filename?]";
                                }
                                forward_exception(ex, "during_eachfn", fname);
                            }
                        }
                        if(isdir && (!ispruned))
                        {
                            if(m_opts.use_dircache)
                            {
                                dircache.push_back(entry);
                            }
                            else
                            {
                                //if((m_opts.max_depth == 0) && ((m_opts.max_depth != 0) && (m_depthlevel != m_opts.max_depth)))
                                if((m_opts.max_depth == 0) || (m_depthlevel != m_opts.max_depth))
                                {
                                    m_depthlevel++;
                                    doWalk(entry, eachfn);
                                }
                            }
                        }
                    }
                    catch(std::runtime_error& ex)
                    {
                        forward_exception(ex, "item_status", dir);
                    }
                    try
                    {
                        iter++;
                    }
                    catch(std::runtime_error& ex)
                    {
                        forward_exception(ex, "iterator_next", dir);
                        return;
                    }
                }
                if(m_opts.use_dircache)
                {
                    for(auto& p: dircache)
                    {
                        doWalk(p, eachfn);
                    }
                }
            }


        public:
            // no explicit directories specified - use CWD as starting point
            Finder()
            {
                setup({});
            }

            // single directory specified
            Finder(const std::filesystem::path& dir)
            {
                setup({dir});
            }

            // list of dirs specified
            Finder(const std::vector<std::filesystem::path>& dirs)
            {
                setup(dirs);
            }

            void setMaxDepth(size_t md)
            {
                m_opts.max_depth = md;
            }

            size_t getMaxDepth() const
            {
                return m_opts.max_depth;
            }

            void addDirectory(const std::filesystem::path& path)
            {
                m_startdirs.push_back(path);
            }

            /*
            * add a conditional pruning callback.
            * if the callback returns true upon the given directory, it will be pruned
            * exactly how 'find -prune' prunes.
            */
            void pruneIf(const PruneIfFunc& fn)
            {
                m_prunefuncs.push_back(fn);
            }

            /*
            * add a conditional skip callback
            * if the callback returns true upon the given filename, it will not be emitted
            * in .walk()
            */
            void skipItemIf(const SkipFunc& fn)
            {
                m_skipfuncs.push_back(fn);
            }

            void ignoreIf(const IgnoreFileFunc& fn)
            {
                m_ignfilefuncs.push_back(fn);
            }

            void onException(ExceptionFunc fn)
            {
                m_exceptionfunc = std::move(fn);
            }

            void walk(const EachFunc& fn)
            {
                if(m_startdirs.empty())
                {
                    m_startdirs.push_back(std::filesystem::current_path());
                }
                for(const auto& dir: m_startdirs)
                {
                    doWalk(dir, fn);
                }
            }
    };
}  // namespace Find

