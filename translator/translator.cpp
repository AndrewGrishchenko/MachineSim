#include <fstream>
#include <optional>

#include "ASTNode.hpp"
#include "binarizer.h"
#include "codeGenerator.h"
#include "semanticAnalyzer.h"
#include "treeGen.h"

struct Args {
    bool isHighLevel = true;
    std::optional<std::string> vizFile;
    std::string inputFile;
    std::string outputFile;
};

Args parseArgs(const std::vector<std::string>& argsVec) {
    size_t argc = argsVec.size();

    if (argc < 3) {
        throw std::runtime_error("Usage: ./translator [--asm|--hl] [--viz file] <input> <output>");
    }

    Args args;
    int counter = 1;

    while (counter < argc - 2) {
        const std::string& flag = argsVec[counter];

        if (flag == "--asm") {
            args.isHighLevel = false;
            counter++;
        } else if (flag == "--hl") {
            args.isHighLevel = true;
            counter++;
        } else if (flag == "--viz") {
            if (counter + 1 >= argc - 2) {
                throw std::runtime_error("--viz requires a filename");
            }
            args.vizFile = argsVec[counter + 1];
            counter += 2;
        } else {
            throw std::runtime_error("Unknown flag: " + flag);
        }
    }

    args.inputFile  = argsVec[argc - 2];
    args.outputFile = argsVec[argc - 1];

    return args;
}

int main(int argc, char* argv[]) {
    try {
        const std::vector<std::string> argsVec(argv, argv + argc);

        Args args = parseArgs(argsVec);
        std::string code;

        if (args.isHighLevel) {
            std::ifstream file(args.inputFile);
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string data = buffer.str();

            TreeGenerator treeGenerator;
            std::unique_ptr<ASTNode> tree = treeGenerator.makeTree(data);

            SemanticAnalyzer semanticAnalyzer;
            semanticAnalyzer.analyze(tree.get());
            std::cout << "Semantic analyze success\n";

            if (args.vizFile) {
                // TreeVisualizer treeViz;
                // std::string uml = treeViz.makeUML(tree);
                // std::ofstream uml_file(*args.vizFile);
                // uml_file << uml;
                // std::cout << "PlantUML visualize saved to " << *args.vizFile << "\n";
            }

            CodeGenerator codeGenerator;
            code = codeGenerator.generateCode(tree.get());
        } else {
            std::ifstream asm_file(args.inputFile);
            std::stringstream buffer;
            buffer << asm_file.rdbuf();
            code = buffer.str();
        }

        Binarizer binarizer;
        binarizer.parse(code);
        binarizer.writeToFile(args.outputFile);

        std::cout << "Binary program saved to " << args.outputFile << "\n";
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
    }
}