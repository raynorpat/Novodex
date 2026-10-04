// Reconstructed from the shipped VC6 ODBlock.obj. See reconstruction/ODBlock.md.
#include "ODBlock.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

namespace {
// Original .data _map at 0x000c: five permutations of bytes 33 through 127.
const unsigned char cryptMap[95 * 5] = {
    16, 23, 93, 4, 59, 82, 91, 10, 41, 14, 5, 58, 33, 22, 31, 51, 67, 4, 33, 71,
    91, 6, 89, 7, 93, 60, 20, 90, 44, 82, 78, 63, 60, 43, 66, 68, 79, 26, 36, 34,
    81, 26, 85, 14, 41, 53, 60, 88, 80, 80, 55, 51, 58, 26, 2, 86, 16, 64, 20, 22,
    75, 33, 6, 83, 88, 84, 68, 57, 75, 3, 85, 30, 50, 32, 49, 52, 75, 20, 81, 75,
    15, 50, 5, 51, 6, 74, 81, 92, 38, 48, 12, 66, 70, 86, 72, 47, 37, 51, 91, 84,
    49, 88, 3, 82, 52, 80, 94, 63, 48, 81, 56, 22, 38, 10, 69, 30, 9, 35, 53, 38,
    90, 48, 84, 6, 89, 35, 43, 52, 9, 40, 63, 71, 23, 46, 37, 42, 69, 79, 35, 60,
    25, 72, 29, 39, 58, 19, 36, 54, 60, 11, 8, 12, 80, 62, 92, 13, 54, 12, 92, 45,
    67, 87, 11, 37, 91, 11, 7, 72, 1, 90, 38, 32, 19, 29, 57, 48, 73, 91, 54, 70,
    77, 1, 66, 93, 1, 58, 76, 0, 8, 79, 73, 21, 36, 45, 9, 93, 5, 17, 0, 26,
    34, 38, 41, 79, 67, 20, 14, 7, 73, 65, 64, 28, 82, 70, 77, 29, 0, 76, 49, 30,
    83, 29, 22, 15, 32, 7, 85, 74, 56, 85, 92, 3, 25, 40, 68, 33, 70, 68, 17, 43,
    40, 78, 32, 58, 4, 1, 17, 48, 18, 15, 76, 35, 37, 2, 76, 70, 13, 75, 89, 61,
    17, 10, 16, 67, 53, 57, 24, 24, 42, 19, 23, 11, 49, 52, 46, 2, 90, 44, 30, 28,
    44, 2, 15, 25, 8, 66, 55, 42, 74, 94, 14, 92, 53, 50, 74, 69, 27, 27, 47, 27,
    10, 31, 69, 11, 16, 54, 42, 28, 64, 35, 65, 84, 59, 71, 55, 18, 77, 14, 78, 36,
    94, 39, 86, 65, 87, 26, 40, 40, 90, 25, 45, 44, 83, 77, 24, 61, 47, 34, 59, 10,
    36, 74, 73, 57, 29, 88, 83, 1, 13, 54, 79, 25, 18, 94, 12, 89, 41, 55, 27, 83,
    0, 46, 30, 85, 73, 50, 52, 45, 28, 39, 71, 34, 2, 12, 21, 21, 64, 13, 69, 62,
    46, 15, 46, 34, 23, 39, 19, 39, 72, 44, 4, 65, 77, 31, 78, 87, 53, 56, 3, 86,
    6, 61, 43, 76, 18, 37, 59, 21, 24, 47, 28, 93, 71, 23, 33, 72, 82, 61, 63, 42,
    41, 49, 65, 68, 7, 43, 80, 67, 19, 20, 22, 45, 62, 66, 5, 32, 18, 94, 21, 17,
    31, 62, 31, 88, 50, 3, 89, 47, 55, 13, 59, 86, 81, 61, 63, 62, 4, 8, 5, 56,
    24, 57, 87, 87, 51, 27, 8, 78, 84, 64, 9, 56, 9, 16, 0
};

struct InverseMap {
    unsigned char bytes[95 * 5];
    InverseMap() {
        for (unsigned c = 0; c < 95; ++c)
            for (unsigned state = 0; state < 5; ++state)
                bytes[cryptMap[c * 5 + state] * 5 + state] = static_cast<unsigned char>(c);
    }
};
const InverseMap inverseMap;
char noname[] = "_noname";
thread_local unsigned outputDepth = 1;
struct IndentScope {
    IndentScope() { ++outputDepth; }
    ~IndentScope() { --outputDepth; }
};
}

unsigned ODBlock::cryptState = 0;
const char * ODBlock::lastError = 0;

const char * ODBlock::ODSyntaxError::asString()
{
    static const char * const messages[] = {
        "Quote unexpected.", "Opening brace unexpected.", "Closing brace unexpected.",
        "Literal or semicolon unexpected.", "Unexpected end of file found.",
        "Unknown encryption algo code."
    };
    return static_cast<unsigned>(err) < sizeof(messages)/sizeof(*messages) ? messages[err] : 0;
}

char ODBlock::encrypt(char c, unsigned & state)
{
    const int value = static_cast<signed char>(c);
    if (value <= 32) return c;
    const unsigned index = value - 33;
    if (state < 1 || state > 5) state = 1;
    const unsigned mapped = cryptMap[index * 5 + state - 1];
    state = (index + state + mapped) % 5 + 1;
    return static_cast<char>(mapped + 33);
}

char ODBlock::decrypt(char c, unsigned & state)
{
    const int value = static_cast<signed char>(c);
    if (value <= 32) return c;
    const unsigned index = value - 33;
    if (state < 1 || state > 5) state = 1;
    const unsigned mapped = inverseMap.bytes[index * 5 + state - 1];
    state = (index + state + mapped) % 5 + 1;
    return static_cast<char>(mapped + 33);
}

void ODBlock::encrypt(char * text, std::ostream & output, unsigned & state)
{
    if (text) while (*text) output.put(encrypt(*text++, state));
}

ODBlock * ODBlock::loadScript(std::istream & input)
{
    try {
        input >> std::skipws;
        char first = 0;
        if (!(input >> first)) throw ODSyntaxError(ODSyntaxError::ODSE_UNEX_EOF);
        if (first == '!') {
            char algorithm = 0;
            if (!(input >> algorithm) || algorithm != '1')
                throw ODSyntaxError(ODSyntaxError::ODSE_ENC_UNKNOWN);
            cryptState = 1;
            return new ODBlock(input, true);
        }
        return new ODBlock(input, false, first);
    } catch (ODSyntaxError & error) {
        lastError = error.asString();
        return 0;
    }
}

ODBlock::ODBlock() : identifier(0), identSize(0), bTerminal(true), termiter(subBlocks.begin()) {}

ODBlock::~ODBlock()
{
    for (ODBlockList::iterator i = subBlocks.begin(); i != subBlocks.end(); ++i) delete *i;
    delete [] identifier;
}

ODBlock::ODBlock(std::istream & input, bool encrypted, char first) : ODBlock()
{
    unsigned state = 0; // 0: no identifier, 1: identifier, 3: child statements.
    bool quoted = false;
    std::string name;
    for (;;) {
        char c;
        if (first) { c = first; first = 0; }
        else {
            if (!(input >> c)) throw ODSyntaxError(ODSyntaxError::ODSE_UNEX_EOF);
            if (encrypted) c = decrypt(c, cryptState);
        }
        if (c == '"') {
            if (state == 3) {
                std::unique_ptr<ODBlock> child(new ODBlock(input, encrypted, c));
                addStatement(*child);
                child.release();
            } else {
                if (state > 1) throw ODSyntaxError(ODSyntaxError::ODSE_UNEX_QUOTE);
                quoted = !quoted;
                state = 1;
                if (quoted) input >> std::noskipws;
                else input >> std::skipws;
            }
        } else if (quoted) {
            name.push_back(c);
        } else if (c == '#') {
            input.ignore(0xffffff, '\n');
        } else if (c == '/') {
            // The VC6 parser skips up to the next slash (the end of /* ... */).
            input.ignore(0xffffff, '/');
        } else if (c == '{') {
            if (state != 1) throw ODSyntaxError(ODSyntaxError::ODSE_UNEX_OBRACE);
            state = 3;
        } else if (c == '}') {
            if (state != 3) throw ODSyntaxError(ODSyntaxError::ODSE_UNEX_CBRACE);
            if (!name.empty()) ident(name.c_str());
            reset();
            return;
        } else if (state == 3) {
            std::unique_ptr<ODBlock> child(new ODBlock(input, encrypted, c));
            addStatement(*child);
            child.release();
        } else if (state == 1 && c == ';') {
            if (!name.empty()) ident(name.c_str());
            reset();
            return;
        } else {
            name.push_back(c);
            state = 1;
        }
    }
}

void ODBlock::saveFile(std::ostream & output, bool quote, bool encrypted)
{
    if (encrypted) {
        output << "!1";
        cryptState = 1;
        saveScript(output, true, true);
    } else saveScript(output, quote, false);
}

void ODBlock::saveScript(std::ostream & output, bool quote, bool encrypted)
{
    if (encrypted) {
        output.put(encrypt('"', cryptState));
        encrypt(identifier ? identifier : noname, output, cryptState);
        output.put(encrypt('"', cryptState));
        if (bTerminal) output.put(encrypt(';', cryptState));
        else {
            output.put(encrypt('{', cryptState));
            for (ODBlockList::iterator i = subBlocks.begin(); i != subBlocks.end(); ++i)
                (*i)->saveScript(output, quote, true);
            output.put(encrypt('}', cryptState));
        }
        return;
    }
    output << std::string(outputDepth - 1, '\t');
    if (quote) output.put('"');
    output << ident();
    if (quote) output.put('"');
    if (bTerminal) output << ";\n";
    else {
        output << '\n' << std::string(outputDepth, '\t') << "{\n";
        {
            IndentScope indent;
            for (ODBlockList::iterator i = subBlocks.begin(); i != subBlocks.end(); ++i)
                (*i)->saveScript(output, quote, false);
        }
        output << std::string(outputDepth, '\t') << "}\n";
    }
}

void ODBlock::saveScript(FILE * file, bool quote)
{
    if (!file) return;
    std::ostringstream output;
    saveScript(output, quote, false);
    const std::string text = output.str();
    std::fwrite(text.data(), 1, text.size(), file);
}

const char * ODBlock::ident() { return identifier ? identifier : noname; }
bool ODBlock::isTerminal() { return bTerminal; }

void ODBlock::ident(const char * name)
{
    if (!name) return;
    const unsigned needed = static_cast<unsigned>(std::strlen(name) + 1);
    if (identSize < needed) {
        char * replacement = new char[needed];
        std::memcpy(replacement, name, needed);
        delete [] identifier;
        identifier = replacement;
        identSize = needed;
    } else std::memmove(identifier, name, needed);
}

void ODBlock::addStatement(ODBlock & child)
{
    const ODBlockList::difference_type position = termiter - subBlocks.begin();
    subBlocks.push_back(&child);
    bTerminal = false;
    termiter = subBlocks.begin() + position;
}

ODBlock * ODBlock::getBlock(const char * name, bool recursive)
{
    if (!name) return 0;
    if (identifier && std::strncmp(identifier, name, OD_MAXID) == 0) return this;
    for (ODBlockList::iterator i = subBlocks.begin(); i != subBlocks.end(); ++i) {
        if (recursive) {
            ODBlock * found = (*i)->getBlock(name, true);
            if (found) return found;
        } else if ((*i)->identifier && std::strncmp((*i)->identifier, name, OD_MAXID) == 0) return *i;
    }
    return 0;
}

void ODBlock::reset() { termiter = subBlocks.begin(); }

bool ODBlock::moreTerminals()
{
    while (termiter != subBlocks.end() && !(*termiter)->bTerminal) ++termiter;
    return termiter != subBlocks.end();
}

char * ODBlock::nextTerminal()
{
    if (termiter == subBlocks.end()) return 0;
    ODBlock * child = *termiter++;
    return child->identifier ? child->identifier : noname;
}

bool ODBlock::moreSubBlocks() { return termiter != subBlocks.end(); }
ODBlock * ODBlock::nextSubBlock() { return moreSubBlocks() ? *termiter++ : 0; }

bool ODBlock::getBlockInt(const char * name, int * value)
{
    ODBlock * block = getBlock(name);
    if (!block) return false;
    block->reset();
    if (!block->moreTerminals()) return false;
    if (value) std::sscanf(block->nextTerminal(), "%d", value);
    return true;
}

bool ODBlock::getBlockString(const char * name, const char ** value)
{
    ODBlock * block = getBlock(name);
    if (!block) return false;
    block->reset();
    if (!block->moreTerminals()) return false;
    if (value) *value = block->nextTerminal();
    return true;
}

bool ODBlock::getBlockFloat(const char * name, float * value)
{
    ODBlock * block = getBlock(name);
    if (!block) return false;
    block->reset();
    if (!block->moreTerminals()) return false;
    if (value) std::sscanf(block->nextTerminal(), "%f", value);
    return true;
}

bool ODBlock::getBlockFloats(const char * name, float * values, unsigned count)
{
    ODBlock * block = getBlock(name);
    if (!block) return false;
    block->reset();
    for (unsigned i = 0; i < count; ++i) {
        if (block->moreTerminals()) {
            const char * text = block->nextTerminal();
            if (values) std::sscanf(text, "%f", values + i);
        }
    }
    return true;
}
