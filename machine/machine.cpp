#include "configParser.hpp"
#include "processorModel.h"

int main(int argc, char* argv[]) {
    const std::vector<std::string> args(argv, argv + argc);

    try {
        if (argc < 3) {
            throw std::runtime_error("Usage: ./machine <config> <binary>");
        }

        MachineConfig cfg = parseConfig(args[1]);
        ProcessorModel processorModel(cfg);
        processorModel.loadBinary(args[2]);
        processorModel.process();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
    }
}