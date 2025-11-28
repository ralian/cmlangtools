#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "cmListFileLexer.h"
#include "cmListFileCache.h"

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

    // Parse the input
    cmListFileLexer* lexer = cmListFileLexer_New();
    if (!lexer) {
        std::cerr << "Error: Failed to create lexer\n";
        return 1;
    }

    if (!cmListFileLexer_SetString(lexer, input.c_str())) {
        cmListFileLexer_Delete(lexer);
        std::cerr << "Error: Failed to set input\n";
        return 1;
    }

    std::vector<std::pair<std::string, std::vector<std::string>>> commands;
    std::string currentCommand;
    std::vector<std::string> currentArgs;
    bool inCommand = false;
    bool haveNewline = true;
    int parenDepth = 0;

    while (cmListFileLexer_Token* token = cmListFileLexer_Scan(lexer)) {
        if (token->type == cmListFileLexer_Token_Space) {
            continue;
        } else if (token->type == cmListFileLexer_Token_Newline) {
            haveNewline = true;
            if (inCommand && parenDepth == 0) {
                // End of command
                if (!currentCommand.empty()) {
                    commands.push_back({currentCommand, currentArgs});
                }
                currentCommand.clear();
                currentArgs.clear();
                inCommand = false;
            }
        } else if (token->type == cmListFileLexer_Token_CommentBracket) {
            haveNewline = false;
        } else if (token->type == cmListFileLexer_Token_Identifier) {
            if (haveNewline && !inCommand) {
                haveNewline = false;
                inCommand = true;
                currentCommand = std::string(token->text, token->length);
                currentArgs.clear();
                parenDepth = 0;
            } else if (inCommand) {
                // Argument
                currentArgs.push_back(std::string(token->text, token->length));
            } else {
                throw std::runtime_error("Parse error: Expected newline before identifier");
            }
        } else if (token->type == cmListFileLexer_Token_ParenLeft) {
            if (inCommand) {
                parenDepth++;
                if (parenDepth == 1) {
                    // Opening paren of command - skip
                    continue;
                } else {
                    // Nested paren - add as argument
                    currentArgs.push_back("(");
                }
            } else {
                throw std::runtime_error("Parse error: Unexpected '('");
            }
        } else if (token->type == cmListFileLexer_Token_ParenRight) {
            if (inCommand) {
                if (parenDepth == 1) {
                    // Closing paren of command
                    parenDepth = 0;
                    if (!currentCommand.empty()) {
                        commands.push_back({currentCommand, currentArgs});
                    }
                    currentCommand.clear();
                    currentArgs.clear();
                    inCommand = false;
                } else {
                    parenDepth--;
                    currentArgs.push_back(")");
                }
            } else {
                throw std::runtime_error("Parse error: Unexpected ')'");
            }
        } else if (token->type == cmListFileLexer_Token_ArgumentUnquoted ||
                   token->type == cmListFileLexer_Token_ArgumentQuoted ||
                   token->type == cmListFileLexer_Token_ArgumentBracket) {
            if (inCommand)
                currentArgs.push_back(std::string(token->text, token->length));
            else
                throw std::runtime_error("Parse error: Argument outside of command");
        } else if (token->type == cmListFileLexer_Token_BadCharacter ||
                   token->type == cmListFileLexer_Token_BadBracket ||
                   token->type == cmListFileLexer_Token_BadString) {
            throw std::runtime_error("Parse error: Bad token encountered");
        }
    }

    // Handle any remaining command
    if (inCommand && !currentCommand.empty()) {
        if (parenDepth == 0)
            commands.push_back({currentCommand, currentArgs});
        else
            throw std::runtime_error("Parse error: Unclosed parentheses");
    }

    cmListFileLexer_Delete(lexer);

    for (const auto& command : commands) {
        std::cout << "Command: " << command.first << "\n";
        for (const auto& arg : command.second) {
            std::cout << "  Argument: " << arg << "\n";
        }
    }

    return 0;
}
