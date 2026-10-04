#include "ODBlock.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

static std::unique_ptr<ODBlock> parse(const std::string & source)
{
    std::istringstream input(source);
    return std::unique_ptr<ODBlock>(ODBlock::loadScript(input));
}

int main(int argc, char ** argv)
{
    // Missing recursion, comment handling, or quoted delimiters breaks real scene data.
    std::unique_ptr<ODBlock> root = parse("# header\nROOT { first; /* comment */ nested { leaf; } \"file name/with#chars\"; count { -12; } vec { 1.5; -2; 3e2; } empty {} }");
    CHECK(root.get() != 0);
    if (!root) return 1;
    CHECK(std::strcmp(root->ident(), "ROOT") == 0);
    CHECK(root->numSubBlocks() == 6);
    CHECK(!root->isTerminal());
    CHECK(root->getBlock("ROOT") == root.get());
    CHECK(root->getBlock("leaf") == 0);
    CHECK(root->getBlock("leaf", true) != 0);
    CHECK(root->getBlock("missing", true) == 0);
    root->reset();
    CHECK(root->moreTerminals());
    CHECK(std::strcmp(root->nextTerminal(), "first") == 0);
    CHECK(root->moreTerminals());
    CHECK(std::strcmp(root->nextTerminal(), "file name/with#chars") == 0);
    CHECK(root->moreTerminals());
    // The original marks empty blocks terminal, even when braces were supplied.
    CHECK(std::strcmp(root->nextTerminal(), "empty") == 0);
    CHECK(!root->moreTerminals());
    root->reset();
    unsigned children = 0;
    while (root->moreSubBlocks()) { CHECK(root->nextSubBlock() != 0); ++children; }
    CHECK(children == 6);
    int integer = 0;
    CHECK(root->getBlockInt("count", &integer) && integer == -12);
    CHECK(root->getBlockInt("count"));
    CHECK(!root->getBlockInt("missing", &integer));
    float values[] = { 9,9,9,9 };
    CHECK(root->getBlockFloats("vec", values, 4));
    CHECK(values[0] == 1.5f && values[1] == -2 && values[2] == 300 && values[3] == 9);
    float scalar = 0;
    CHECK(root->getBlockFloat("vec", &scalar) && scalar == 1.5f);
    const char * string = 0;
    CHECK(root->getBlockString("count", &string) && std::strcmp(string, "-12") == 0);
    CHECK(!root->getBlockString("empty", &string));

    // Fixture bytes are derived from the original COFF map, not a roundtrip.
    const char * ciphertext = "!1s0ee^+`Jtj6j MHOZRcOiJ@nXx\\h|FJq+`JB&QsFJces|Fxx";
    std::unique_ptr<ODBlock> coded = parse(ciphertext);
    CHECK(coded.get() != 0);
    if (coded) {
        CHECK(std::strcmp(coded->ident(), "ROOT") == 0);
        CHECK(coded->numSubBlocks() == 2);
        CHECK(coded->getBlockInt("v", &integer) && integer == -12);
        std::ostringstream out;
        coded->saveFile(out, false, true);
        CHECK(out.str() == ciphertext);
        std::ostringstream plain;
        coded->saveFile(plain, true);
        CHECK(plain.str() == "\"ROOT\"\n\t{\n\t\"file name/with#chars\";\n\t\"v\"\n\t\t{\n\t\t\"-12\";\n\t\t\"3.5\";\n\t\t}\n\t}\n");
        CHECK(parse(plain.str()).get() != 0);
        FILE * fp = std::tmpfile();
        CHECK(fp != 0);
        if (fp) {
            coded->saveScript(fp, true);
            std::rewind(fp);
            std::string actual;
            int byte;
            while ((byte = std::fgetc(fp)) != EOF) actual.push_back(static_cast<char>(byte));
            CHECK(actual == plain.str());
            std::fclose(fp);
        }
    }

    // Malformed recursion must return a useful error instead of accepting truncation.
    const char * bad[] = { "", "ROOT {", "ROOT { leaf", "ROOT }", "{", "!2junk", "ROOT { \"unterminated" };
    const char * errors[] = { "Unexpected end of file found.", "Unexpected end of file found.", "Unexpected end of file found.", "Closing brace unexpected.", "Opening brace unexpected.", "Unknown encryption algo code.", "Unexpected end of file found." };
    for (unsigned i = 0; i < sizeof(bad)/sizeof(*bad); ++i) {
        CHECK(parse(bad[i]).get() == 0);
        CHECK(ODBlock::lastError && std::strcmp(ODBlock::lastError, errors[i]) == 0);
    }
    std::unique_ptr<ODBlock> whitespace = parse("a b c;");
    CHECK(whitespace && std::strcmp(whitespace->ident(), "abc") == 0);
    std::unique_ptr<ODBlock> longid = parse("ROOT { abcdefghijklmnopqrstuvwxyz123456-other; }");
    CHECK(longid && longid->getBlock("abcdefghijklmnopqrstuvwxyz123456-different") != 0);
    std::unique_ptr<ODBlock> invalidNumber = parse("ROOT { count { abc; } }");
    integer = 42;
    CHECK(invalidNumber && invalidNumber->getBlockInt("count", &integer) && integer == 42);
    ODBlock built;
    CHECK(std::strcmp(built.ident(), "_noname") == 0 && built.isTerminal());
    built.ident("made");
    ODBlock * child = new ODBlock;
    child->ident("100% complete");
    built.addStatement(*child); // Ownership transfers to the containing block.
    built.reset();
    CHECK(!built.isTerminal() && built.nextSubBlock() == child);
    std::ostringstream builtText;
    built.saveScript(builtText, true);
    CHECK(parse(builtText.str()).get() != 0);

    // Optional manifest contains one absolute .ods path per line for corpus checks.
    if (argc > 1) {
        std::ifstream manifest(argv[1]);
        CHECK(manifest.good());
        std::string path;
        unsigned scenes = 0;
        while (std::getline(manifest, path)) {
            if (!path.empty() && path.back() == '\r') path.pop_back();
            std::ifstream input(path.c_str(), std::ios::binary);
            std::unique_ptr<ODBlock> scene(ODBlock::loadScript(input));
            if (!scene) { std::fprintf(stderr, "%s: %s\n", path.c_str(), ODBlock::lastError); ++failures; }
            ++scenes;
        }
        CHECK(scenes > 0);
        std::printf("Parsed %u ViewerScenes scripts\n", scenes);
    }
    std::printf("GraphicsScriptTests: %d failures\n", failures);
    return failures ? 1 : 0;
}
