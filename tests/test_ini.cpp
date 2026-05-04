#include <gtest/gtest.h>
#include <polycpp/ini.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace polycpp::ini;

// ===========================================================================
// Fixture data — loaded byte-exactly from upstream test/fixtures/
// (copies of npm/ini@v6.0.0 test/fixtures/foo.ini and duplicate.ini live in
// tests/fixtures/; INI_TESTS_FIXTURES_DIR is set by CMake.)
// ===========================================================================

namespace {

std::string slurpFixture(const char* name) {
    std::string path = std::string(INI_TESTS_FIXTURES_DIR) + "/" + name;
    std::ifstream f(path);
    if (!f) {
        ADD_FAILURE() << "missing fixture: " << path;
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

const std::string& fooIni() {
    static const std::string data = slurpFixture("foo.ini");
    return data;
}

const std::string& duplicateIni() {
    static const std::string data = slurpFixture("duplicate.ini");
    return data;
}

}  // namespace

#define FOO_INI (fooIni().c_str())
#define DUPLICATE_INI (duplicateIni().c_str())

// ===========================================================================
// 1. Safe / Unsafe Tests
// ===========================================================================

TEST(IniTest, SafeEscapesSpecialChars) {
    // ; and # are escaped with backslash
    EXPECT_EQ(safe("hello;world"), "hello\\;world");
    EXPECT_EQ(safe("hello#world"), "hello\\#world");
    EXPECT_EQ(safe("a;b#c"), "a\\;b\\#c");
}

TEST(IniTest, SafeJsonStringifiesWhenNeeded) {
    // Contains =
    EXPECT_EQ(safe("a=b"), "\"a=b\"");
    // Contains \r
    EXPECT_EQ(safe("a\rb"), "\"a\\rb\"");
    // Contains \n
    EXPECT_EQ(safe("a\nb"), "\"a\\nb\"");
    // Starts with [
    EXPECT_EQ(safe("[section]"), "\"[section]\"");
    // Quoted string (double)
    EXPECT_EQ(safe("\"hello\""), "\"\\\"hello\\\"\"");
    // Quoted string (single)
    EXPECT_EQ(safe("'hello'"), "\"'hello'\"");
    // Leading whitespace
    EXPECT_EQ(safe(" x"), "\" x\"");
    // Trailing whitespace
    EXPECT_EQ(safe("x "), "\"x \"");
}

TEST(IniTest, SafePassesThroughPlainStrings) {
    EXPECT_EQ(safe("hello"), "hello");
    EXPECT_EQ(safe(""), "");
    EXPECT_EQ(safe("simple value"), "simple value");
}

TEST(IniTest, SafeBoolAndNullValues) {
    // safe() takes a string, but for encode purposes we test the
    // value-to-string conversion
    EXPECT_EQ(safe("true"), "true");
    EXPECT_EQ(safe("false"), "false");
    EXPECT_EQ(safe("null"), "null");
}

TEST(IniTest, UnsafeTrimsInput) {
    EXPECT_EQ(unsafe("  hello  "), "hello");
    EXPECT_EQ(unsafe("\thello\t"), "hello");
}

TEST(IniTest, UnsafeHandlesEmptyString) {
    EXPECT_EQ(unsafe(""), "");
    EXPECT_EQ(unsafe("   "), "");
}

TEST(IniTest, UnsafeStripsInlineComments) {
    EXPECT_EQ(unsafe("x;y"), "x");
    EXPECT_EQ(unsafe("x  # y"), "x");
    EXPECT_EQ(unsafe("x # y ; z"), "x");
}

TEST(IniTest, UnsafeHandlesEscapedComments) {
    EXPECT_EQ(unsafe("x\\;y"), "x;y");
    EXPECT_EQ(unsafe("x\\#y"), "x#y");
    EXPECT_EQ(unsafe("this\\; this is not a comment"), "this; this is not a comment");
    EXPECT_EQ(unsafe("this\\# this is not a comment"), "this# this is not a comment");
}

TEST(IniTest, UnsafeHandlesDoubleQuoted) {
    // JSON-parse double-quoted strings
    EXPECT_EQ(unsafe("\"hello world\""), "hello world");
    EXPECT_EQ(unsafe("\"eq=eq\""), "eq=eq");
    EXPECT_EQ(unsafe("\"a\\nb\""), "a\nb");
}

TEST(IniTest, UnsafeHandlesSingleQuoted) {
    // Single-quoted: just strip quotes
    EXPECT_EQ(unsafe("'something'"), "something");
    EXPECT_EQ(unsafe("'   '"), "   ");
    EXPECT_EQ(unsafe("' a '"), " a ");
}

TEST(IniTest, UnsafeTrailingBackslash) {
    // x "\ → the backslash at end is kept
    EXPECT_EQ(unsafe("x \"\\"), "x \"\\");
}

TEST(IniTest, UnsafeHandlesEscapedBackslash) {
    EXPECT_EQ(unsafe("x\\\\y"), "x\\y");
}

// ===========================================================================
// 2. Parse Tests
// ===========================================================================

TEST(IniTest, ParseSimpleKeyValue) {
    auto doc = parse("key=value\n");
    auto* v = find(doc, "key");
    ASSERT_NE(v, nullptr);
    ASSERT_TRUE(v->isString());
    EXPECT_EQ(v->asString(), "value");
}

TEST(IniTest, ParseSectionHeaders) {
    auto doc = parse("[section]\nkey=value\n");
    auto* sec = find(doc, "section");
    ASSERT_NE(sec, nullptr);
    ASSERT_TRUE(sec->isDocument());
    auto* v = find(sec->asDocument(), "key");
    ASSERT_NE(v, nullptr);
    ASSERT_TRUE(v->isString());
    EXPECT_EQ(v->asString(), "value");
}

TEST(IniTest, ParseNestedSections) {
    auto doc = parse("[a.b.c]\ne=1\nj=2\n");
    // Should create nested a → b → c
    auto* a = find(doc, "a");
    ASSERT_NE(a, nullptr);
    ASSERT_TRUE(a->isDocument());
    auto* b = find(a->asDocument(), "b");
    ASSERT_NE(b, nullptr);
    ASSERT_TRUE(b->isDocument());
    auto* c = find(b->asDocument(), "c");
    ASSERT_NE(c, nullptr);
    ASSERT_TRUE(c->isDocument());
    auto* e = find(c->asDocument(), "e");
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->asString(), "1");
}

TEST(IniTest, ParseEscapedDotSections) {
    auto doc = parse("[x\\.y\\.z]\nx.y.z=xyz\n");
    // x\.y\.z is a literal key "x.y.z"
    auto* sec = find(doc, "x.y.z");
    ASSERT_NE(sec, nullptr);
    ASSERT_TRUE(sec->isDocument());
    auto* v = find(sec->asDocument(), "x.y.z");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->asString(), "xyz");
}

TEST(IniTest, ParseSpecialValues) {
    auto doc = parse("t=true\nf=false\nn=null\n");
    auto* t = find(doc, "t");
    ASSERT_NE(t, nullptr);
    ASSERT_TRUE(t->isBool());
    EXPECT_TRUE(t->asBool());

    auto* f = find(doc, "f");
    ASSERT_NE(f, nullptr);
    ASSERT_TRUE(f->isBool());
    EXPECT_FALSE(f->asBool());

    auto* n = find(doc, "n");
    ASSERT_NE(n, nullptr);
    EXPECT_TRUE(n->isNull());
}

TEST(IniTest, ParseBareKey) {
    auto doc = parse("s7\n");
    auto* v = find(doc, "s7");
    ASSERT_NE(v, nullptr);
    ASSERT_TRUE(v->isBool());
    EXPECT_TRUE(v->asBool());
}

TEST(IniTest, ParseEmptyValue) {
    auto doc = parse("s3=\n");
    auto* v = find(doc, "s3");
    ASSERT_NE(v, nullptr);
    ASSERT_TRUE(v->isString());
    EXPECT_EQ(v->asString(), "");
}

TEST(IniTest, ParseValueWithSpaces) {
    auto doc = parse("   a with spaces   =     b  c\n");
    auto* v = find(doc, "a with spaces");
    ASSERT_NE(v, nullptr);
    ASSERT_TRUE(v->isString());
    EXPECT_EQ(v->asString(), "b  c");
}

TEST(IniTest, ParseComments) {
    auto doc = parse("; this is a comment\n# this too\nkey=value\n");
    EXPECT_FALSE(hasKey(doc, "; this is a comment"));
    EXPECT_FALSE(hasKey(doc, "# this too"));
    EXPECT_TRUE(hasKey(doc, "key"));
}

TEST(IniTest, ParseEscapedComments) {
    auto doc = parse("nocomment = this\\; this is not a comment\n");
    auto* v = find(doc, "nocomment");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->asString(), "this; this is not a comment");
}

TEST(IniTest, ParseBracketedArrays) {
    auto doc = parse("ar[]=one\nar[]=three\n");
    auto* v = find(doc, "ar");
    ASSERT_NE(v, nullptr);
    ASSERT_TRUE(v->isArray());
    EXPECT_EQ(v->asArray().size(), 2u);
    EXPECT_EQ(v->asArray()[0].asString(), "one");
    EXPECT_EQ(v->asArray()[1].asString(), "three");
}

TEST(IniTest, ParseArrayPromotionFromExisting) {
    // ar[] creates array, then plain ar appends to existing array
    auto doc = parse("ar[]=one\nar[]=three\nar=this is included\n");
    auto* v = find(doc, "ar");
    ASSERT_NE(v, nullptr);
    ASSERT_TRUE(v->isArray());
    EXPECT_EQ(v->asArray().size(), 3u);
    EXPECT_EQ(v->asArray()[0].asString(), "one");
    EXPECT_EQ(v->asArray()[1].asString(), "three");
    EXPECT_EQ(v->asArray()[2].asString(), "this is included");
}

TEST(IniTest, ParseProtoProtection) {
    std::string data =
        "__proto__ = quux\n"
        "constructor.prototype.foo = asdfasdf\n"
        "foo = baz\n"
        "[__proto__]\n"
        "foo = bar\n"
        "[other]\n"
        "foo = asdf\n"
        "[kid.__proto__.foo]\n"
        "foo = kid\n"
        "[arrproto]\n"
        "hello = snyk\n"
        "__proto__[] = you did a good job\n"
        "__proto__[] = so you deserve arrays\n"
        "thanks = true\n"
        "[ctor.constructor.prototype]\n"
        "foo = asdfasdf\n";

    auto doc = parse(data);

    // __proto__ top-level key should be skipped
    EXPECT_FALSE(hasKey(doc, "__proto__"));

    // constructor.prototype.foo should be present as a plain key
    auto* cpf = find(doc, "constructor.prototype.foo");
    ASSERT_NE(cpf, nullptr);
    EXPECT_EQ(cpf->asString(), "asdfasdf");

    // foo should be present
    auto* foo = find(doc, "foo");
    ASSERT_NE(foo, nullptr);
    EXPECT_EQ(foo->asString(), "baz");

    // other section
    auto* other = find(doc, "other");
    ASSERT_NE(other, nullptr);
    ASSERT_TRUE(other->isDocument());
    EXPECT_EQ(find(other->asDocument(), "foo")->asString(), "asdf");

    // kid.__proto__.foo → nested kid → (skip __proto__) → foo
    auto* kid = find(doc, "kid");
    ASSERT_NE(kid, nullptr);
    ASSERT_TRUE(kid->isDocument());
    auto* kidFoo = find(kid->asDocument(), "foo");
    ASSERT_NE(kidFoo, nullptr);
    ASSERT_TRUE(kidFoo->isDocument());
    EXPECT_EQ(find(kidFoo->asDocument(), "foo")->asString(), "kid");

    // arrproto: __proto__[] should be skipped
    auto* arrproto = find(doc, "arrproto");
    ASSERT_NE(arrproto, nullptr);
    ASSERT_TRUE(arrproto->isDocument());
    EXPECT_FALSE(hasKey(arrproto->asDocument(), "__proto__"));
    EXPECT_EQ(find(arrproto->asDocument(), "hello")->asString(), "snyk");
    auto* thanks = find(arrproto->asDocument(), "thanks");
    ASSERT_NE(thanks, nullptr);
    ASSERT_TRUE(thanks->isBool());
    EXPECT_TRUE(thanks->asBool());

    // ctor.constructor.prototype → nested
    auto* ctor = find(doc, "ctor");
    ASSERT_NE(ctor, nullptr);
    ASSERT_TRUE(ctor->isDocument());
    auto* constructor = find(ctor->asDocument(), "constructor");
    ASSERT_NE(constructor, nullptr);
    ASSERT_TRUE(constructor->isDocument());
    auto* prototype = find(constructor->asDocument(), "prototype");
    ASSERT_NE(prototype, nullptr);
    ASSERT_TRUE(prototype->isDocument());
    EXPECT_EQ(find(prototype->asDocument(), "foo")->asString(), "asdfasdf");
}

TEST(IniTest, ParseQuotedValues) {
    auto doc = parse("s = 'something'\ns2 = \"something else\"\n");
    auto* s = find(doc, "s");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->asString(), "something");

    auto* s2 = find(doc, "s2");
    ASSERT_NE(s2, nullptr);
    EXPECT_EQ(s2->asString(), "something else");
}

TEST(IniTest, ParseJunkLine) {
    // =just junk! should be skipped (regex won't match: key part is empty)
    auto doc = parse("=just junk!\n[foo]\nbar\n");
    EXPECT_FALSE(hasKey(doc, ""));
    auto* foo = find(doc, "foo");
    ASSERT_NE(foo, nullptr);
    ASSERT_TRUE(foo->isDocument());
    auto* bar = find(foo->asDocument(), "bar");
    ASSERT_NE(bar, nullptr);
    ASSERT_TRUE(bar->isBool());
    EXPECT_TRUE(bar->asBool());
}

TEST(IniTest, ParseCRLF) {
    auto doc = parse("key1=val1\r\nkey2=val2\r\n");
    EXPECT_EQ(find(doc, "key1")->asString(), "val1");
    EXPECT_EQ(find(doc, "key2")->asString(), "val2");
}

// ===========================================================================
// 3. Encode Tests
// ===========================================================================

TEST(IniTest, EncodeSimple) {
    IniDocument doc;
    set(doc, "key", IniValue("value"));
    std::string result = encode(doc);
    EXPECT_EQ(result, "key=value\n");
}

TEST(IniTest, EncodeWithSection) {
    // obj = { log: { type: 'file', level: { label: 'debug', value: 10 } } }
    IniDocument level;
    set(level, "label", IniValue("debug"));
    set(level, "value", IniValue("10"));
    IniDocument log;
    set(log, "type", IniValue("file"));
    set(log, "level", IniValue(std::move(level)));
    IniDocument doc;
    set(doc, "log", IniValue(std::move(log)));

    std::string result = encode(doc, EncodeOptions{.section = "prefix"});
    std::string expected =
        "[prefix.log]\n"
        "type=file\n"
        "\n"
        "[prefix.log.level]\n"
        "label=debug\n"
        "value=10\n";
    EXPECT_EQ(result, expected);
}

TEST(IniTest, EncodeWithWhitespace) {
    IniDocument level;
    set(level, "label", IniValue("debug"));
    set(level, "value", IniValue("10"));
    IniDocument log;
    set(log, "type", IniValue("file"));
    set(log, "level", IniValue(std::move(level)));
    IniDocument doc;
    set(doc, "log", IniValue(std::move(log)));

    std::string result = encode(doc, EncodeOptions{.whitespace = true});
    std::string expected =
        "[log]\n"
        "type = file\n"
        "\n"
        "[log.level]\n"
        "label = debug\n"
        "value = 10\n";
    EXPECT_EQ(result, expected);
}

TEST(IniTest, EncodeWithNewline) {
    IniDocument level;
    set(level, "label", IniValue("debug"));
    set(level, "value", IniValue("10"));
    IniDocument log;
    set(log, "type", IniValue("file"));
    set(log, "level", IniValue(std::move(level)));
    IniDocument doc;
    set(doc, "log", IniValue(std::move(log)));

    std::string result = encode(doc, EncodeOptions{.newline = true});
    std::string expected =
        "[log]\n"
        "\n"
        "type=file\n"
        "\n"
        "[log.level]\n"
        "\n"
        "label=debug\n"
        "value=10\n";
    EXPECT_EQ(result, expected);
}

TEST(IniTest, EncodeWithPlatformWin32) {
    IniDocument level;
    set(level, "label", IniValue("debug"));
    set(level, "value", IniValue("10"));
    IniDocument log;
    set(log, "type", IniValue("file"));
    set(log, "level", IniValue(std::move(level)));
    IniDocument doc;
    set(doc, "log", IniValue(std::move(log)));

    std::string result = encode(doc, EncodeOptions{.platform = "win32"});
    std::string expected =
        "[log]\r\n"
        "type=file\r\n"
        "\r\n"
        "[log.level]\r\n"
        "label=debug\r\n"
        "value=10\r\n";
    EXPECT_EQ(result, expected);
}

TEST(IniTest, EncodeArraysBracketed) {
    IniDocument doc;
    IniValue::ArrayType arr;
    arr.push_back(IniValue("1"));
    arr.push_back(IniValue("2"));
    arr.push_back(IniValue("3"));
    set(doc, "ar", IniValue(std::move(arr)));
    IniValue::ArrayType brr;
    brr.push_back(IniValue("1"));
    brr.push_back(IniValue("2"));
    set(doc, "br", IniValue(std::move(brr)));

    std::string result = encode(doc);
    std::string expected =
        "ar[]=1\n"
        "ar[]=2\n"
        "ar[]=3\n"
        "br[]=1\n"
        "br[]=2\n";
    EXPECT_EQ(result, expected);
}

TEST(IniTest, EncodeArraysUnbracketed) {
    IniDocument doc;
    IniValue::ArrayType arr;
    arr.push_back(IniValue("1"));
    arr.push_back(IniValue("2"));
    arr.push_back(IniValue("3"));
    set(doc, "ar", IniValue(std::move(arr)));
    IniValue::ArrayType brr;
    brr.push_back(IniValue("1"));
    brr.push_back(IniValue("2"));
    set(doc, "br", IniValue(std::move(brr)));

    std::string result = encode(doc, EncodeOptions{.bracketedArray = false});
    std::string expected =
        "ar=1\n"
        "ar=2\n"
        "ar=3\n"
        "br=1\n"
        "br=2\n";
    EXPECT_EQ(result, expected);
}

TEST(IniTest, EncodeNeverBlankFirstOrLastLine) {
    IniDocument level;
    set(level, "label", IniValue("debug"));
    set(level, "value", IniValue("10"));
    IniDocument log;
    set(log, "type", IniValue("file"));
    set(log, "level", IniValue(std::move(level)));
    IniDocument doc;
    set(doc, "log", IniValue(std::move(log)));

    std::string result = encode(doc);
    EXPECT_NE(result.substr(0, 1), "\n");
    EXPECT_FALSE(result.size() >= 2 &&
                 result[result.size() - 1] == '\n' &&
                 result[result.size() - 2] == '\n');
}

// ===========================================================================
// 4. Round-Trip Tests
// ===========================================================================

TEST(IniTest, RoundTripNumber) {
    IniDocument doc;
    set(doc, "count", IniValue("10"));
    auto result = parse(stringify(doc));
    ASSERT_TRUE(hasKey(result, "count"));
    EXPECT_EQ(find(result, "count")->asString(), "10");
}

TEST(IniTest, RoundTripString) {
    IniDocument doc;
    set(doc, "drink", IniValue("white russian"));
    auto result = parse(stringify(doc));
    ASSERT_TRUE(hasKey(result, "drink"));
    EXPECT_EQ(find(result, "drink")->asString(), "white russian");
}

TEST(IniTest, RoundTripBoolean) {
    IniDocument doc;
    set(doc, "isTrue", IniValue(true));
    auto result = parse(stringify(doc));
    ASSERT_TRUE(hasKey(result, "isTrue"));
    EXPECT_TRUE(find(result, "isTrue")->isBool());
    EXPECT_TRUE(find(result, "isTrue")->asBool());
}

TEST(IniTest, RoundTripNested) {
    IniDocument inner;
    set(inner, "abides", IniValue(true));
    set(inner, "rugCount", IniValue("1"));
    IniDocument doc;
    set(doc, "theDude", IniValue(std::move(inner)));

    auto result = parse(stringify(doc));
    auto* td = find(result, "theDude");
    ASSERT_NE(td, nullptr);
    ASSERT_TRUE(td->isDocument());
    auto* abides = find(td->asDocument(), "abides");
    ASSERT_NE(abides, nullptr);
    EXPECT_TRUE(abides->isBool());
    EXPECT_TRUE(abides->asBool());
    auto* rc = find(td->asDocument(), "rugCount");
    ASSERT_NE(rc, nullptr);
    EXPECT_EQ(rc->asString(), "1");
}

// ===========================================================================
// 5. Full Fixture Tests (foo.ini)
// ===========================================================================

TEST(IniTest, FooIniDecodeFromFile) {
    auto doc = parse(FOO_INI);

    // o → "p"
    ASSERT_TRUE(hasKey(doc, "o"));
    EXPECT_EQ(find(doc, "o")->asString(), "p");

    // a with spaces → "b  c"
    ASSERT_TRUE(hasKey(doc, "a with spaces"));
    EXPECT_EQ(find(doc, "a with spaces")->asString(), "b  c");

    // " xa  n          p " → "\r\nyoyoyo\r\r\n"
    ASSERT_TRUE(hasKey(doc, " xa  n          p "));
    {
        std::string expected = "\"\r\nyoyoyo\r\r\n";
        EXPECT_EQ(find(doc, " xa  n          p ")->asString(), expected);
    }

    // [disturbing] → "hey you never know"
    ASSERT_TRUE(hasKey(doc, "[disturbing]"));
    EXPECT_EQ(find(doc, "[disturbing]")->asString(), "hey you never know");

    // s → "something"
    ASSERT_TRUE(hasKey(doc, "s"));
    EXPECT_EQ(find(doc, "s")->asString(), "something");

    // s1 → "\"something'" (literal double-quote + something')
    ASSERT_TRUE(hasKey(doc, "s1"));
    EXPECT_EQ(find(doc, "s1")->asString(), "\"something'");

    // s2 → "something else"
    ASSERT_TRUE(hasKey(doc, "s2"));
    EXPECT_EQ(find(doc, "s2")->asString(), "something else");

    // s3 → ""
    ASSERT_TRUE(hasKey(doc, "s3"));
    EXPECT_EQ(find(doc, "s3")->asString(), "");

    // s4 → ""
    ASSERT_TRUE(hasKey(doc, "s4"));
    EXPECT_EQ(find(doc, "s4")->asString(), "");

    // s5 → "   " (three spaces)
    ASSERT_TRUE(hasKey(doc, "s5"));
    EXPECT_EQ(find(doc, "s5")->asString(), "   ");

    // s6 → " a "
    ASSERT_TRUE(hasKey(doc, "s6"));
    EXPECT_EQ(find(doc, "s6")->asString(), " a ");

    // s7 → true
    ASSERT_TRUE(hasKey(doc, "s7"));
    ASSERT_TRUE(find(doc, "s7")->isBool());
    EXPECT_TRUE(find(doc, "s7")->asBool());

    // true → true
    ASSERT_TRUE(hasKey(doc, "true"));
    ASSERT_TRUE(find(doc, "true")->isBool());
    EXPECT_TRUE(find(doc, "true")->asBool());

    // false → false
    ASSERT_TRUE(hasKey(doc, "false"));
    ASSERT_TRUE(find(doc, "false")->isBool());
    EXPECT_FALSE(find(doc, "false")->asBool());

    // null → null
    ASSERT_TRUE(hasKey(doc, "null"));
    EXPECT_TRUE(find(doc, "null")->isNull());

    // undefined → "undefined" (string)
    ASSERT_TRUE(hasKey(doc, "undefined"));
    ASSERT_TRUE(find(doc, "undefined")->isString());
    EXPECT_EQ(find(doc, "undefined")->asString(), "undefined");

    // zr → ["deedee"]
    ASSERT_TRUE(hasKey(doc, "zr"));
    ASSERT_TRUE(find(doc, "zr")->isArray());
    EXPECT_EQ(find(doc, "zr")->asArray().size(), 1u);
    EXPECT_EQ(find(doc, "zr")->asArray()[0].asString(), "deedee");

    // ar → ["one", "three", "this is included"]
    ASSERT_TRUE(hasKey(doc, "ar"));
    ASSERT_TRUE(find(doc, "ar")->isArray());
    EXPECT_EQ(find(doc, "ar")->asArray().size(), 3u);
    EXPECT_EQ(find(doc, "ar")->asArray()[0].asString(), "one");
    EXPECT_EQ(find(doc, "ar")->asArray()[1].asString(), "three");
    EXPECT_EQ(find(doc, "ar")->asArray()[2].asString(), "this is included");

    // br → "warm" (last value wins)
    ASSERT_TRUE(hasKey(doc, "br"));
    ASSERT_TRUE(find(doc, "br")->isString());
    EXPECT_EQ(find(doc, "br")->asString(), "warm");

    // eq → "eq=eq"
    ASSERT_TRUE(hasKey(doc, "eq"));
    EXPECT_EQ(find(doc, "eq")->asString(), "eq=eq");

    // Section a
    ASSERT_TRUE(hasKey(doc, "a"));
    ASSERT_TRUE(find(doc, "a")->isDocument());
    const auto& secA = find(doc, "a")->asDocument();
    EXPECT_EQ(find(secA, "av")->asString(), "a val");
    EXPECT_EQ(find(secA, "e")->asString(),
              "{ o: p, a: { av: a val, b: { c: { e: \"this [value]\" } } } }");
    // j is the double-quoted JSON string
    EXPECT_EQ(find(secA, "j")->asString(),
              "\"{ o: \"p\", a: { av: \"a val\", b: { c: { e: \"this [value]\" } } } }\"");
    EXPECT_EQ(find(secA, "[]")->asString(), "a square?");

    // cr[] in section a → ["four", "eight"]
    ASSERT_TRUE(hasKey(secA, "cr"));
    ASSERT_TRUE(find(secA, "cr")->isArray());
    EXPECT_EQ(find(secA, "cr")->asArray().size(), 2u);
    EXPECT_EQ(find(secA, "cr")->asArray()[0].asString(), "four");
    EXPECT_EQ(find(secA, "cr")->asArray()[1].asString(), "eight");

    // Section a has nested b → c
    ASSERT_TRUE(hasKey(secA, "b"));
    ASSERT_TRUE(find(secA, "b")->isDocument());
    const auto& secAB = find(secA, "b")->asDocument();
    ASSERT_TRUE(hasKey(secAB, "c"));
    ASSERT_TRUE(find(secAB, "c")->isDocument());
    const auto& secABC = find(secAB, "c")->asDocument();
    EXPECT_EQ(find(secABC, "e")->asString(), "1");
    EXPECT_EQ(find(secABC, "j")->asString(), "2");

    // Section b → empty
    ASSERT_TRUE(hasKey(doc, "b"));
    ASSERT_TRUE(find(doc, "b")->isDocument());
    EXPECT_TRUE(find(doc, "b")->asDocument().empty());

    // Section x.y.z (literal dots)
    ASSERT_TRUE(hasKey(doc, "x.y.z"));
    ASSERT_TRUE(find(doc, "x.y.z")->isDocument());
    const auto& secXYZ = find(doc, "x.y.z")->asDocument();
    EXPECT_EQ(find(secXYZ, "x.y.z")->asString(), "xyz");

    // Nested a.b.c under x.y.z
    ASSERT_TRUE(hasKey(secXYZ, "a.b.c"));
    ASSERT_TRUE(find(secXYZ, "a.b.c")->isDocument());
    const auto& secXYZABC = find(secXYZ, "a.b.c")->asDocument();
    EXPECT_EQ(find(secXYZABC, "a.b.c")->asString(), "abc");
    EXPECT_EQ(find(secXYZABC, "nocomment")->asString(), "this; this is not a comment");
    EXPECT_EQ(find(secXYZABC, "noHashComment")->asString(), "this# this is not a comment");
}

TEST(IniTest, FooIniEncodeFromData) {
    auto doc = parse(FOO_INI);
    std::string result = encode(doc);

    // Build expected output matching the JS snapshot
    std::string expected =
        "o=p\n"
        "a with spaces=b  c\n"
        "\" xa  n          p \"=\"\\\"\\r\\nyoyoyo\\r\\r\\n\"\n"
        "\"[disturbing]\"=hey you never know\n"
        "s=something\n"
        "s1=\"something'\n"
        "s2=something else\n"
        "s3=\n"
        "s4=\n"
        "s5=\"   \"\n"
        "s6=\" a \"\n"
        "s7=true\n"
        "true=true\n"
        "false=false\n"
        "null=null\n"
        "undefined=undefined\n"
        "zr[]=deedee\n"
        "ar[]=one\n"
        "ar[]=three\n"
        "ar[]=this is included\n"
        "br=warm\n"
        "eq=\"eq=eq\"\n"
        "\n"
        "[a]\n"
        "av=a val\n"
        "e={ o: p, a: { av: a val, b: { c: { e: \"this [value]\" } } } }\n"
        "j=\"\\\"{ o: \\\"p\\\", a: { av: \\\"a val\\\", b: { c: { e: \\\"this [value]\\\" } } } }\\\"\"\n"
        "\"[]\"=a square?\n"
        "cr[]=four\n"
        "cr[]=eight\n"
        "\n"
        "[a.b.c]\n"
        "e=1\n"
        "j=2\n"
        "\n"
        "[x\\.y\\.z]\n"
        "x.y.z=xyz\n"
        "\n"
        "[x\\.y\\.z.a\\.b\\.c]\n"
        "a.b.c=abc\n"
        "nocomment=this\\; this is not a comment\n"
        "noHashComment=this\\# this is not a comment\n";

    EXPECT_EQ(result, expected);
}

// ===========================================================================
// 6. Duplicate Property Tests (duplicate.ini)
// ===========================================================================

TEST(IniTest, DuplicatePropertiesBracketedArrayTrue) {
    auto doc = parse(DUPLICATE_INI);

    // zr → ["deedee", "123"]
    ASSERT_TRUE(hasKey(doc, "zr"));
    ASSERT_TRUE(find(doc, "zr")->isArray());
    EXPECT_EQ(find(doc, "zr")->asArray().size(), 2u);
    EXPECT_EQ(find(doc, "zr")->asArray()[0].asString(), "deedee");
    EXPECT_EQ(find(doc, "zr")->asArray()[1].asString(), "123");

    // ar → ["one", "three"]
    ASSERT_TRUE(hasKey(doc, "ar"));
    ASSERT_TRUE(find(doc, "ar")->isArray());
    EXPECT_EQ(find(doc, "ar")->asArray().size(), 2u);
    EXPECT_EQ(find(doc, "ar")->asArray()[0].asString(), "one");
    EXPECT_EQ(find(doc, "ar")->asArray()[1].asString(), "three");

    // str → "3"
    ASSERT_TRUE(hasKey(doc, "str"));
    EXPECT_EQ(find(doc, "str")->asString(), "3");

    // brr → "3" (last value wins, no [] suffix)
    ASSERT_TRUE(hasKey(doc, "brr"));
    ASSERT_TRUE(find(doc, "brr")->isString());
    EXPECT_EQ(find(doc, "brr")->asString(), "3");
}

TEST(IniTest, DuplicatePropertiesBracketedArrayFalse) {
    DecodeOptions opt;
    opt.bracketedArray = false;
    auto doc = parse(DUPLICATE_INI, opt);

    // zr[] → "deedee" (literal key)
    ASSERT_TRUE(hasKey(doc, "zr[]"));
    ASSERT_TRUE(find(doc, "zr[]")->isString());
    EXPECT_EQ(find(doc, "zr[]")->asString(), "deedee");

    // zr → "123"
    ASSERT_TRUE(hasKey(doc, "zr"));
    ASSERT_TRUE(find(doc, "zr")->isString());
    EXPECT_EQ(find(doc, "zr")->asString(), "123");

    // ar[] → "one" (first occurrence)
    ASSERT_TRUE(hasKey(doc, "ar[]"));
    ASSERT_TRUE(find(doc, "ar[]")->isString());
    EXPECT_EQ(find(doc, "ar[]")->asString(), "one");

    // ar → ["three"] (second ar[] triggers array, strips [], becomes "ar")
    ASSERT_TRUE(hasKey(doc, "ar"));
    ASSERT_TRUE(find(doc, "ar")->isArray());
    EXPECT_EQ(find(doc, "ar")->asArray().size(), 1u);
    EXPECT_EQ(find(doc, "ar")->asArray()[0].asString(), "three");

    // str → "3"
    ASSERT_TRUE(hasKey(doc, "str"));
    EXPECT_EQ(find(doc, "str")->asString(), "3");

    // brr → ["1", "2", "3", "3"]
    ASSERT_TRUE(hasKey(doc, "brr"));
    ASSERT_TRUE(find(doc, "brr")->isArray());
    EXPECT_EQ(find(doc, "brr")->asArray().size(), 4u);
    EXPECT_EQ(find(doc, "brr")->asArray()[0].asString(), "1");
    EXPECT_EQ(find(doc, "brr")->asArray()[1].asString(), "2");
    EXPECT_EQ(find(doc, "brr")->asArray()[2].asString(), "3");
    EXPECT_EQ(find(doc, "brr")->asArray()[3].asString(), "3");
}

TEST(IniTest, DuplicatePropertiesEncodeBracketed) {
    IniDocument doc;
    IniValue::ArrayType arr;
    arr.push_back(IniValue("1"));
    arr.push_back(IniValue("2"));
    arr.push_back(IniValue("3"));
    set(doc, "ar", IniValue(std::move(arr)));
    IniValue::ArrayType brr;
    brr.push_back(IniValue("1"));
    brr.push_back(IniValue("2"));
    set(doc, "br", IniValue(std::move(brr)));

    std::string result = encode(doc);
    std::string expected =
        "ar[]=1\n"
        "ar[]=2\n"
        "ar[]=3\n"
        "br[]=1\n"
        "br[]=2\n";
    EXPECT_EQ(result, expected);
}

TEST(IniTest, DuplicatePropertiesEncodeUnbracketed) {
    IniDocument doc;
    IniValue::ArrayType arr;
    arr.push_back(IniValue("1"));
    arr.push_back(IniValue("2"));
    arr.push_back(IniValue("3"));
    set(doc, "ar", IniValue(std::move(arr)));
    IniValue::ArrayType brr;
    brr.push_back(IniValue("1"));
    brr.push_back(IniValue("2"));
    set(doc, "br", IniValue(std::move(brr)));

    std::string result = encode(doc, EncodeOptions{.bracketedArray = false});
    std::string expected =
        "ar=1\n"
        "ar=2\n"
        "ar=3\n"
        "br=1\n"
        "br=2\n";
    EXPECT_EQ(result, expected);
}

// ===========================================================================
// 7. Win32 Tests
// ===========================================================================

TEST(IniTest, Win32Encode) {
    IniDocument bar;
    set(bar, "bar", IniValue("baz"));
    IniDocument doc;
    set(doc, "foo", IniValue(std::move(bar)));

    std::string result = encode(doc, EncodeOptions{.platform = "win32"});
    EXPECT_EQ(result, "[foo]\r\nbar=baz\r\n");
}

TEST(IniTest, Win32EncodeWithSectionString) {
    // encode(obj, "foo") is JS shorthand for section. We test the equivalent:
    IniDocument doc;
    set(doc, "bar", IniValue("baz"));
    EncodeOptions opt;
    opt.section = "foo";
    opt.platform = "win32";
    std::string result = encode(doc, opt);
    EXPECT_EQ(result, "[foo]\r\nbar=baz\r\n");
}

TEST(IniTest, Win32DecodeWithCRLF) {
    auto doc = parse("=just junk!\r\n[foo]\r\nbar\r\n");
    // =just junk! is skipped
    auto* foo = find(doc, "foo");
    ASSERT_NE(foo, nullptr);
    ASSERT_TRUE(foo->isDocument());
    auto* bar = find(foo->asDocument(), "bar");
    ASSERT_NE(bar, nullptr);
    ASSERT_TRUE(bar->isBool());
    EXPECT_TRUE(bar->asBool());
}

TEST(IniTest, Win32DecodeArrayMerge) {
    auto doc = parse("[x]\r\ny=1\r\ny[]=2\r\n");
    auto* x = find(doc, "x");
    ASSERT_NE(x, nullptr);
    ASSERT_TRUE(x->isDocument());
    auto* y = find(x->asDocument(), "y");
    ASSERT_NE(y, nullptr);
    ASSERT_TRUE(y->isArray());
    EXPECT_EQ(y->asArray().size(), 2u);
    // y=1 → "1" (string), then y[]=2 promotes to array [old, new]
    EXPECT_EQ(y->asArray()[0].asString(), "1");
    EXPECT_EQ(y->asArray()[1].asString(), "2");
}

// ===========================================================================
// 8. Encode with align/sort from foo.ini
// ===========================================================================

TEST(IniTest, EncodeWithAlign) {
    auto doc = parse(FOO_INI);
    std::string result = encode(doc, EncodeOptions{.align = true});

    // Verify alignment: all keys padded to 20 chars (length of safe(" xa  n          p "))
    // We check specific lines
    std::string expected =
        "o                    = p\n"
        "a with spaces        = b  c\n"
        "\" xa  n          p \" = \"\\\"\\r\\nyoyoyo\\r\\r\\n\"\n"
        "\"[disturbing]\"       = hey you never know\n"
        "s                    = something\n"
        "s1                   = \"something'\n"
        "s2                   = something else\n"
        "s3                   = \n"
        "s4                   = \n"
        "s5                   = \"   \"\n"
        "s6                   = \" a \"\n"
        "s7                   = true\n"
        "true                 = true\n"
        "false                = false\n"
        "null                 = null\n"
        "undefined            = undefined\n"
        "zr[]                 = deedee\n"
        "ar[]                 = one\n"
        "ar[]                 = three\n"
        "ar[]                 = this is included\n"
        "br                   = warm\n"
        "eq                   = \"eq=eq\"\n"
        "\n"
        "[a]\n"
        "av   = a val\n"
        "e    = { o: p, a: { av: a val, b: { c: { e: \"this [value]\" } } } }\n"
        "j    = \"\\\"{ o: \\\"p\\\", a: { av: \\\"a val\\\", b: { c: { e: \\\"this [value]\\\" } } } }\\\"\"\n"
        "\"[]\" = a square?\n"
        "cr[] = four\n"
        "cr[] = eight\n"
        "\n"
        "[a.b.c]\n"
        "e = 1\n"
        "j = 2\n"
        "\n"
        "[x\\.y\\.z]\n"
        "x.y.z = xyz\n"
        "\n"
        "[x\\.y\\.z.a\\.b\\.c]\n"
        "a.b.c         = abc\n"
        "nocomment     = this\\; this is not a comment\n"
        "noHashComment = this\\# this is not a comment\n";

    EXPECT_EQ(result, expected);
}

TEST(IniTest, EncodeWithSort) {
    auto doc = parse(FOO_INI);
    std::string result = encode(doc, EncodeOptions{.sort = true});

    std::string expected =
        "\" xa  n          p \"=\"\\\"\\r\\nyoyoyo\\r\\r\\n\"\n"
        "\"[disturbing]\"=hey you never know\n"
        "a with spaces=b  c\n"
        "ar[]=one\n"
        "ar[]=three\n"
        "ar[]=this is included\n"
        "br=warm\n"
        "eq=\"eq=eq\"\n"
        "false=false\n"
        "null=null\n"
        "o=p\n"
        "s=something\n"
        "s1=\"something'\n"
        "s2=something else\n"
        "s3=\n"
        "s4=\n"
        "s5=\"   \"\n"
        "s6=\" a \"\n"
        "s7=true\n"
        "true=true\n"
        "undefined=undefined\n"
        "zr[]=deedee\n"
        "\n"
        "[a]\n"
        "\"[]\"=a square?\n"
        "av=a val\n"
        "cr[]=four\n"
        "cr[]=eight\n"
        "e={ o: p, a: { av: a val, b: { c: { e: \"this [value]\" } } } }\n"
        "j=\"\\\"{ o: \\\"p\\\", a: { av: \\\"a val\\\", b: { c: { e: \\\"this [value]\\\" } } } }\\\"\"\n"
        "\n"
        "[a.b.c]\n"
        "e=1\n"
        "j=2\n"
        "\n"
        "[x\\.y\\.z]\n"
        "x.y.z=xyz\n"
        "\n"
        "[x\\.y\\.z.a\\.b\\.c]\n"
        "a.b.c=abc\n"
        "noHashComment=this\\# this is not a comment\n"
        "nocomment=this\\; this is not a comment\n";

    EXPECT_EQ(result, expected);
}

TEST(IniTest, EncodeWithAlignAndSort) {
    auto doc = parse(FOO_INI);
    std::string result = encode(doc, EncodeOptions{.align = true, .sort = true});

    std::string expected =
        "\" xa  n          p \" = \"\\\"\\r\\nyoyoyo\\r\\r\\n\"\n"
        "\"[disturbing]\"       = hey you never know\n"
        "a with spaces        = b  c\n"
        "ar[]                 = one\n"
        "ar[]                 = three\n"
        "ar[]                 = this is included\n"
        "br                   = warm\n"
        "eq                   = \"eq=eq\"\n"
        "false                = false\n"
        "null                 = null\n"
        "o                    = p\n"
        "s                    = something\n"
        "s1                   = \"something'\n"
        "s2                   = something else\n"
        "s3                   = \n"
        "s4                   = \n"
        "s5                   = \"   \"\n"
        "s6                   = \" a \"\n"
        "s7                   = true\n"
        "true                 = true\n"
        "undefined            = undefined\n"
        "zr[]                 = deedee\n"
        "\n"
        "[a]\n"
        "\"[]\" = a square?\n"
        "av   = a val\n"
        "cr[] = four\n"
        "cr[] = eight\n"
        "e    = { o: p, a: { av: a val, b: { c: { e: \"this [value]\" } } } }\n"
        "j    = \"\\\"{ o: \\\"p\\\", a: { av: \\\"a val\\\", b: { c: { e: \\\"this [value]\\\" } } } }\\\"\"\n"
        "\n"
        "[a.b.c]\n"
        "e = 1\n"
        "j = 2\n"
        "\n"
        "[x\\.y\\.z]\n"
        "x.y.z = xyz\n"
        "\n"
        "[x\\.y\\.z.a\\.b\\.c]\n"
        "a.b.c         = abc\n"
        "noHashComment = this\\# this is not a comment\n"
        "nocomment     = this\\; this is not a comment\n";

    EXPECT_EQ(result, expected);
}

// ===========================================================================
// 9. IniDocument Helper Tests
// ===========================================================================

TEST(IniTest, DocumentHelperFind) {
    IniDocument doc;
    set(doc, "key1", IniValue("val1"));
    set(doc, "key2", IniValue("val2"));

    ASSERT_NE(find(doc, "key1"), nullptr);
    EXPECT_EQ(find(doc, "key1")->asString(), "val1");
    EXPECT_EQ(find(doc, "nonexistent"), nullptr);
}

TEST(IniTest, DocumentHelperHasKey) {
    IniDocument doc;
    set(doc, "exists", IniValue("yes"));
    EXPECT_TRUE(hasKey(doc, "exists"));
    EXPECT_FALSE(hasKey(doc, "missing"));
}

TEST(IniTest, DocumentHelperSetOverwrite) {
    IniDocument doc;
    set(doc, "key", IniValue("first"));
    set(doc, "key", IniValue("second"));
    EXPECT_EQ(find(doc, "key")->asString(), "second");
    EXPECT_EQ(doc.size(), 1u);
}

TEST(IniTest, DocumentHelperKeys) {
    IniDocument doc;
    set(doc, "b", IniValue("2"));
    set(doc, "a", IniValue("1"));
    set(doc, "c", IniValue("3"));
    auto k = keys(doc);
    ASSERT_EQ(k.size(), 3u);
    EXPECT_EQ(k[0], "b");
    EXPECT_EQ(k[1], "a");
    EXPECT_EQ(k[2], "c");
}

TEST(IniTest, DocumentHelperRemove) {
    IniDocument doc;
    set(doc, "key", IniValue("value"));
    EXPECT_TRUE(remove(doc, "key"));
    EXPECT_FALSE(hasKey(doc, "key"));
    EXPECT_FALSE(remove(doc, "key"));
}

// ===========================================================================
// 10. IniValue Type Tests
// ===========================================================================

TEST(IniTest, IniValueTypes) {
    IniValue null_val(nullptr);
    EXPECT_TRUE(null_val.isNull());
    EXPECT_FALSE(null_val.isBool());
    EXPECT_FALSE(null_val.isString());

    IniValue bool_val(true);
    EXPECT_TRUE(bool_val.isBool());
    EXPECT_TRUE(bool_val.asBool());

    IniValue str_val("hello");
    EXPECT_TRUE(str_val.isString());
    EXPECT_EQ(str_val.asString(), "hello");

    IniValue::ArrayType arr;
    arr.push_back(IniValue("item"));
    IniValue arr_val(std::move(arr));
    EXPECT_TRUE(arr_val.isArray());
    EXPECT_EQ(arr_val.asArray().size(), 1u);

    IniDocument inner;
    set(inner, "k", IniValue("v"));
    IniValue doc_val(std::move(inner));
    EXPECT_TRUE(doc_val.isDocument());
}

TEST(IniTest, IniValueEquality) {
    EXPECT_EQ(IniValue(nullptr), IniValue(nullptr));
    EXPECT_EQ(IniValue(true), IniValue(true));
    EXPECT_EQ(IniValue("test"), IniValue("test"));
    EXPECT_NE(IniValue(true), IniValue(false));
    EXPECT_NE(IniValue("a"), IniValue("b"));
    EXPECT_NE(IniValue(nullptr), IniValue(true));
}

TEST(IniTest, IniValueDefaultIsNull) {
    IniValue v;
    EXPECT_TRUE(v.isNull());
}

// ===========================================================================
// Surrogate pair handling in JSON-quoted values
// ===========================================================================

TEST(IniTest, UnsafeSurrogatePairEmoji) {
    // \uD83D\uDE00 is the surrogate pair for U+1F600 (😀)
    // UTF-8 encoding: F0 9F 98 80
    std::string input = "\"\\uD83D\\uDE00\"";
    std::string result = unsafe(input);
    EXPECT_EQ(result.size(), 4u);
    EXPECT_EQ(static_cast<unsigned char>(result[0]), 0xF0);
    EXPECT_EQ(static_cast<unsigned char>(result[1]), 0x9F);
    EXPECT_EQ(static_cast<unsigned char>(result[2]), 0x98);
    EXPECT_EQ(static_cast<unsigned char>(result[3]), 0x80);
}

TEST(IniTest, UnsafeSurrogatePairInValue) {
    // INI value with JSON-quoted emoji should round-trip through parse
    auto doc = parse("key=\"\\uD83D\\uDE00\"\n");
    auto* val = find(doc, "key");
    ASSERT_NE(val, nullptr);
    ASSERT_TRUE(val->isString());
    // U+1F600 in UTF-8 is F0 9F 98 80
    std::string expected = "\xF0\x9F\x98\x80";
    EXPECT_EQ(val->asString(), expected);
}

TEST(IniTest, UnsafeLoneSurrogatePreservesAsWtf8) {
    // A lone high surrogate is preserved as WTF-8 by polycpp::JSON::parse,
    // matching Node.js behavior
    std::string input = "\"\\uD83D\"";
    std::string result = unsafe(input);
    std::string expected = "\xED\xA0\xBD"; // U+D83D in WTF-8/CESU-8
    EXPECT_EQ(result, expected);
}

TEST(IniTest, UnsafeLoneLowSurrogatePreservesAsWtf8) {
    // A lone low surrogate is preserved as WTF-8 by polycpp::JSON::parse,
    // matching Node.js behavior
    std::string input = "\"\\uDE00\"";
    std::string result = unsafe(input);
    std::string expected = "\xED\xB8\x80"; // U+DE00 in WTF-8/CESU-8
    EXPECT_EQ(result, expected);
}

TEST(IniTest, UnsafeSingleQuotedWithJsonContent) {
    // Single-quoted value wrapping valid JSON should be JSON-parsed
    // (matches JS behavior: strip quotes then JSON.parse)
    EXPECT_EQ(unsafe("'\"hello\"'"), "hello");
    // Plain single-quoted: JSON.parse fails, returns stripped content
    EXPECT_EQ(unsafe("'something'"), "something");
}

// ===========================================================================
// Deliberate behavior divergences from upstream JS (pinning tests)
// See docs/divergences.md ## Deliberate Behavior Changes for rationale.
// ===========================================================================

TEST(IniTest, SectionHeaderOverwritesPreExistingScalar) {
    // AF-2026-05-04-H: when a section header [foo] follows an earlier
    // foo=bar row, the C++ port replaces the scalar with a fresh
    // section. Upstream JS would silently keep the scalar and drop
    // the subsequent key=val row.
    auto doc = parse("foo=bar\n[foo]\nkey=val\n");
    auto* foo = find(doc, "foo");
    ASSERT_NE(foo, nullptr);
    ASSERT_TRUE(foo->isDocument());
    auto* key = find(foo->asDocument(), "key");
    ASSERT_NE(key, nullptr);
    EXPECT_EQ(key->asString(), "val");
}

TEST(IniTest, SectionHeaderOverwritesPreExistingArray) {
    // AF-2026-05-04-H: same rule for an existing array.
    auto doc = parse("foo[]=a\nfoo[]=b\n[foo]\nkey=val\n");
    auto* foo = find(doc, "foo");
    ASSERT_NE(foo, nullptr);
    ASSERT_TRUE(foo->isDocument());
    EXPECT_FALSE(foo->isArray());
    auto* key = find(foo->asDocument(), "key");
    ASSERT_NE(key, nullptr);
    EXPECT_EQ(key->asString(), "val");
}

// ===========================================================================
// IniValue::toJSON() and polycpp::JSON::stringify(IniValue) interop
// ===========================================================================

TEST(IniTest, ToJsonScalarVariants) {
    EXPECT_TRUE(IniValue(nullptr).toJSON().isNull());
    EXPECT_TRUE(IniValue(true).toJSON().isBool());
    EXPECT_TRUE(IniValue(true).toJSON().asBool());
    EXPECT_FALSE(IniValue(false).toJSON().asBool());
    EXPECT_EQ(IniValue("hello").toJSON().asString(), "hello");
    // Numeric-looking INI values stay strings, mirroring upstream.
    EXPECT_EQ(IniValue("42").toJSON().asString(), "42");
}

TEST(IniTest, ToJsonArray) {
    IniValue::ArrayType arr;
    arr.push_back(IniValue("one"));
    arr.push_back(IniValue("two"));
    arr.push_back(IniValue(true));
    polycpp::JsonValue j = IniValue(std::move(arr)).toJSON();
    ASSERT_TRUE(j.isArray());
    ASSERT_EQ(j.asArray().size(), 3u);
    EXPECT_EQ(j.asArray()[0].asString(), "one");
    EXPECT_EQ(j.asArray()[1].asString(), "two");
    EXPECT_TRUE(j.asArray()[2].asBool());
}

TEST(IniTest, ToJsonNestedDocumentPreservesOrder) {
    IniDocument inner;
    set(inner, "host", IniValue("localhost"));
    set(inner, "port", IniValue("5432"));
    IniDocument doc;
    set(doc, "db", IniValue(std::move(inner)));
    set(doc, "debug", IniValue(true));

    polycpp::JsonValue j = IniValue(std::move(doc)).toJSON();
    ASSERT_TRUE(j.isObject());

    // Keys preserve insertion order (db before debug).
    auto keys_it = j.asObject().begin();
    ASSERT_NE(keys_it, j.asObject().end());
    EXPECT_EQ(keys_it->first, "db");
    ++keys_it;
    ASSERT_NE(keys_it, j.asObject().end());
    EXPECT_EQ(keys_it->first, "debug");

    EXPECT_EQ(j.asObject().at("db").asObject().at("host").asString(), "localhost");
    EXPECT_EQ(j.asObject().at("db").asObject().at("port").asString(), "5432");
    EXPECT_TRUE(j.asObject().at("debug").asBool());
}

TEST(IniTest, JSONStringifyOnIniValueViaHasToJson) {
    // polycpp's HasToJson concept lights up the templated
    // polycpp::JSON::stringify(const T&) overload as soon as
    // IniValue::toJSON() exists.
    auto doc = parse("[a]\nx=1\n");
    IniValue v(std::move(doc));
    std::string j = polycpp::JSON::stringify(v);
    EXPECT_EQ(j, "{\"a\":{\"x\":\"1\"}}");
}

TEST(IniTest, JSONStringifyOnIniValueArrayRoundtrip) {
    IniValue::ArrayType arr;
    arr.push_back(IniValue("a"));
    arr.push_back(IniValue(false));
    arr.push_back(IniValue(nullptr));
    IniValue v(std::move(arr));
    EXPECT_EQ(polycpp::JSON::stringify(v), "[\"a\",false,null]");
}

TEST(IniTest, DottedSectionMergeOverwritesIntermediateArray) {
    // AF-2026-05-04-I: during the dotted-section merge post-pass,
    // when an intermediate path part collides with an existing array,
    // the C++ port replaces the array with a fresh document.
    // Upstream JS would reuse the array and silently set a hidden
    // named property on it.
    auto doc = parse("foo[]=item1\n[foo.bar]\nkey=val\n");
    auto* foo = find(doc, "foo");
    ASSERT_NE(foo, nullptr);
    ASSERT_TRUE(foo->isDocument());
    EXPECT_FALSE(foo->isArray());
    auto* bar = find(foo->asDocument(), "bar");
    ASSERT_NE(bar, nullptr);
    ASSERT_TRUE(bar->isDocument());
    auto* key = find(bar->asDocument(), "key");
    ASSERT_NE(key, nullptr);
    EXPECT_EQ(key->asString(), "val");
}
