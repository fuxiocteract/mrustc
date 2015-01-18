/*
 */
#include <iostream>
#include <string>
#include "parse/lex.hpp"
#include "parse/parseerror.hpp"
#include "ast/ast.hpp"
#include <serialiser_texttree.hpp>
#include <cstring>
#include <main_bindings.hpp>


int g_debug_indent_level = 0;

/// main!
int main(int argc, char *argv[])
{
    const char *infile = NULL;
    ::std::string   outfile;
    const char *crate_path = ".";
    const char *emit_type = "c";
    for( int i = 1; i < argc; i ++ )
    {
        const char* arg = argv[i];
        
        if( arg[0] != '-' )
        {
            infile = arg;
        }
        else if( arg[1] != '-' )
        {
            arg ++; // eat '-'
            for( ; *arg; arg ++ )
            {
                switch(*arg)
                {
                // "-o <file>" : Set output file
                case 'o':
                    if( i == argc - 1 ) {
                        // TODO: BAIL!
                        return 1;
                    }
                    outfile = argv[++i];
                    break;
                default:
                    return 1;
                }
            }
        }
        else
        {
            if( strcmp(arg, "--crate-path") == 0 ) {
                if( i == argc - 1 ) {
                    // TODO: BAIL!
                    return 1;
                }
                crate_path = argv[++i];
            }
            else if( strcmp(arg, "--emit") == 0 ) {
                if( i == argc - 1 ) {
                    // TODO: BAIL!
                    return 1;
                }
                emit_type = argv[++i];
            }
            else {
                return 1;
            }
        }
    }
    
    if( outfile == "" )
    {
        outfile = infile;
        outfile += ".o";
    }
    
    Serialiser_TextTree s_tt(::std::cout);
    Serialiser& s = s_tt;
    try
    {
        AST::Crate crate = Parse_Crate(infile);

        //s << crate;
    
        // Resolve names to be absolute names (include references to the relevant struct/global/function)
        ResolvePaths(crate);
        //s << crate;

        // Typecheck / type propagate module (type annotations of all values)
        // - Check all generic conditions (ensure referenced trait is valid)
        //  > Also mark parameter with applicable traits
        Typecheck_GenericBounds(crate);
        // - Check all generic parameters match required conditions
        Typecheck_GenericParams(crate);
        // - Typecheck statics and consts
        // - Typecheck + propagate functions
        //  > Forward pass first
        Typecheck_Expr(crate);

        if( strcmp(emit_type, "ast") == 0 )
        {
            ::std::ofstream os(outfile);
            Serialiser_TextTree os_tt(os);
            ((Serialiser&)os_tt) << crate;
            return 0;
        }
        // Flatten modules into "mangled" set
        AST::Flat flat_crate = Convert_Flatten(crate);

        // Convert structures to C structures / tagged enums
        //Convert_Render(flat_crate, stdout);
    }
    catch(const ParseError::Base& e)
    {
        ::std::cerr << "Parser Error: " << e.what() << ::std::endl;
        return 2;
    }
    return 0;
}
