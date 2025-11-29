#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

#include "cmListFileLexer.h"
#include "cmListFileCache.h"

bool ichar_equals(char a, char b)
{
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
}

bool iequals(const std::string& lhs, const std::string& rhs)
{
    return std::ranges::equal(lhs, rhs, ichar_equals);
}

using cm_command = std::pair<std::string, std::vector<std::string>>;

struct lexer_context {
    cmListFileLexer* lexer{ nullptr };
    cmListFileLexer_Token* token{ nullptr };
    std::string current_command{};
    std::vector<std::string> current_args{};
    bool in_command{ false };
    bool have_newline{ true };
    int paren_depth{ 0 };
};

// Take the token pointed to by ctx.token and add it to the context - possibly pushing onto the commands list if the token completes a command.
void handle_token(lexer_context& ctx, std::vector<cm_command>& commands)
{
    if (!ctx.lexer) throw std::runtime_error("Lexer not valid!");
    if (!ctx.token) throw std::runtime_error("Current token not valid!");

    if (ctx.token->type == cmListFileLexer_Token_Space) {
        return;
    } else if (ctx.token->type == cmListFileLexer_Token_Newline) {
        ctx.have_newline = true;
        if (ctx.in_command && ctx.paren_depth == 0) {
            // End of command
            if (!ctx.current_command.empty()) {
                commands.push_back({ctx.current_command, ctx.current_args});
            }
            ctx.current_command.clear();
            ctx.current_args.clear();
            ctx.in_command = false;
        }
    } else if (ctx.token->type == cmListFileLexer_Token_CommentBracket) {
        ctx.have_newline = false;
    } else if (ctx.token->type == cmListFileLexer_Token_Identifier) {
        if (ctx.have_newline && !ctx.in_command) {
            ctx.have_newline = false;
            ctx.in_command = true;
            ctx.current_command = std::string(ctx.token->text, ctx.token->length);
            ctx.current_args.clear();
            ctx.paren_depth = 0;
        } else if (ctx.in_command) {
            // Argument
            ctx.current_args.push_back(std::string(ctx.token->text, ctx.token->length));
        } else {
            throw std::runtime_error("Parse error: Expected newline before identifier");
        }
    } else if (ctx.token->type == cmListFileLexer_Token_ParenLeft) {
        if (ctx.in_command) {
            ctx.paren_depth++;
            if (ctx.paren_depth == 1) {
                // Opening paren of command - skip
                return;
            } else {
                // Nested paren - add as argument
                ctx.current_args.push_back("(");
            }
        } else {
            throw std::runtime_error("Parse error: Unexpected '('");
        }
    } else if (ctx.token->type == cmListFileLexer_Token_ParenRight) {
        if (ctx.in_command) {
            if (ctx.paren_depth == 1) {
                // Closing paren of command
                ctx.paren_depth = 0;
                if (!ctx.current_command.empty()) {
                    commands.push_back({ctx.current_command, ctx.current_args});
                }
                ctx.current_command.clear();
                ctx.current_args.clear();
                ctx.in_command = false;
            } else {
                ctx.paren_depth--;
                ctx.current_args.push_back(")");
            }
        } else {
            throw std::runtime_error("Parse error: Unexpected ')'");
        }
    } else if (ctx.token->type == cmListFileLexer_Token_ArgumentUnquoted ||
               ctx.token->type == cmListFileLexer_Token_ArgumentQuoted ||
               ctx.token->type == cmListFileLexer_Token_ArgumentBracket) {
        if (ctx.in_command)
            ctx.current_args.push_back(std::string(ctx.token->text, ctx.token->length));
        else
            throw std::runtime_error("Parse error: Argument outside of command");
    } else if (ctx.token->type == cmListFileLexer_Token_BadCharacter ||
            ctx.token->type == cmListFileLexer_Token_BadBracket ||
            ctx.token->type == cmListFileLexer_Token_BadString) {
        throw std::runtime_error("Parse error: Bad token encountered");
    }
}

// A simple self test, just print all commands and their arguments.
// todo: we should pass an output stream to this, so we can write to a file or different buffer instead of stdout.
std::string cml0_print_all(const std::vector<cm_command>& commands)
{
    for (const auto& command : commands) {
        std::cout << "Command: " << command.first << "\n";
        for (const auto& arg : command.second) {
            std::cout << "  Argument: " << arg << "\n";
        }
    }
    return "PASS";
}

// CML1: Report a failure if we find a command which could use access specifiers, but does not.
// todo: some of these have mandatory specifiers and can be skipped.
// todo: we need to pass more token location info in from the lexer and report every failure, not just any failure.
std::string cml1_access_specifiers(const std::vector<cm_command>& commands)
{
    for (const auto& command : commands) {
        if (iequals(command.first, "target_compile_definitions")
            || iequals(command.first, "target_compile_options")
            || iequals(command.first, "target_include_directories")
            || iequals(command.first, "target_link_directories")
            || iequals(command.first, "target_link_options")
            || iequals(command.first, "target_link_libraries")
            || iequals(command.first, "target_precompile_headers")
            || iequals(command.first, "target_sources")
        ) {
            bool found_specifier = false;
            for (const auto& arg : command.second) {
                if (iequals(arg, "PRIVATE") || iequals(arg, "INTERFACE") || iequals(arg, "PUBLIC"))
                    found_specifier = true;
            }
            if (!found_specifier) return "FAIL";
        }
    }
    return "PASS";
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    // Read input file
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open input file: " << argv[1] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string input = buffer.str();
    file.close();

    std::vector<cm_command> commands;
    lexer_context ctx;

    auto run_lexer = [&](){
        if (!cmListFileLexer_SetString(ctx.lexer, input.c_str())) {
            cmListFileLexer_Delete(ctx.lexer);
            std::cerr << "Error: Failed to set input\n";
            return 1;
        }
    
        while (ctx.token = cmListFileLexer_Scan(ctx.lexer)) {
            handle_token(ctx, commands);
        }
    
        // Handle any remaining command
        if (ctx.in_command && !ctx.current_command.empty()) {
            if (ctx.paren_depth == 0)
                commands.push_back({ctx.current_command, ctx.current_args});
            else
                throw std::runtime_error("Parse error: Unclosed parentheses");
        }
    };

    ctx.lexer = cmListFileLexer_New();
    try {
        run_lexer();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what();
        return 1;
    }
    cmListFileLexer_Delete(ctx.lexer);

    std::cout << "cml0_print_all         => " << cml0_print_all(commands) << "\n";
    std::cout << "cml1_access_specifiers => " << cml1_access_specifiers(commands) << "\n";

    return 0;
}
