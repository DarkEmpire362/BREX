#include "glob.h"
#include "glob_parser.h"
#include "glob_compiler.h"
#include "glob_machine.h"
#include "glob_executor.h"

#include <iostream>

int main(int argc, char **argv) {
    std::string str = "a${fruit}c";
    auto glob = brex::GlobParser::parseGlobCString(str);
    auto compiled_glob = brex::GlobCompiler::compile(glob);
    std::cout << glob->toBSQStandard() << std::endl;

    std::string substr = "(b|q)";
    auto glob_expr = brex::GlobParser::parseGlobExpressionCString(substr);
    auto compiled_expr = brex::GlobExpressionCompiler::compile(glob_expr);

    std::cout << ((brex::CompiledExpressionFragment*) compiled_glob->states[0])->exprMachine->stringify() << std::endl;

    compiled_glob->link(u8"fruit", compiled_expr);

    std::cout << ((brex::CompiledExpressionFragment*) compiled_glob->states[0])->exprMachine->stringify() << std::endl;

    brex::CString test_str("abc");
    // std::cout << ((brex::CompiledExpressionFragment*) compiled_glob->states[3])->exprMachine->stringify() << std::endl;
    std::cout << (int) brex::CGlobExecutor(compiled_glob).match(&test_str) << std::endl;

    // brex::UnicodeString test_str(u8"🥒");
    // std::cout << (int) (brex::UnicodeGlobExecutor(compiled_glob).match(&test_str)) << std::endl;
}