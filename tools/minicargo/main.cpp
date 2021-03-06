/*
 * Mini version of cargo 
 *
 * main.cpp
 * - Entrypoint
 */
#include <iostream>
#include <cstring>  // strcmp
#include <map>
#include "debug.h"
#include "manifest.h"
#include "helpers.h"
#include "repository.h"
#include "build.h"

struct ProgramOptions
{
    const char* directory = nullptr;

    // Directory containing build script outputs
    const char* override_directory = nullptr;

    // Directory containing "vendored" (packaged) copies of packages
    const char* vendor_dir = nullptr;

    const char* output_directory = nullptr;

    int parse(int argc, const char* argv[]);
    void usage() const;
};

int main(int argc, const char* argv[])
{
    ProgramOptions  opts;
    if( opts.parse(argc, argv) ) {
        return 1;
    }

    try
    {
        // Load package database
        Repository repo;
        // TODO: load repository from a local cache
        if( opts.vendor_dir )
        {
            repo.load_vendored(opts.vendor_dir);
        }

        auto bs_override_dir = opts.override_directory ? ::helpers::path(opts.override_directory) : ::helpers::path();

        // 1. Load the Cargo.toml file from the passed directory
        auto dir = ::helpers::path(opts.directory ? opts.directory : ".");
        auto m = PackageManifest::load_from_toml( dir / "Cargo.toml" );

        // 2. Load all dependencies
        m.load_dependencies(repo, !bs_override_dir.is_valid());

        // 3. Build dependency tree and build program.
        if( !MiniCargo_Build(m, bs_override_dir ) )
        {
            ::std::cerr << "BUILD FAILED" << ::std::endl;
#if _WIN32
            ::std::cout << "Press enter to exit..." << ::std::endl;
            ::std::cin.get();
#endif
            return 1;
        }
    }
    catch(const ::std::exception& e)
    {
        ::std::cerr << "EXCEPTION: " << e.what() << ::std::endl;
#if _WIN32
        ::std::cout << "Press enter to exit..." << ::std::endl;
        ::std::cin.get();
#endif
        return 1;
    }

#if _WIN32
    ::std::cout << "Press enter to exit..." << ::std::endl;
    ::std::cin.get();
#endif
    return 0;
}

int ProgramOptions::parse(int argc, const char* argv[])
{
    bool all_free = false;
    for(int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];
        if( arg[0] != '-' || all_free )
        {
            // Free arguments
            if( !this->directory ) {
                this->directory = arg;
            }
            else {
            }
        }
        else if( arg[1] != '-' )
        {
            // Short arguments
        }
        else if( arg[1] == '\0' )
        {
            all_free = true;
        }
        else
        {
            // Long arguments
            if( ::std::strcmp(arg, "--script-overrides") == 0 ) {
                if(i+1 == argc) {
                    ::std::cerr << "Flag " << arg << " takes an argument" << ::std::endl;
                    return 1;
                }
                this->override_directory = argv[++i];
            }
            else if( ::std::strcmp(arg, "--vendor-dir") == 0 ) {
                if(i+1 == argc) {
                    ::std::cerr << "Flag " << arg << " takes an argument" << ::std::endl;
                    return 1;
                }
                this->vendor_dir = argv[++i];
            }
            else {
                ::std::cerr << "Unknown flag " << arg << ::std::endl;
                return 1;
            }
        }
    }

    if( !this->directory /*|| !this->outfile*/ )
    {
        usage();
        exit(1);
    }

    return 0;
}

void ProgramOptions::usage() const
{
    ::std::cerr
        << "Usage: minicargo <package dir>" << ::std::endl
        ;
}

static int giIndentLevel = 0;
void Debug_Print(::std::function<void(::std::ostream& os)> cb)
{
    for(auto i = giIndentLevel; i --; )
        ::std::cout << " ";
    cb(::std::cout);
    ::std::cout << ::std::endl;
}
void Debug_EnterScope(const char* name, dbg_cb_t cb)
{
    for(auto i = giIndentLevel; i --; )
        ::std::cout << " ";
    ::std::cout << ">>> " << name << "(";
    cb(::std::cout);
    ::std::cout << ")" << ::std::endl;
    giIndentLevel ++;
}
void Debug_LeaveScope(const char* name, dbg_cb_t cb)
{
    giIndentLevel --;
    for(auto i = giIndentLevel; i --; )
        ::std::cout << " ";
    ::std::cout << "<<< " << name << ::std::endl;
}

