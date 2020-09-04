
/*
* this file sets up some platform stuff.
*/

#pragma once

/* these are needed for msvc */
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS

#if defined(__unix__) || defined(__linux__) || defined(__CYGWIN__) || defined(__CYGWIN32__)
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>

    /*
    * fix GCCs pants-on-head retarded versioning system, in which
    * stuff like fileno() is no longer declared in anything past c++11.
    * ... and no, this is not exactly cygwin-specific - it just so happens
    * to happen on cygwin.
    */
    #if defined(__CYGWIN__)
        #if !defined(DT_REG)
            #define DT_REG 8
        #endif
        #if !defined(DT_DIR)
            #define DT_DIR 4
        #endif
        extern "C"
        {
            int fileno(FILE*);
        }
    #endif

    /*
    * this doesn't mean we're factually running UNIX, just that
    * these macros indicate that the platform is (somewhat) like unix.
    *
    * also, i don't know if these also apply to android?
    * they should, technically, since only POSIX stuff is needed on unixlike platforms.
    */
    #define COE_ISUNIXLIKE
    #if defined(__linux__)
        #define COE_ISLINUX
    #endif
    #if defined(__CYGWIN__) || defined(__CYGWIN32__)
        #define COE_ISCYGWIN
    #endif
#else
    #if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
        #include <windows.h>
        #include <io.h>
        #define COE_ISWINDOWS

        /* msc "fixes". thanks, microsoft. so helpful. */
        #if defined(_MSC_VER)
            #define isatty _isatty
            #define fileno _fileno
        #endif
        #if defined(_WIN32) || defined(_WIN64)
            #if defined(_WIN32)
                #define COE_ISWINNT32
            #endif
            #if defined(_WIN64)
                #define COE_ISWINNT64
            #endif
        #endif
    #else
        #define COE_ISUNKNOWN
    #endif
#endif

#if defined(COE_ISUNKNOWN)
    #warning "your platform might not be supported!"
#endif
