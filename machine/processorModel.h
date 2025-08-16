#ifndef _PROCESSOR_MODEL_H
#define _PROCESSOR_MODEL_H

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "configParser.hpp"

constexpr size_t MEM_SIZE       = 1 << 24;
constexpr uint32_t FULL_MASK    = 0xFFFFFFFF;
constexpr uint32_t MSB_MASK     = 0x80000000;
constexpr uint32_t FULL_MASK_24 = 0xFFFFFF;
constexpr uint32_t FULL_MASK_8  = 0xFF;

constexpr uint32_t MSB_INDEX = 31;
constexpr uint32_t WORD_BITS = 32;
constexpr uint32_t BITS_24   = 24;
constexpr uint32_t BITS_16   = 16;
constexpr uint32_t BITS_8    = 8;

constexpr uint8_t latchAC_index  = 0;
constexpr uint8_t latchAR_index  = 1;
constexpr uint8_t latchDR_index  = 2;
constexpr uint8_t latchPC_index  = 3;
constexpr uint8_t latchSP_index  = 4;
constexpr uint8_t latchSPC_index = 5;

class Memory {
public:
    Memory() {
        reset();
        data.resize(MEM_SIZE);
    }

    void reset() {
        data.clear();
    }

    void write(size_t address, uint32_t value) {
        if (address >= MEM_SIZE) {
            throw std::out_of_range("Memory write out of bounds");
        }
        data.at(address) = value;
    }

    [[nodiscard]] uint32_t& at(size_t address) {
        if (address >= MEM_SIZE) {
            throw std::runtime_error("Memory access out of bounds");
        }
        return data.at(address);
    }

    [[nodiscard]] const uint32_t& at(size_t address) const {
        if (address >= MEM_SIZE) {
            throw std::runtime_error("Memory access out of bounds");
        }
        return data.at(address);
    }

    [[nodiscard]] uint32_t& operator[](size_t address) {
        return at(address);
    }

    [[nodiscard]] const uint32_t& operator[](size_t address) const {
        return at(address);
    }

    std::function<uint32_t&()> makeGetterAtRef(const size_t& addressRef) {
        return [this, &addressRef]() -> uint32_t& { return this->at(addressRef); };
    }

    std::function<uint32_t&()> makeDynamicGetter(const std::function<uint32_t&()>& addressGetter) {
        return [this, addressGetter]() -> uint32_t& { return this->at(addressGetter()); };
    }

private:
    std::vector<uint32_t> data;
};

struct FlagsRegister {
    bool N = false;
    bool Z = false;
    bool V = false;
    bool C = false;

    void reset() {
        N = Z = V = C = false;
    }
};

class ALU {
public:
    ALU() = default;

    enum class Operation : uint8_t {
        ADD,
        SUB,
        MUL,
        DIV,
        REM,
        INC,
        DEC,
        NOT,
        AND,
        OR,
        XOR,
        SHL,
        SHR,
        NOP
    };

    static std::string opStr(Operation operation) {
        switch (operation) {
            case Operation::ADD:
                return "ADD";
            case Operation::SUB:
                return "SUB";
            case Operation::MUL:
                return "MUL";
            case Operation::DIV:
                return "DIV";
            case Operation::REM:
                return "REM";
            case Operation::INC:
                return "INC";
            case Operation::DEC:
                return "DEC";
            case Operation::NOT:
                return "NOT";
            case Operation::AND:
                return "AND";
            case Operation::OR:
                return "OR";
            case Operation::XOR:
                return "XOR";
            case Operation::SHL:
                return "SHL";
            case Operation::SHR:
                return "SHR";
            case Operation::NOP:
                return "NOP";
            default:
                return "unknown";
        }
    }

    void setLeftInputGetter(std::function<uint32_t&()> getter) {
        leftGetter = std::move(getter);
    }

    void setRightInputGetter(std::function<uint32_t&()> getter) {
        rightGetter = std::move(getter);
    }

    void setOperation(Operation operation) {
        this->operation = operation;
    }

    void setWriteFlags(bool writeFlags) {
        this->writeFlags = writeFlags;
    }

    void connectFlags(const std::shared_ptr<FlagsRegister>& flags) {
        this->flagsWeakPtr = flags;
    }

    void perform() {
        uint32_t left  = leftGetter();
        uint32_t right = rightGetter();
        uint32_t value = 0;
        bool NFlag     = false;
        bool ZFlag     = false;
        bool VFlag     = false;
        bool CFlag     = false;

        switch (operation) {
            case Operation::ADD: {
                uint64_t tmp = static_cast<uint64_t>(left) + right;
                value        = tmp & FULL_MASK;
                CFlag        = tmp > FULL_MASK;
                VFlag        = (((left ^ value) & (right ^ value)) & MSB_MASK) != 0;
                break;
            }
            case Operation::SUB: {
                uint64_t tmp = static_cast<uint64_t>(left) - right;
                value        = tmp & FULL_MASK;
                CFlag        = left >= right;
                VFlag        = (((left ^ right) & (left ^ value)) & MSB_MASK) != 0;
                break;
            }
            case Operation::MUL:
                value = left * right;
                CFlag = false;
                VFlag = false;
                break;
            case Operation::DIV:
                value = right != 0U ? left / right : 0;
                break;
            case Operation::REM:
                value = right != 0U ? left % right : 0;
                break;
            case Operation::INC:
                value = left + right + 1;
                break;
            case Operation::DEC:
                value = left + right - 1;
                break;
            case Operation::NOT:
                value = ~(left + right);
                break;
            case Operation::AND:
                value = left & right;
                break;
            case Operation::OR:
                value = left | right;
                break;
            case Operation::XOR:
                value = left ^ right;
                break;
            case Operation::SHL:
                value = left << right;
                CFlag = ((left >> (WORD_BITS - right)) & 1U) != 0U;
                break;
            case Operation::SHR:
                value = left >> right;
                CFlag = ((left >> (right - 1)) & 1U) != 0U;
                break;
            case Operation::NOP:
                value = left + right;
                break;
        }

        NFlag = (value >> MSB_INDEX) != 0U;
        ZFlag = value == 0;

        result = value;
        if (writeFlags) {
            if (auto flags = flagsWeakPtr.lock()) {
                flags->N = NFlag;
                flags->Z = ZFlag;
                flags->V = VFlag;
                flags->C = CFlag;
            }
        }
    }

    [[nodiscard]] uint32_t getResult() const {
        return result;
    }

    uint32_t& getResultRef() {
        return result;
    }

private:
    std::function<uint32_t&()> leftGetter;
    std::function<uint32_t&()> rightGetter;

    Operation operation = Operation::NOP;
    uint32_t result     = 0;
    std::weak_ptr<FlagsRegister> flagsWeakPtr{};  // NOLINT(readability-redundant-member-init)
    bool writeFlags = false;
};

class MUX {
public:
    MUX() = default;

    void addInput(uint32_t& value) {
        inputs.emplace_back(value);
    }

    void replaceInput(size_t index, uint32_t& value) {
        if (index >= inputs.size()) {
            throw std::out_of_range("MUX replace input out of range");
        }
        inputs[index] = value;
    }

    void select(size_t index) {
        if (index >= inputs.size()) {
            throw std::out_of_range("MUX select out of range");
        }
        selectedIndex = index;
    }

    [[nodiscard]] uint32_t& getSelected() const {
        return inputs[selectedIndex].get();
    }

    std::function<uint32_t&()> makeGetter() {
        return [this]() -> uint32_t& { return this->getSelected(); };
    }

private:
    std::vector<std::reference_wrapper<uint32_t>> inputs;
    size_t selectedIndex = 0;
};

class Registers {
public:
    Registers() {
        flags = std::make_shared<FlagsRegister>();
        reset();
    }

    enum RegName : uint8_t { ACC, IR, AR, DR, IP, SP, REG_COUNT };

    void reset() {
        regs.fill(0);
        regs[SP] = FULL_MASK_24;
        flags->reset();
    }

    [[nodiscard]] uint32_t get(RegName reg) const {
        if (reg >= REG_COUNT) {
            throw std::out_of_range("Invalid register");
        }
        return regs.at(reg);
    }

    uint32_t& getRef(RegName reg) {
        if (reg >= REG_COUNT) {
            throw std::out_of_range("Invalid register");
        }
        return regs.at(reg);
    }

    void set(RegName reg, uint32_t value) {
        if (reg >= REG_COUNT) {
            throw std::out_of_range("Invalid register");
        }
        regs.at(reg) = value;
    }

    [[nodiscard]] std::shared_ptr<FlagsRegister> getFlags() const {
        return flags;
    }

private:
    std::array<uint32_t, REG_COUNT> regs{};
    std::shared_ptr<FlagsRegister> flags{};  // NOLINT(readability-redundant-member-init)
};

class Latch {
public:
    Latch() {
        sourceGetter = [this]() -> uint32_t& { return source.get(); };
        targetGetter = [this]() -> uint32_t& { return this->target.get(); };
    }

    void setSource(std::reference_wrapper<uint32_t> source) {
        this->source = source;
        sourceGetter = [this]() -> uint32_t& { return this->source.get(); };
    }

    void setSourceGetter(std::function<uint32_t&()> sourceGetter) {
        this->sourceGetter = std::move(sourceGetter);
    }

    void setTarget(std::reference_wrapper<uint32_t> target) {
        this->target = target;
        targetGetter = [this]() -> uint32_t& { return this->target.get(); };
    }

    void setTargetGetter(std::function<uint32_t&()> targetGetter) {
        this->targetGetter = std::move(targetGetter);
    }

    void setEnabled(bool enabled) {
        this->enabled = enabled;
    }

    void propagate() {
        if (enabled) {
            targetGetter() = sourceGetter();
        }
    }

private:
    std::reference_wrapper<uint32_t> source = dummyInput;
    std::reference_wrapper<uint32_t> target = dummyInput;

    std::function<uint32_t&()> sourceGetter;
    std::function<uint32_t&()> targetGetter;

    bool enabled        = false;
    uint32_t dummyInput = 0;
};

class LatchRouter {
public:
    LatchRouter() = default;

    void addLatch(Latch& latch) {
        latches.emplace_back(latch);
    }

    template <typename... Latches>
    void setLatches(Latches&... latches) {
        this->latches = {std::ref(latches)...};
    }

    void setLatchState(size_t index, int enabled) {
        if (index >= latches.size()) {
            throw std::invalid_argument("Latch index out of range");
        }
        latches[index].get().setEnabled(enabled == 1);
    }

    void setLatchStates(const std::vector<int>& latchStates) {
        if (latchStates.size() != latches.size()) {
            throw std::invalid_argument("Latch state count doesn't match output count");
        }

        for (size_t i = 0; i < latches.size(); i++) {
            latches[i].get().setEnabled(latchStates[i] == 1);
        }
    }

    void propagate() {
        for (auto& latchRef : latches) {
            latchRef.get().propagate();
        }
    }

private:
    std::vector<std::reference_wrapper<Latch>> latches;
};

class InterruptHandler {
public:
    InterruptHandler() = default;

    void connect(Latch& latchALU_SPC, Latch& latchSPC_PC, Latch& latchVec_PC) {
        this->latchALU_SPC = &latchALU_SPC;
        this->latchSPC_PC  = &latchSPC_PC;
        this->latchVec_PC  = &latchVec_PC;

        this->latchVec_PC->setSource(inputVec);
    }

    enum class IRQType : uint8_t { NONE = 0, IO_INPUT = 1 };

    void setIRQ(IRQType irq) {
        if (!ipc) {
            this->irq = irq;
        }
    }

    bool& getIERef() {
        return ie;
    }
    bool& getIPCRef() {
        return ipc;
    }

    uint32_t& getSPCRef() {
        return SPC;
    }

    void setVectorTable(uint32_t defaultVec, uint32_t inputVec) {
        this->defaultVec = defaultVec;
        this->inputVec   = inputVec;
    }

    [[nodiscard]] bool shouldInterrupt() const {
        return ie && irq != IRQType::NONE && !ipc;
    }

    bool isEnteringInterrupt() {
        return intState == InterruptState::Executing;
    }

    void step();

private:
    IRQType irq = IRQType::NONE;
    bool ie     = false;
    bool ipc    = false;

    uint32_t dummyInput                 = 0;
    std::reference_wrapper<uint32_t> PC = dummyInput;

    uint32_t defaultVec = 0;
    uint32_t inputVec   = 0;

    Latch* latchALU_SPC = nullptr;
    Latch* latchSPC_PC  = nullptr;
    Latch* latchVec_PC  = nullptr;
    uint32_t SPC        = 0;

    enum class InterruptState : uint8_t { SavingPC, Executing, Restoring };
    InterruptState intState = InterruptState::SavingPC;
};

class IOSimulator {
public:
    IOSimulator() = default;

    void connect(InterruptHandler& interruptHandler, Memory& memory) {
        this->interruptHandler = &interruptHandler;
        this->memory           = &memory;
    }

    void connectOutput(std::ofstream& outputFile) {
        this->outputFile = &outputFile;
    }

    struct IOScheduleEntry {
        size_t tick;
        int token;
    };

    void addInput(IOScheduleEntry entry) {
        inputSchedule.push_back(entry);
    }

    void output(const std::string& data) {
        if (outputFile != nullptr) {
            (*outputFile) << data;
        }
    }

    void output(char character) {
        if (outputFile != nullptr) {
            (*outputFile) << character;
        }
    }

    void check(size_t tick) {
        for (const auto& entry : inputSchedule) {
            if (entry.tick == tick) {
                interruptHandler->setIRQ(InterruptHandler::IRQType::IO_INPUT);
                memory->write(input_address, entry.token);
            }
        }

        if ((*memory)[output_address] != 0x0) {
            uint32_t token = (*memory).at(output_address);
            outputSchedule.push_back({tick, static_cast<char>(token)});
            output(static_cast<char>(token));
            memory->write(output_address, 0);
        }
    }

    std::string getTokenOutput() {
        std::ostringstream data;
        data << "[";
        for (size_t i = 0; i < outputSchedule.size(); i++) {
            data << "(" << outputSchedule[i].tick << ", '";
            char token = static_cast<char>(outputSchedule[i].token);
            if (token == '\n') {
                data << "\\n";
            } else if (token == '\t') {
                data << "\\t";
            } else {
                data << token;
            }
            data << "')";
            if (i < outputSchedule.size() - 1) {
                data << ", ";
            }
        }
        data << "]";
        return data.str();
    }

private:
    InterruptHandler* interruptHandler = nullptr;
    Memory* memory                     = nullptr;

    std::vector<IOScheduleEntry> inputSchedule;
    std::vector<IOScheduleEntry> outputSchedule;

    static constexpr size_t input_address  = 0x10;
    static constexpr size_t output_address = 0x11;

    std::ofstream* outputFile = nullptr;
};

class CU {
public:
    CU() = default;

    void connect(InterruptHandler& interruptHandler, MUX& mux1, MUX& mux2, ALU& alu,
                 LatchRouter& latchRouter, Latch& latchMEM_IR, Latch& latchMEM_DR,
                 Latch& latchDR_MEM) {
        this->interruptHandler = &interruptHandler;
        this->mux1             = &mux1;
        this->mux2             = &mux2;
        this->alu              = &alu;
        this->latchRouter      = &latchRouter;
        this->latchMEM_IR      = &latchMEM_IR;
        this->latchMEM_DR      = &latchMEM_DR;
        this->latchDR_MEM      = &latchDR_MEM;

        this->mux1->replaceInput(2, operand);
    }

    void setLog(std::string& logChunk) {
        this->logChunk = &logChunk;
    }

    void setIRInput(const uint32_t& IRreg) {
        this->IR = &IRreg;
    }

    void connectFlags(const std::shared_ptr<FlagsRegister>& flags) {
        this->flagsWeakPtr = flags;
    }

    [[nodiscard]] bool isHalted() const {
        return halted;
    }

    void decode();

    // clang-format off
    enum Opcode : uint8_t {
        OP_ADD  = 0b000001,
        OP_SUB  = 0b000010,
        OP_DIV  = 0b000011,
        OP_MUL  = 0b000100,
        OP_REM  = 0b000101,
        OP_INC  = 0b000110,
        OP_DEC  = 0b000111,
        OP_NOT  = 0b001000,
        OP_CLA  = 0b001001,
        OP_JMP  = 0b001010,
        OP_CMP  = 0b001011,
        OP_JZ   = 0b001100,
        OP_JNZ  = 0b001101,
        OP_JG   = 0b001110,
        OP_JGE  = 0b001111,
        OP_JL   = 0b010000,
        OP_JLE  = 0b010001,
        OP_JA   = 0b010010,
        OP_JAE  = 0b010011,
        OP_JB   = 0b010100,
        OP_JBE  = 0b010101,
        OP_PUSH = 0b010110,
        OP_POP  = 0b010111,
        OP_LD   = 0b011000,
        OP_LDA  = 0b011001,
        OP_LDI  = 0b011010,
        OP_ST   = 0b011011,
        OP_STA  = 0b011100,
        OP_CALL = 0b011101,
        OP_RET  = 0b011110,
        OP_EI   = 0b011111,
        OP_DI   = 0b100000,
        OP_IRET = 0b100001,
        OP_HALT = 0b100010
    };

    static std::string opcodeStr (uint8_t code) {
        auto opcode = static_cast<Opcode>(code);
        switch (opcode) {
            case OP_ADD:  return "add";
            case OP_SUB:  return "sub";
            case OP_DIV:  return "div";
            case OP_MUL:  return "mul";
            case OP_REM:  return "rem";
            case OP_INC:  return "inc";
            case OP_DEC:  return "dec";
            case OP_NOT:  return "not";
            case OP_CLA:  return "cla";
            case OP_JMP:  return "jmp";
            case OP_CMP:  return "cmp";
            case OP_JZ:   return "jz";
            case OP_JNZ:  return "jnz";
            case OP_JG:   return "jg";
            case OP_JGE:  return "jge";
            case OP_JL:   return "jl";
            case OP_JLE:  return "jle";
            case OP_JA:   return "ja";
            case OP_JAE:  return "jae";
            case OP_JB:   return "jb";
            case OP_JBE:  return "jbe";
            case OP_PUSH: return "push";
            case OP_POP:  return "pop";
            case OP_LD:   return "ld";
            case OP_LDA:  return "lda";
            case OP_LDI:  return "ldi";
            case OP_ST:   return "st";
            case OP_STA:  return "sta";
            case OP_CALL: return "call";
            case OP_RET:  return "ret";
            case OP_EI:   return "ei";
            case OP_DI:   return "di";
            case OP_IRET: return "iret";
            case OP_HALT: return "halt";
            default:      return "unknown"; 
        }
    }
    // clang-format on

    static bool hasOperand(uint8_t code) {
        auto opcode = static_cast<Opcode>(code);
        switch (opcode) {
            case OP_INC:
            case OP_DEC:
            case OP_NOT:
            case OP_CLA:
            case OP_PUSH:
            case OP_POP:
            case OP_RET:
            case OP_EI:
            case OP_DI:
            case OP_IRET:
            case OP_HALT:
                return false;

            default:
                return true;
        }
    }

private:
    InterruptHandler* interruptHandler = nullptr;

    MUX* mux1                = nullptr;
    MUX* mux2                = nullptr;
    ALU* alu                 = nullptr;
    LatchRouter* latchRouter = nullptr;
    Latch* latchMEM_IR       = nullptr;
    Latch* latchMEM_DR       = nullptr;
    Latch* latchDR_MEM       = nullptr;

    const uint32_t* IR = nullptr;
    std::weak_ptr<FlagsRegister> flagsWeakPtr;

    enum class CPUState : uint8_t { FetchAR, FetchIR, Decode, IncrementIP, Halt };

    std::string stateStr() {
        switch (state) {
            case CPUState::FetchAR:
                return "FetchAR";
            case CPUState::FetchIR:
                return "FetchIR";
            case CPUState::Decode:
                return "Decode";
            case CPUState::IncrementIP:
                return "IncrementIP";
            case CPUState::Halt:
                return "Halt";
            default:
                return "";
        }
    }

    CPUState state = CPUState::FetchAR;

    bool instructionDone = false;
    void instructionTick();

    size_t microstep = 0;

    uint8_t opcode   = 0;
    uint32_t operand = 0;

    bool halted = false;

    std::string* logChunk = nullptr;
    void log(const std::string& line) {
        if (logChunk != nullptr) {
            (*logChunk) += line + "\n";
        }
    }
};

class IncrementalFNV1a {
public:
    IncrementalFNV1a() : hash_(FNV_offset_basis) {
    }

    void update(const void* data, size_t len) {
        const auto* bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < len; ++i) {
            hash_ ^= bytes[i];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            hash_ *= FNV_prime;
        }
    }

    [[nodiscard]] uint64_t final() const {
        return hash_;
    }

private:
    static constexpr uint64_t FNV_offset_basis = 14695981039346656037ULL;
    static constexpr uint64_t FNV_prime        = 1099511628211ULL;

    uint64_t hash_;
};

class ProcessorModel {
public:
    ProcessorModel(MachineConfig& cfg);

    void loadBinary(const std::string& filename);
    void process();

private:
    MachineConfig cfg;

    size_t textSize     = 0;
    size_t dataStart    = 0;
    size_t dataSize     = 0;
    uint32_t entryPoint = 0;

    uint32_t zero    = 0;
    size_t tickCount = 0;

    bool halted       = false;
    bool binaryLoaded = false;

    Memory memory;
    Registers registers;
    ALU alu;
    MUX mux1, mux2;

    Latch latchALU_DR, latchALU_AR, latchALU_SP, latchALU_AC, latchALU_PC, latchALU_SPC;
    LatchRouter latchRouter;
    Latch latchMEM_IR, latchMEM_DR, latchDR_MEM;
    Latch latchSPC_PC;
    Latch latchVec_PC;

    CU cu;
    InterruptHandler interruptHandler;

    IOSimulator iosim;

    void tick();

    std::string memDump();
    std::string registerDump();

    static bool isNumberArray(const std::string& val);
    static std::vector<int> parseStreamLine(const std::string& line);
    static std::vector<int> parseTokenStr(const std::string& tokenStr);
    void parseInput();

    static uint32_t read_uint32(std::ifstream& inFile);

    std::string logChunk;

    std::ofstream logFile;
    std::ofstream outputFile;
    std::ofstream binaryReprFile;
    std::ofstream logHashFile;

    IncrementalFNV1a hasher;
};

#endif