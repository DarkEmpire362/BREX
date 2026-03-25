#include <boost/test/unit_test.hpp>

#include "../../src/path/glob.h"
#include "../../src/path/glob_parser.h"
#include "../../src/path/glob_compiler.h"
#include "../../src/path/glob_executor.h"

std::optional<brex::FragmentMachine*> tryCompileGlobC(const std::string& globstr) {
    auto ast = brex::GlobParser::parseGlobCString(globstr);
    // TODO: Check Errs
    auto machine = brex::GlobCompiler::compile(ast);
    // TODO: Check Errs
    return std::make_optional(machine);
}

std::optional<brex::ExpressionMachine*> tryCompileGlobExpressionC(const std::string& globstr) {
    auto ast = brex::GlobParser::parseGlobExpressionCString(globstr);
    auto machine = brex::GlobExpressionCompiler::compile(ast);
    return std::make_optional(machine);
}

std::optional<brex::FragmentMachine*> tryCompileGlobUnicode(const std::u8string& globstr) {
    auto ast = brex::GlobParser::parseGlobUnicodeString(globstr);
    // TODO: Check Errs
    auto machine = brex::GlobCompiler::compile(ast);
    // TODO: Check Errs
    return std::make_optional(machine);
}

std::optional<brex::ExpressionMachine*> tryCompileGlobExpressionUnicode(const std::u8string& globstr) {
    auto ast = brex::GlobParser::parseGlobExpressionUnicodeString(globstr);
    auto machine = brex::GlobExpressionCompiler::compile(ast);
    return std::make_optional(machine);
}

bool accepts_cstr(brex::FragmentMachine* machine, const std::string& test) {
    brex::CString str(test);
    return brex::CGlobExecutor(machine).match(&str);
}

bool accepts_unicode(brex::FragmentMachine* machine, const std::u8string& test) {
    brex::UnicodeString str(test);
    return brex::UnicodeGlobExecutor(machine).match(&str);
}

#define ACCEPTS_CSTR(RE, STR) { BOOST_CHECK(accepts_cstr(RE, STR)); }
#define REJECTS_CSTR(RE, STR) { BOOST_CHECK(!accepts_cstr(RE, STR)); }

#define ACCEPTS_UNICODE(RE, STR) { BOOST_CHECK(accepts_unicode(RE, STR)); }
#define REJECTS_UNICODE(RE, STR) { BOOST_CHECK(!accepts_unicode(RE, STR)); }

BOOST_AUTO_TEST_SUITE(Glob)
    BOOST_AUTO_TEST_SUITE(Literals)
        BOOST_AUTO_TEST_SUITE(CString)
            BOOST_AUTO_TEST_CASE(empty) {
                auto glob = tryCompileGlobC("");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "");
                REJECTS_CSTR(mach, "a");
            }

            BOOST_AUTO_TEST_CASE(abc) {
                auto glob = tryCompileGlobC("abc");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                REJECTS_CSTR(mach, "a");
                REJECTS_CSTR(mach, "ab");
                ACCEPTS_CSTR(mach, "abc");
                REJECTS_CSTR(mach, "abcd");
                REJECTS_CSTR(mach, "aaa");
            }

            BOOST_AUTO_TEST_CASE(escape) {
                // TODO: This test is currently failing in the executor. It
                //   calls code to convert the input string to unescaped regex,
                //   but the percent symbol is being treated as part of an
                //   escape when it shouldn't be. Fix coming.
                auto glob = tryCompileGlobC("%%;%underscore;%x61;");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "%_a")
            }
        BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_SUITE(Unicode)
            BOOST_AUTO_TEST_CASE(empty) {
                auto glob = tryCompileGlobUnicode(u8"");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"");
                REJECTS_UNICODE(mach, u8"a");
            }

            BOOST_AUTO_TEST_CASE(abc) {
                auto glob = tryCompileGlobUnicode(u8"abc");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                REJECTS_UNICODE(mach, u8"a");
                REJECTS_UNICODE(mach, u8"ab");
                ACCEPTS_UNICODE(mach, u8"abc");
                REJECTS_UNICODE(mach, u8"abcd");
                REJECTS_UNICODE(mach, u8"aaa");
            }

            BOOST_AUTO_TEST_CASE(escape) {
                // TODO: Failing, same as CStr Escape test.
                auto glob = tryCompileGlobUnicode(u8"%%;%underscore;%x61;");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                REJECTS_UNICODE(mach, u8"%_");
                REJECTS_UNICODE(mach, u8"_a");
                ACCEPTS_UNICODE(mach, u8"%_a");
                REJECTS_UNICODE(mach, u8"%_b");
            }

            BOOST_AUTO_TEST_CASE(cucumber) {
                auto glob = tryCompileGlobUnicode(u8"🥒");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"🥒");
                REJECTS_UNICODE(mach, u8"cucumber");
                REJECTS_UNICODE(mach, u8"🫑")
            }
        BOOST_AUTO_TEST_SUITE_END()
    BOOST_AUTO_TEST_SUITE_END()
    
    // Correctness of Sequence behavior is tested by this as well. 
    // *a is a sequence of { WILDCARD, LITERAL("a") }
    BOOST_AUTO_TEST_SUITE(Wildcard)
        BOOST_AUTO_TEST_SUITE(CString)
            BOOST_AUTO_TEST_CASE(singlewild) {
                auto glob = tryCompileGlobC("*");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "abc");
                ACCEPTS_CSTR(mach, "a somewhat large number of characters in one fragment");
                ACCEPTS_CSTR(mach, "*");
                REJECTS_CSTR(mach, "abc/abc");
            }

            BOOST_AUTO_TEST_CASE(wildprefixed) {
                auto glob = tryCompileGlobC("*a");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "a");
                ACCEPTS_CSTR(mach, "ba");
                REJECTS_CSTR(mach, "apple");
                ACCEPTS_CSTR(mach, "banana");
            }

            BOOST_AUTO_TEST_CASE(wildsuffixed) {
                auto glob = tryCompileGlobC("a*");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "a");
                ACCEPTS_CSTR(mach, "ab");
                ACCEPTS_CSTR(mach, "apple");
                REJECTS_CSTR(mach, "banana");
            }

            BOOST_AUTO_TEST_CASE(contains) {
                auto glob = tryCompileGlobC("*a*");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "")
                REJECTS_CSTR(mach, "b");
                ACCEPTS_CSTR(mach, "a");
                ACCEPTS_CSTR(mach, "ba");
                ACCEPTS_CSTR(mach, "ab");
                ACCEPTS_CSTR(mach, "banana");
            }
        BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_SUITE(Unicode)
            BOOST_AUTO_TEST_CASE(single_wild) {
                auto glob = tryCompileGlobUnicode(u8"*");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"");
                ACCEPTS_UNICODE(mach, u8"abc");
                ACCEPTS_UNICODE(mach, u8"a somewhat large number of characters in one fragment")
                ACCEPTS_UNICODE(mach, u8"*");
                ACCEPTS_UNICODE(mach, u8"🍎");
                ACCEPTS_UNICODE(mach, u8"🍌");
            }

            BOOST_AUTO_TEST_CASE(wild_prefixed) {
                auto glob = tryCompileGlobUnicode(u8"*a");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                REJECTS_UNICODE(mach, u8"b");
                REJECTS_UNICODE(mach, u8"ab");
                ACCEPTS_UNICODE(mach, u8"ba");
                ACCEPTS_UNICODE(mach, u8"banana");
                REJECTS_UNICODE(mach, u8"apple");
            }

            BOOST_AUTO_TEST_CASE(wild_suffixed) {
                auto glob = tryCompileGlobUnicode(u8"a*");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                REJECTS_UNICODE(mach, u8"b");
                ACCEPTS_UNICODE(mach, u8"ab");
                REJECTS_UNICODE(mach, u8"ba");
                REJECTS_UNICODE(mach, u8"banana");
                ACCEPTS_UNICODE(mach, u8"apple");
            }

            BOOST_AUTO_TEST_CASE(contains) {
                auto glob = tryCompileGlobUnicode(u8"*a*");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"")
                REJECTS_UNICODE(mach, u8"b");
                ACCEPTS_UNICODE(mach, u8"a");
                ACCEPTS_UNICODE(mach, u8"ba");
                ACCEPTS_UNICODE(mach, u8"ab");
                ACCEPTS_UNICODE(mach, u8"banana");
            }
        BOOST_AUTO_TEST_SUITE_END()
    BOOST_AUTO_TEST_SUITE_END()
    
    BOOST_AUTO_TEST_SUITE(Union)
        BOOST_AUTO_TEST_SUITE(CString)
            BOOST_AUTO_TEST_CASE(basic_or) {
                auto glob = tryCompileGlobC("(a|b)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                REJECTS_CSTR(mach, "c");
                ACCEPTS_CSTR(mach, "a");
                ACCEPTS_CSTR(mach, "b");
                REJECTS_CSTR(mach, "(a|b)");
            }

            BOOST_AUTO_TEST_CASE(multi_or) {
                auto glob = tryCompileGlobC("(a|b|c|d)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "a");
                ACCEPTS_CSTR(mach, "b");
                ACCEPTS_CSTR(mach, "c");
                ACCEPTS_CSTR(mach, "d");
                REJECTS_CSTR(mach, "ab");
                REJECTS_CSTR(mach, "abcd");
            }

            BOOST_AUTO_TEST_CASE(empty_or) {
                auto glob = tryCompileGlobC("(|a)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "a");
            }

            BOOST_AUTO_TEST_CASE(nested_wild) {
                auto glob = tryCompileGlobC("(*a*|b)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "a");
                ACCEPTS_CSTR(mach, "b");
                ACCEPTS_CSTR(mach, "ab");
                ACCEPTS_CSTR(mach, "bab");
                ACCEPTS_CSTR(mach, "there is \"a\"");
                REJECTS_CSTR(mach, "no first vowel");
            }

            BOOST_AUTO_TEST_CASE(or_prefix) {
                auto glob = tryCompileGlobC("(log|debug).txt");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "log.txt");
                ACCEPTS_CSTR(mach, "debug.txt");
                REJECTS_CSTR(mach, ".txt");
            }

            BOOST_AUTO_TEST_CASE(or_suffix) {
                auto glob = tryCompileGlobC("note.(pdf|docx)");
                BOOST_CHECK(glob.has_value());
                
                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "note.pdf");
                ACCEPTS_CSTR(mach, "note.docx");
                REJECTS_CSTR(mach, "note.");
            }

            BOOST_AUTO_TEST_CASE(interior_or) {
                auto glob = tryCompileGlobC("w(o|y)rm");
                BOOST_CHECK(glob.has_value());
                
                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "worm");
                ACCEPTS_CSTR(mach, "wyrm");
                REJECTS_CSTR(mach, "wrm");
                REJECTS_CSTR(mach, "woyrm");
                REJECTS_CSTR(mach, "wyorm");
                REJECTS_CSTR(mach, "orm");
            }

            BOOST_AUTO_TEST_CASE(wrapped_by_or) {
                auto glob = tryCompileGlobC("(glob|brex)_compiler.(h|cpp)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "glob_compiler.h");
                ACCEPTS_CSTR(mach, "glob_compiler.cpp");
                ACCEPTS_CSTR(mach, "brex_compiler.h");
                ACCEPTS_CSTR(mach, "brex_compiler.cpp");
                REJECTS_CSTR(mach, "blob_compiler.hpp");
            }

            BOOST_AUTO_TEST_CASE(nested_or) {
                auto glob = tryCompileGlobC("w(yvern|(o|y)rm)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "wyvern");
                ACCEPTS_CSTR(mach, "worm");
                ACCEPTS_CSTR(mach, "wyrm");
                REJECTS_CSTR(mach, "wyvernorm");
                REJECTS_CSTR(mach, "wyvernrm");
            }

            BOOST_AUTO_TEST_CASE(wild_flanked_or_left) {
                auto glob = tryCompileGlobC("*(a|b)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "a");
                ACCEPTS_CSTR(mach, "b");
                ACCEPTS_CSTR(mach, "123a");
                ACCEPTS_CSTR(mach, "123b");
                REJECTS_CSTR(mach, "123");
            }

            BOOST_AUTO_TEST_CASE(wild_flanked_or_right) {
                auto glob = tryCompileGlobC("(a|b)*");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "a");
                ACCEPTS_CSTR(mach, "b");
                ACCEPTS_CSTR(mach, "a123");
                ACCEPTS_CSTR(mach, "b123");
                REJECTS_CSTR(mach, "123");
            }
        BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_SUITE(Unicode) 
            BOOST_AUTO_TEST_CASE(basic_or) {
                auto glob = tryCompileGlobUnicode(u8"(a|b)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                REJECTS_UNICODE(mach, u8"c");
                ACCEPTS_UNICODE(mach, u8"a");
                ACCEPTS_UNICODE(mach, u8"b");
                REJECTS_UNICODE(mach, u8"(a|b)");
            }

            BOOST_AUTO_TEST_CASE(multi_or) {
                auto glob = tryCompileGlobUnicode(u8"(a|b|c|d)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                ACCEPTS_UNICODE(mach, u8"a");
                ACCEPTS_UNICODE(mach, u8"b");
                ACCEPTS_UNICODE(mach, u8"c");
                ACCEPTS_UNICODE(mach, u8"d");
                REJECTS_UNICODE(mach, u8"ab");
                REJECTS_UNICODE(mach, u8"abcd");
            }

            BOOST_AUTO_TEST_CASE(empty_or) {
                auto glob = tryCompileGlobUnicode(u8"(|a)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"");
                ACCEPTS_UNICODE(mach, u8"a");
            }

            BOOST_AUTO_TEST_CASE(nested_wild) {
                auto glob = tryCompileGlobUnicode(u8"(*a*|b)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"a");
                ACCEPTS_UNICODE(mach, u8"b");
                ACCEPTS_UNICODE(mach, u8"ab");
                ACCEPTS_UNICODE(mach, u8"bab");
                ACCEPTS_UNICODE(mach, u8"there is \"a\"");
                REJECTS_UNICODE(mach, u8"no first vowel");
            }

            BOOST_AUTO_TEST_CASE(or_prefix) {
                auto glob = tryCompileGlobUnicode(u8"(log|debug).txt");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"log.txt");
                ACCEPTS_UNICODE(mach, u8"debug.txt");
                REJECTS_UNICODE(mach, u8".txt");
            }

            BOOST_AUTO_TEST_CASE(or_suffix) {
                auto glob = tryCompileGlobUnicode(u8"note.(pdf|docx)");
                BOOST_CHECK(glob.has_value());
                
                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"note.pdf");
                ACCEPTS_UNICODE(mach, u8"note.docx");
                REJECTS_UNICODE(mach, u8"note.");
            }

            BOOST_AUTO_TEST_CASE(interior_or) {
                auto glob = tryCompileGlobUnicode(u8"w(o|y)rm");
                BOOST_CHECK(glob.has_value());
                
                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"worm");
                ACCEPTS_UNICODE(mach, u8"wyrm");
                REJECTS_UNICODE(mach, u8"wrm");
                REJECTS_UNICODE(mach, u8"woyrm");
                REJECTS_UNICODE(mach, u8"wyorm");
                REJECTS_UNICODE(mach, u8"orm");
            }

            BOOST_AUTO_TEST_CASE(wrapped_by_or) {
                auto glob = tryCompileGlobUnicode(u8"(glob|brex)_compiler.(h|cpp)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"glob_compiler.h");
                ACCEPTS_UNICODE(mach, u8"glob_compiler.cpp");
                ACCEPTS_UNICODE(mach, u8"brex_compiler.h");
                ACCEPTS_UNICODE(mach, u8"brex_compiler.cpp");
                REJECTS_UNICODE(mach, u8"blob_compiler.hpp");
            }

            BOOST_AUTO_TEST_CASE(nested_or) {
                auto glob = tryCompileGlobUnicode(u8"w(yvern|(o|y)rm)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"wyvern");
                ACCEPTS_UNICODE(mach, u8"worm");
                ACCEPTS_UNICODE(mach, u8"wyrm");
                REJECTS_UNICODE(mach, u8"wyvernorm");
                REJECTS_UNICODE(mach, u8"wyvernrm");
            }

            BOOST_AUTO_TEST_CASE(wild_flanked_or_left) {
                auto glob = tryCompileGlobUnicode(u8"*(a|b)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                ACCEPTS_UNICODE(mach, u8"a");
                ACCEPTS_UNICODE(mach, u8"b");
                ACCEPTS_UNICODE(mach, u8"123a");
                ACCEPTS_UNICODE(mach, u8"123b");
                REJECTS_UNICODE(mach, u8"123");
            }

            BOOST_AUTO_TEST_CASE(wild_flanked_or_right) {
                auto glob = tryCompileGlobUnicode(u8"(a|b)*");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                ACCEPTS_UNICODE(mach, u8"a");
                ACCEPTS_UNICODE(mach, u8"b");
                ACCEPTS_UNICODE(mach, u8"a123");
                ACCEPTS_UNICODE(mach, u8"b123");
                REJECTS_UNICODE(mach, u8"123");
            }
        BOOST_AUTO_TEST_SUITE_END()
    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE(Substitution)
        BOOST_AUTO_TEST_SUITE(CString)
            BOOST_AUTO_TEST_CASE(substitution) {
                auto segment = tryCompileGlobExpressionC("abc");
                BOOST_CHECK(segment.has_value());

                auto glob = tryCompileGlobC("${segment}");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());
                
                REJECTS_CSTR(mach, "${segment}");
                REJECTS_CSTR(mach, "segment");
                ACCEPTS_CSTR(mach, "abc");
            }

            BOOST_AUTO_TEST_CASE(in_middle) {
                auto segment = tryCompileGlobExpressionC("banana");
                BOOST_CHECK(segment.has_value());

                auto glob = tryCompileGlobC("apple${segment}coconut");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());

                REJECTS_CSTR(mach, "applecoconut");
                REJECTS_CSTR(mach, "apple${segment}coconut");
                ACCEPTS_CSTR(mach, "applebananacoconut");
            }

            BOOST_AUTO_TEST_CASE(with_wild) {
                auto segment = tryCompileGlobExpressionC("*");
                BOOST_CHECK(segment.has_value());

                auto glob = tryCompileGlobC("apple${segment}coconut");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());

                ACCEPTS_CSTR(mach, "applecoconut");
                ACCEPTS_CSTR(mach, "applebananacoconut");
                ACCEPTS_CSTR(mach, "apple${segment}coconut");
            }

            BOOST_AUTO_TEST_CASE(with_union) {
                auto segment = tryCompileGlobExpressionC("(b|q)");
                BOOST_CHECK(segment.has_value());
                auto glob = tryCompileGlobC("a${segment}c");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());

                ACCEPTS_CSTR(mach, "abc");
                ACCEPTS_CSTR(mach, "aqc");
                REJECTS_CSTR(mach, "ac");
                REJECTS_CSTR(mach, "a(b|q)c");
            }

            BOOST_AUTO_TEST_CASE(nested) {
                auto inner_segment = tryCompileGlobExpressionC("worm");
                BOOST_CHECK(inner_segment.has_value());
                auto segment = tryCompileGlobExpressionC("${inner_segment}");
                BOOST_CHECK(segment.has_value());

                segment.value()->link(u8"inner_segment", inner_segment.value());

                auto glob = tryCompileGlobC("a${segment}c");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());

                ACCEPTS_CSTR(mach, "awormc");
                REJECTS_CSTR(mach, "ac");
                REJECTS_CSTR(mach, "a${inner_segment}c");
            }

        BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_SUITE(Unicode)
            BOOST_AUTO_TEST_CASE(substitution) {
                auto segment = tryCompileGlobExpressionUnicode(u8"abc");
                BOOST_CHECK(segment.has_value());

                auto glob = tryCompileGlobUnicode(u8"${segment}");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());
                
                REJECTS_UNICODE(mach, u8"${segment}");
                REJECTS_UNICODE(mach, u8"segment");
                ACCEPTS_UNICODE(mach, u8"abc");
            }

            BOOST_AUTO_TEST_CASE(in_middle) {
                auto segment = tryCompileGlobExpressionUnicode(u8"banana");
                BOOST_CHECK(segment.has_value());

                auto glob = tryCompileGlobUnicode(u8"apple${segment}coconut");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());

                REJECTS_UNICODE(mach, u8"applecoconut");
                REJECTS_UNICODE(mach, u8"apple${segment}coconut");
                ACCEPTS_UNICODE(mach, u8"applebananacoconut");
            }

            BOOST_AUTO_TEST_CASE(with_wild) {
                auto segment = tryCompileGlobExpressionUnicode(u8"*");
                BOOST_CHECK(segment.has_value());

                auto glob = tryCompileGlobUnicode(u8"apple${segment}coconut");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());

                ACCEPTS_UNICODE(mach, u8"applecoconut");
                ACCEPTS_UNICODE(mach, u8"applebananacoconut");
                ACCEPTS_UNICODE(mach, u8"apple${segment}coconut");
            }

            BOOST_AUTO_TEST_CASE(with_union) {
                auto segment = tryCompileGlobExpressionUnicode(u8"(b|q)");
                BOOST_CHECK(segment.has_value());
                auto glob = tryCompileGlobUnicode(u8"a${segment}c");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());

                ACCEPTS_UNICODE(mach, u8"abc");
                ACCEPTS_UNICODE(mach, u8"aqc");
                REJECTS_UNICODE(mach, u8"ac");
                REJECTS_UNICODE(mach, u8"a(b|q)c");
            }

            BOOST_AUTO_TEST_CASE(nested) {
                auto inner_segment = tryCompileGlobExpressionUnicode(u8"🪱");
                BOOST_CHECK(inner_segment.has_value());
                auto segment = tryCompileGlobExpressionUnicode(u8"${inner_segment}");
                BOOST_CHECK(segment.has_value());

                segment.value()->link(u8"inner_segment", inner_segment.value());

                auto glob = tryCompileGlobUnicode(u8"a${segment}c");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                mach->link(u8"segment", segment.value());

                ACCEPTS_UNICODE(mach, u8"a🪱c");
                REJECTS_UNICODE(mach, u8"ac");
                REJECTS_UNICODE(mach, u8"a${inner_segment}c");
            }

        BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE(Paths)
        BOOST_AUTO_TEST_SUITE(CString)
            BOOST_AUTO_TEST_CASE(basic_path) {
                auto glob = tryCompileGlobC("a/b/c/d");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                REJECTS_CSTR(mach, "a");
                REJECTS_CSTR(mach, "a/b");
                REJECTS_CSTR(mach, "a/b/c");
                ACCEPTS_CSTR(mach, "a/b/c/d");
                REJECTS_CSTR(mach, "abcd");
                REJECTS_CSTR(mach, "a/b/cd");
                REJECTS_CSTR(mach, "a/c/b/d");
            }

            BOOST_AUTO_TEST_CASE(recursive_wildcard) {
                auto glob = tryCompileGlobC("**");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "a");
                ACCEPTS_CSTR(mach, "I can put whatever I want");
                ACCEPTS_CSTR(mach, "I/can/even/put/it/in/a/directory/structure");
            }

            BOOST_AUTO_TEST_CASE(interior_recursive_wildcard) {
                auto glob = tryCompileGlobC("a/b/**/c/d");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_CSTR(mach, "");
                ACCEPTS_CSTR(mach, "a/b/c/d");
                ACCEPTS_CSTR(mach, "a/b/e/e/e/c/d");
                ACCEPTS_CSTR(mach, "a/b/c/d/c/d");
            }

            BOOST_AUTO_TEST_CASE(comprehensive_path) {
                auto glob = tryCompileGlobC("path/to/(some|your)/file(|s)/**/(report|notes)_*.(txt|md|pdf|docx)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "path/to/some/file/called/report_1234.md");
                ACCEPTS_CSTR(mach, "path/to/your/files/notes_january26.pdf");
                ACCEPTS_CSTR(mach, "path/to/your/file/report_.docx")
                REJECTS_CSTR(mach, "path/to/your/file/report_2.docx/but/wait/there's/more")
                REJECTS_CSTR(mach, "path/to/a/diffent/file/that/I/didn't/give/you/permissions/for/notes_personal.docx");
                ACCEPTS_CSTR(mach, "path/to/some/files/report_2/report_2.docx");
                ACCEPTS_CSTR(mach, "path/to/some/files/report_2/report_2.pdf");
            }

        BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_SUITE(Unicode)
            BOOST_AUTO_TEST_CASE(basic_path) {
                auto glob = tryCompileGlobUnicode(u8"a/b/c/d");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                REJECTS_UNICODE(mach, u8"a");
                REJECTS_UNICODE(mach, u8"a/b");
                REJECTS_UNICODE(mach, u8"a/b/c");
                ACCEPTS_UNICODE(mach, u8"a/b/c/d");
                REJECTS_UNICODE(mach, u8"abcd");
                REJECTS_UNICODE(mach, u8"a/b/cd");
                REJECTS_UNICODE(mach, u8"a/c/b/d");
            }

            BOOST_AUTO_TEST_CASE(recursive_wildcard) {
                auto glob = tryCompileGlobUnicode(u8"**");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"");
                ACCEPTS_UNICODE(mach, u8"a");
                ACCEPTS_UNICODE(mach, u8"I can put whatever I want");
                ACCEPTS_UNICODE(mach, u8"I/can/even/put/it/in/a/directory/structure");
            }

            BOOST_AUTO_TEST_CASE(interior_recursive_wildcard) {
                auto glob = tryCompileGlobUnicode(u8"a/b/**/c/d");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                REJECTS_UNICODE(mach, u8"");
                ACCEPTS_UNICODE(mach, u8"a/b/c/d");
                ACCEPTS_UNICODE(mach, u8"a/b/e/e/e/c/d");
                ACCEPTS_UNICODE(mach, u8"a/b/c/d/c/d");
            }

            BOOST_AUTO_TEST_CASE(comprehensive_path) {
                auto glob = tryCompileGlobUnicode(u8"path/to/(some|your)/file(|s)/**/(report|notes)_*.(txt|md|pdf|docx)");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_UNICODE(mach, u8"path/to/some/file/called/report_1234.md");
                ACCEPTS_UNICODE(mach, u8"path/to/your/files/notes_january26.pdf");
                ACCEPTS_UNICODE(mach, u8"path/to/your/file/report_.docx")
                REJECTS_UNICODE(mach, u8"path/to/your/file/report_.docx/but/wait/there's/more")
                REJECTS_UNICODE(mach, u8"path/to/a/diffent/file/that/I/didn't/give/you/permissions/for/notes_personal.docx");
                ACCEPTS_UNICODE(mach, u8"path/to/some/files/report_2/report_2.docx");
                ACCEPTS_UNICODE(mach, u8"path/to/some/files/report_2/report_2.pdf");
            }
        BOOST_AUTO_TEST_SUITE_END()
    BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()