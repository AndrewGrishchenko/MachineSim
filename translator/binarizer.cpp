#include "binarizer.h"

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Binarizer::parse(const std::string& inputData) {
    std::istringstream iss(inputData);
    std::string line;

    Section current = Section::None;

    textStart = 1;
    dataStart = 0;

    size_t textSize = 0;
    size_t dataSize = 0;

    labelAddress.clear();
    dataAddress.clear();
    instructions.clear();
    dataSection.clear();

    while (std::getline(iss, line)) {
        stripComment(line);
        trim(line);
        if (line.empty()) {
            continue;
        }

        if (line == ".text") {
            current = Section::Text;
            continue;
        }
        if (line == ".data") {
            current = Section::Data;
            continue;
        }

        if (line.size() > 4 && line.substr(0, 4) == ".org") {
            std::string addrStr = line.substr(4);
            trim(addrStr);
            size_t orgAddr = 0;
            if (!isNumber(addrStr)) {
                throw std::runtime_error(".org must have numeric address: " + addrStr);
            }
            orgAddr = parseNumber(addrStr);
            if (current == Section::Text) {
                textStart = orgAddr;
            } else if (current == Section::Data) {
                dataStart = orgAddr;
            }
            continue;
        }

        if (line.back() == ':') {
            std::string label = line.substr(0, line.size() - 1);
            trim(label);

            size_t absoluteAddr = 0;
            if (current == Section::Text) {
                absoluteAddr        = textStart + textSize;
                labelAddress[label] = absoluteAddr;
            } else if (current == Section::Data) {
                absoluteAddr       = dataStart + dataSize;
                dataAddress[label] = absoluteAddr;
            }
            continue;
        }

        if (current == Section::Text) {
            textSize++;
        } else if (current == Section::Data) {
            dataSize++;
        }
    }

    dataStart = textStart + textSize;

    instructions.resize(textSize);
    dataSection.resize(dataSize);
    iss.clear();
    iss.seekg(0);
    current = Section::None;

    size_t textCursor = 0;
    size_t dataCursor = 0;

    while (std::getline(iss, line)) {
        stripComment(line);
        trim(line);
        if (line.empty()) {
            continue;
        }

        if (line == ".text") {
            current    = Section::Text;
            textCursor = 0;
            continue;
        }
        if (line == ".data") {
            current = Section::Data;
            continue;
        }

        if (line.size() > 4 && line.substr(0, 4) == ".org") {
            std::string addrStr = line.substr(4);
            trim(addrStr);
            size_t orgAddr = 0;
            if (!isNumber(addrStr)) {
                throw std::runtime_error(".org must have numeric address: " + addrStr);
            }
            orgAddr = parseNumber(addrStr);

            if (current == Section::Text) {
                textCursor = orgAddr - textStart;
            } else if (current == Section::Data) {
                dataCursor = orgAddr - dataStart;
            }
            continue;
        }

        if (line.back() == ':') {
            continue;
        }

        if (current == Section::Text) {
            std::istringstream iss(line);
            std::string mnemonic;
            std::string operandStr;
            iss >> mnemonic >> operandStr;
            toLower(mnemonic);

            if (opcodeMap.find(mnemonic) == opcodeMap.end()) {
                throw std::runtime_error("Unknown opcode: " + mnemonic);
            }

            uint8_t opcode   = opcodeMap.at(mnemonic);
            uint32_t operand = 0;

            if (!operandStr.empty()) {
                if (isNumber(operandStr)) {
                    operand = parseNumber(operandStr);
                } else if (labelAddress.count(operandStr) != 0U) {
                    operand = labelAddress[operandStr];
                } else if (dataAddress.count(operandStr) != 0U) {
                    operand = dataAddress[operandStr];
                } else {
                    throw std::runtime_error("Unknown operand label: " + operandStr);
                }
            }

            instructions[textCursor] = {opcode, operand};
            textCursor++;
        } else if (current == Section::Data) {
            auto colonPos = line.find(':');
            if (colonPos == std::string::npos) {
                throw std::runtime_error("Invalid data entry: " + line);
            }

            std::string label    = line.substr(0, colonPos);
            std::string valueStr = line.substr(colonPos + 1);
            trim(label);
            trim(valueStr);

            uint32_t value = 0;

            if (valueStr.size() >= const_5 && valueStr.substr(0, const_5) == ".zero") {
                int count = std::stoi(valueStr.substr(const_5));
                if (count <= 0) {
                    throw std::runtime_error("Invalid .zero count: " + valueStr);
                }

                size_t index       = dataCursor;
                dataAddress[label] = dataStart + index;
                dataCursor += count;
                dataSection.resize(dataSection.size() + count - 1);
            } else if (isNumber(valueStr)) {
                value        = parseNumber(valueStr);
                size_t index = dataCursor;
                if (dataSection.size() <= index) {
                    dataSection.resize(index + 1);
                }
                dataSection[index] = value;
                dataAddress[label] = dataStart + index;
                dataCursor++;
            } else if (valueStr.front() == '"' && valueStr.back() == '"') {
                std::string str = valueStr.substr(1, valueStr.size() - 2);
                std::vector<uint8_t> bytes;

                for (size_t i = 0; i < str.size(); ++i) {
                    if (str[i] == '\\') {
                        if (i + 1 >= str.size()) {
                            throw std::runtime_error("Invalid escape sequence in string: " +
                                                     valueStr);
                        }
                        ++i;
                        switch (str[i]) {
                            case '0':
                                bytes.push_back('\0');
                                break;
                            case 'n':
                                bytes.push_back('\n');
                                break;
                            case 't':
                                bytes.push_back('\t');
                                break;
                            case '\\':
                                bytes.push_back('\\');
                                break;
                            case '"':
                                bytes.push_back('"');
                                break;
                            default:
                                throw std::runtime_error("Unsupported escape sequence: \\" +
                                                         std::string(1, str[i]));
                        }
                    } else {
                        bytes.push_back(static_cast<uint8_t>(str[i]));
                    }
                }

                size_t index = dataCursor;
                if (dataSection.size() < index + bytes.size()) {
                    dataSection.resize(index + bytes.size());
                }
                for (size_t j = 0; j < bytes.size(); ++j) {
                    dataSection[index + j] = bytes[j];
                }
                dataAddress[label] = dataStart + index;
                dataCursor += bytes.size();
            } else if (valueStr.find(',') != std::string::npos) {
                std::vector<std::string> parts;
                std::stringstream sss(valueStr);
                std::string item;
                while (std::getline(sss, item, ',')) {
                    trim(item);
                    parts.push_back(item);
                }

                size_t index       = dataCursor;
                dataAddress[label] = dataStart + index;

                for (const auto& part : parts) {
                    uint32_t value = 0;
                    if (isNumber(part)) {
                        value = parseNumber(part);
                    } else if (labelAddress.count(part) != 0U) {
                        value = labelAddress[part];
                    } else if (dataAddress.count(part) != 0U) {
                        value = dataAddress[part];
                    } else {
                        throw std::runtime_error("Unknown array value: " + part);
                    }

                    if (dataSection.size() <= dataCursor) {
                        dataSection.resize(dataCursor + 1);
                    }
                    dataSection[dataCursor++] = value;
                }
            } else if (labelAddress.count(valueStr) != 0U) {
                value        = labelAddress[valueStr];
                size_t index = dataCursor;
                if (dataSection.size() <= index) {
                    dataSection.resize(index + 1);
                }
                dataSection[index] = value;
                dataAddress[label] = dataStart + index;
                dataCursor++;
            } else if (dataAddress.count(valueStr) != 0U) {
                value        = dataAddress[valueStr];
                size_t index = dataCursor;
                if (dataSection.size() <= index) {
                    dataSection.resize(index + 1);
                }
                dataSection[index] = value;
                dataAddress[label] = dataStart + index;
                dataCursor++;
            } else {
                throw std::runtime_error("Unknown data value: " + valueStr);
            }
        }
    }

    if (labelAddress.count("_start") == 0U) {
        throw std::runtime_error("Missing _start label");
    }

    if (instructions.empty()) {
        instructions.resize(1, {0, 0});
    }
}

void Binarizer::writeToFile(const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open output file " + filename);
    }

    if (labelAddress.find("_start") == labelAddress.end()) {
        throw std::runtime_error("Unable to find _start label");
    }

    auto codeSize = static_cast<uint32_t>(textStart + instructions.size());
    auto dataSize = static_cast<uint32_t>(dataSection.size());

    out.put(static_cast<char>((codeSize >> SHIFT_24) & MASK_8));
    out.put(static_cast<char>((codeSize >> SHIFT_16) & MASK_8));
    out.put(static_cast<char>((codeSize >> SHIFT_8) & MASK_8));
    out.put(static_cast<char>((codeSize)&MASK_8));

    out.put(static_cast<char>((dataSize >> SHIFT_24) & MASK_8));
    out.put(static_cast<char>((dataSize >> SHIFT_16) & MASK_8));
    out.put(static_cast<char>((dataSize >> SHIFT_8) & MASK_8));
    out.put(static_cast<char>(dataSize & MASK_8));

    size_t memSize = codeSize + dataSize;
    std::vector<uint32_t> mem(memSize, 0);

    uint32_t startAddr = labelAddress.at("_start");
    uint8_t jmpOpcode  = opcodeMap.at("jmp");

    mem[0] = (static_cast<uint32_t>(jmpOpcode) << SHIFT_24) | (startAddr & MASK_24);

    for (size_t i = 0; i < instructions.size(); i++) {
        uint32_t raw = (instructions[i].opcode << SHIFT_24) | (instructions[i].operand & MASK_24);
        mem[textStart + i] = raw;
    }

    for (size_t i = 0; i < dataSection.size(); i++) {
        uint32_t value     = dataSection[i];
        mem[dataStart + i] = value;
    }

    for (uint32_t val : mem) {
        out.put(static_cast<char>((val >> SHIFT_24) & MASK_8));
        out.put(static_cast<char>((val >> SHIFT_16) & MASK_8));
        out.put(static_cast<char>((val >> SHIFT_8) & MASK_8));
        out.put(static_cast<char>(val & MASK_8));
    }
}