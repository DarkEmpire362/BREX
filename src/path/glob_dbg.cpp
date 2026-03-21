#include "glob.h"
#include "glob_parser.h"
#include "glob_compiler.h"
#include "glob_executor.h"

#include <iostream>

int main(int argc, char **argv) {
    std::string str = "**";
    auto glob = brex::GlobParser::parseGlobCString(str);
    
    // std::u8string str = "";
    // auto glob = brex::GlobParser::parseGlobUnicodeString(str);

    auto compiled_glob = brex::GlobCompiler::compile(glob);
    std::cout << glob->toBSQStandard() << std::endl;

    brex::CString test_str("a");
    // std::cout << ((brex::CompiledExpressionFragment*) compiled_glob->states[3])->exprMachine->stringify() << std::endl;
    std::cout << (int) brex::CGlobExecutor(compiled_glob).match(&test_str) << std::endl;

    // brex::UnicodeString test_str(u8"🥒");
    // std::cout << (int) (brex::UnicodeGlobExecutor(compiled_glob).match(&test_str)) << std::endl;
}