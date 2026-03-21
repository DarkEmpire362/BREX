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

std::optional<brex::FragmentMachine*> tryCompileGlobUnicode(const std::u8string& globstr) {
    auto ast = brex::GlobParser::parseGlobUnicodeString(globstr);
    // TODO: Check Errs
    auto machine = brex::GlobCompiler::compile(ast);
    // TODO: Check Errs
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
                ACCEPTS_UNICODE(mach, u8"%_a");
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

            BOOST_AUTO_TEST_CASE(doublewild) {
                auto glob = tryCompileGlobC("*a*");
                BOOST_CHECK(glob.has_value());

                auto mach = glob.value();
                ACCEPTS_CSTR(mach, "a");
                REJECTS_CSTR(mach, "b");
            }
        BOOST_AUTO_TEST_SUITE_END()
    BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()