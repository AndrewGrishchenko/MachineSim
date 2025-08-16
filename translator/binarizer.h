#ifndef _BINARIZER_H
#define _BINARIZER_H

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

constexpr size_t HEX_VAL = 16;
constexpr size_t const_5 = 5;

constexpr uint32_t MASK_24 = 0xFFFFFF;
constexpr uint32_t MASK_8  = 0xFF;

constexpr uint32_t SHIFT_24 = 24;
constexpr uint32_t SHIFT_16 = 16;
constexpr uint32_t SHIFT_8  = 8;

class Binarizer {
public:
    Binarizer() = default;

    void parse(const std::string& data);
    void writeToFile(const std::string& filename) const;

private:
    struct Instruction {
        uint8_t opcode;
        uint32_t operand;
    };

    // clang-format off
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    std::unordered_map<std::string, uint8_t> opcodeMap = {
        {"add",  0b000001},
        {"sub",  0b000010},
        {"div",  0b000011},
        {"mul",  0b000100},
        {"rem",  0b000101},
        {"inc",  0b000110},
        {"dec",  0b000111},
        {"not",  0b001000},
        {"cla",  0b001001},
        {"jmp",  0b001010},
        {"cmp",  0b001011},
        {"jz",   0b001100},
        {"jnz",  0b001101},
        {"jg",   0b001110},
        {"jge",  0b001111},
        {"jl",   0b010000},
        {"jle",  0b010001},
        {"ja",   0b010010},
        {"jae",  0b010011},
        {"jb",   0b010100},
        {"jbe",  0b010101},
        {"push", 0b010110},
        {"pop",  0b010111},
        {"ld",   0b011000},
        {"lda",  0b011001},
        {"ldi",  0b011010},
        {"st",   0b011011},
        {"sta",  0b011100},
        {"call", 0b011101},
        {"ret",  0b011110},
        {"ei",   0b011111},
        {"di",   0b100000},
        {"iret", 0b100001},
        {"halt", 0b100010}
    };
    // clang-format off
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

    enum class Section : uint8_t { None, Text, Data, InterruptTable };

    size_t textStart = 0;
    size_t dataStart = 0;

    std::vector<uint32_t> dataSection;
    std::unordered_map<std::string, size_t> dataAddress;

    std::vector<Instruction> instructions;
    std::unordered_map<std::string, size_t> labelAddress;

    static void trim(std::string& val) {
        size_t start = val.find_first_not_of(" \t\r\n");
        size_t end = val.find_last_not_of(" \t\r\n");
        val = (start == std::string::npos) ? "" : val.substr(start, end - start + 1);
    }

    static void stripComment(std::string& val) {
        size_t pos = val.find(';');
        if (pos != std::string::npos) {
            val = val.substr(0, pos);
        }
    }

    static void toLower(std::string& val) {
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    }

    static bool isNumber(const std::string& val) {
        if (val.empty()) {
            return false;
        }

        if (val.size() > 2 && val[0] == '0' && (val[1] == 'x' || val[1] == 'X')) {
            return std::all_of(val.begin() + 2, val.end(), ::isxdigit);
        }
        if (val.size() > 2 && val[0] == '0' && (val[1] == 'b' || val[1] == 'B')) {
            return std::all_of(val.begin() + 2, val.end(), [](char character) { return character == '0' || character == '1'; });
        }

        long start = (val[0] == '-') ? 1 : 0;
        return std::all_of(val.begin() + start, val.end(), ::isdigit);
    }

    static long parseNumber(const std::string& val) {
        if (val.size() > 2 && val.substr(0, 2) == "0x") {
            return std::stol(val, nullptr, HEX_VAL);
        }
        if (val.size() > 2 && val.substr(0, 2) == "0b") {
            return std::stol(val.substr(2), nullptr, 2);
        }
        return std::stol(val);
    }
};

#endif