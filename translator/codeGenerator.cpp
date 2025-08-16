#include "codeGenerator.h"

std::string CodeGenerator::generateCode(ASTNode* root) {
    if (root == nullptr || root->nodeType != ASTNodeType::Block) {
        throw std::runtime_error("Root node must be block");
    }

    dataSection.clear();
    codeSection.clear();
    funcSection.clear();
    variables.clear();
    functionLabels.clear();
    functions.clear();

    labelCounter = 0;
    strCounter   = 0;
    arrCounter   = 0;

    emitCodeLabel("_start");
    root->accept(*this);
    emitCode("halt");

    return assembleCode();
}

void CodeGenerator::visit(VarDeclNode& node) {
    std::string varLabel = getVarLabel(node.name);

    if (node.type == "int[]") {
        emitData(varLabel + ": 0");
        variables[varLabel] = node.type;

        node.value->accept(*this);

        emitCode("st " + varLabel);
    } else {
        emitData(varLabel + ": 0");
        variables[varLabel] = node.type;
        node.value->accept(*this);
        emitCode("st " + varLabel);
    }
}

void CodeGenerator::visit(NumberLiteralNode& node) {
    if (node.number > FULL_MASK_24) {
        std::string constLabel = "const_" + std::to_string(node.number);
        emitData(constLabel + ": " + std::to_string(node.number));
        emitCode("ld " + constLabel);
    } else {
        emitCode("ldi " + std::to_string(node.number));
    }
}

void CodeGenerator::visit(CharLiteralNode& node) {
    int char_code = static_cast<int>(node.value);

    emitCode("ldi " + std::to_string(char_code));
}

void CodeGenerator::visit(StringLiteralNode& node) {
    std::string strLabel = "str_" + std::to_string(strCounter++);

    emitData(strLabel + ": \"" + node.value + "\\0\"");
    emitCode("ldi " + strLabel);
}

void CodeGenerator::visit(BooleanLiteralNode& node) {
    emitCode("ldi " + std::to_string(node.value ? 1 : 0));
}

void CodeGenerator::visit(VoidLiteralNode& node) {
}

void CodeGenerator::visit(IntArrayLiteralNode& node) {
    std::string arrLabel = "arr_" + std::to_string(arrCounter++);

    std::string dataLine = arrLabel + ": ";
    for (size_t i = 0; i < node.values.size(); i++) {
        auto* numberNode = dynamic_cast<NumberLiteralNode*>(node.values[i].get());
        dataLine += std::to_string(numberNode->number) + (i == node.values.size() - 1 ? "" : ", ");
    }
    emitData(dataLine);

    emitCode("ldi " + arrLabel);
}

void CodeGenerator::visit(ArrayGetNode& node) {
    node.object->accept(*this);
    emitCode("push");
    node.index->accept(*this);

    emitCode("st temp_right");
    emitCode("pop");
    emitCode("add temp_right");
    emitCode("st temp_right");
    emitCode("lda temp_right");
}

void CodeGenerator::visit(MethodCallNode& node) {
    if (node.object->nodeType != ASTNodeType::Identifier) {
        throw std::runtime_error("method call on complex expressions not supported");
    }

    auto* objIdentifier = dynamic_cast<IdentifierNode*>(node.object.get());

    std::string varLabel = getVarLabel(objIdentifier->name);

    if (node.methodName == "size") {
        node.object->accept(*this);

        emitCode("call arr_size");
    }
}

void CodeGenerator::visit(IdentifierNode& node) {
    std::string varLabel = getVarLabel(node.name);
    std::string varType  = variables.at(varLabel);

    if (varType == "int[]") {
        emitCode("ld " + varLabel);
    } else {
        emitCode("ld " + varLabel);
    }
}

void CodeGenerator::visit(AssignNode& node) {
    node.var2->accept(*this);

    ASTNode* lhs = node.var1.get();

    if (lhs->nodeType == ASTNodeType::Identifier) {
        auto* identifier     = dynamic_cast<IdentifierNode*>(lhs);
        std::string varLabel = getVarLabel(identifier->name);
        emitCode("st " + varLabel);
    } else if (lhs->nodeType == ASTNodeType::ArrayGet) {
        auto* arrayGet = dynamic_cast<ArrayGetNode*>(lhs);
        emitCode("push");

        arrayGet->index->accept(*this);
        emitCode("st temp_right");

        arrayGet->object->accept(*this);
        emitCode("add temp_right");
        emitCode("st temp_right");

        emitCode("pop");
        emitCode("sta temp_right");
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void CodeGenerator::visit(BinaryOpNode& node) {
    const std::string& opr = node.op;
    bool isLogicalOp = (opr == ">" || opr == "<" || opr == ">=" || opr == "<=" || opr == "==" ||
                        opr == "!=" || opr == "&&" || opr == "||");

    std::string leftType  = dynamic_cast<ExpressionNode*>(node.left.get())->resolvedType;
    std::string rightType = dynamic_cast<ExpressionNode*>(node.right.get())->resolvedType;

    if (isLogicalOp && !currentTrueLabel.empty() && !currentFalseLabel.empty()) {
        if (opr == "&&") {
            std::string rightSideLabel = getNewLabel();
            visitWithLabels(node.left.get(), rightSideLabel, currentFalseLabel);

            emitCodeLabel(rightSideLabel);
            visitWithLabels(node.right.get(), currentTrueLabel, currentFalseLabel);

            return;
        }

        if (opr == "||") {
            std::string rightSideLabel = getNewLabel();
            visitWithLabels(node.left.get(), currentTrueLabel, rightSideLabel);

            emitCodeLabel(rightSideLabel);
            visitWithLabels(node.right.get(), currentTrueLabel, currentFalseLabel);

            return;
        }

        node.left->accept(*this);
        emitCode("push");

        node.right->accept(*this);
        emitCode("st temp_right");
        emitCode("pop");
        emitCode("sub temp_right");

        if (opr == "==") {
            emitCode("jz " + currentTrueLabel);
        } else if (opr == "!=") {
            emitCode("jnz " + currentTrueLabel);
        } else if (opr == ">") {
            emitCode("jg " + currentTrueLabel);
        } else if (opr == ">=") {
            emitCode("jge " + currentTrueLabel);
        } else if (opr == "<") {
            emitCode("jl " + currentTrueLabel);
        } else if (opr == "<=") {
            emitCode("jle " + currentTrueLabel);
        }

        emitCode("jmp " + currentFalseLabel);
    } else {
        node.left->accept(*this);
        emitCode("push");

        node.right->accept(*this);
        emitCode("st temp_right");
        emitCode("pop");

        if (opr == "+") {
            emitCode("add temp_right");
        } else if (opr == "-") {
            emitCode("sub temp_right");
        } else if (opr == "*") {
            emitCode("mul temp_right");
        } else if (opr == "/") {
            emitCode("div temp_right");
        } else if (opr == "%") {
            emitCode("rem temp_right");
        }

        else if (opr == "&&") {
            emitCode("mul temp_right");
        } else if (opr == "||") {
            emitCode("add temp_right");

            std::string falseLabel = getNewLabel();
            std::string endLabel   = getNewLabel();

            emitCode("jz " + falseLabel);
            emitCode("ldi 1");
            emitCode("jmp " + endLabel);
            emitCodeLabel(falseLabel);
            emitCode("ldi 0");
            emitCodeLabel(endLabel);
        } else if (opr == ">" || opr == "<" || opr == ">=" || opr == "<=" || opr == "==" ||
                   opr == "!=") {
            emitCode("cmp temp_right");

            bool isUnsignedCmp = (leftType == "uint" || rightType == "uint");

            std::string trueLabel = getNewLabel();
            std::string endLabel  = getNewLabel();

            if (isUnsignedCmp) {
                if (opr == "==") {
                    emitCode("jz " + trueLabel);
                } else if (opr == "!=") {
                    emitCode("jnz " + trueLabel);
                } else if (opr == ">") {
                    emitCode("ja " + trueLabel);
                } else if (opr == ">=") {
                    emitCode("jae " + trueLabel);
                } else if (opr == "<") {
                    emitCode("jb " + trueLabel);
                } else if (opr == "<=") {
                    emitCode("jbe " + trueLabel);
                }
            } else {
                if (opr == "==") {
                    emitCode("jz " + trueLabel);
                } else if (opr == "!=") {
                    emitCode("jnz " + trueLabel);
                } else if (opr == ">") {
                    emitCode("jg " + trueLabel);
                } else if (opr == ">=") {
                    emitCode("jge " + trueLabel);
                } else if (opr == "<") {
                    emitCode("jl " + trueLabel);
                } else if (opr == "<=") {
                    emitCode("jle " + trueLabel);
                }
            }

            emitCode("ldi 0");
            emitCode("jmp " + endLabel);

            emitCodeLabel(trueLabel);
            emitCode("ldi 1");

            emitCodeLabel(endLabel);
        }
    }
}

void CodeGenerator::visit(UnaryOpNode& node) {
    const std::string& opr = node.op;

    if (opr == "!" && !currentTrueLabel.empty() && !currentFalseLabel.empty()) {
        visitWithLabels(node.operand.get(), currentFalseLabel, currentTrueLabel);
        return;
    }

    node.operand->accept(*this);

    if (opr == "-") {
        emitCode("not");
        emitCode("inc");
    } else if (opr == "!") {
        std::string trueLabel = getNewLabel();
        std::string endLabel  = getNewLabel();

        emitCode("jz " + trueLabel);

        emitCode("ldi 0");
        emitCode("jmp " + endLabel);

        emitCodeLabel(trueLabel);
        emitCode("ldi 1");

        emitCodeLabel(endLabel);
    }
}

void CodeGenerator::visit(IfNode& node) {
    std::string thenLabel = getNewLabel();
    std::string elseLabel = node.elseBranch != nullptr ? getNewLabel() : "";
    std::string endLabel  = getNewLabel();

    this->currentTrueLabel  = thenLabel;
    this->currentFalseLabel = node.elseBranch != nullptr ? elseLabel : endLabel;

    node.condition->accept(*this);

    auto* condNode = node.condition.get();
    if (condNode->nodeType != ASTNodeType::BinaryOp && condNode->nodeType != ASTNodeType::UnaryOp) {
        emitCode("jnz " + this->currentTrueLabel);
        emitCode("jmp " + this->currentFalseLabel);
    }

    this->currentTrueLabel  = "";
    this->currentFalseLabel = "";

    emitCodeLabel(thenLabel);
    node.thenBranch->accept(*this);
    if (node.elseBranch != nullptr) {
        emitCode("jmp " + endLabel);
        emitCodeLabel(elseLabel);
        node.elseBranch->accept(*this);
    }

    emitCodeLabel(endLabel);
}

void CodeGenerator::visit(WhileNode& node) {
    std::string startLabel = getNewLabel();
    std::string bodyLabel  = getNewLabel();
    std::string endLabel   = getNewLabel();

    breakLabels.push_back(endLabel);

    emitCodeLabel(startLabel);

    this->currentTrueLabel  = bodyLabel;
    this->currentFalseLabel = endLabel;

    node.condition->accept(*this);

    this->currentTrueLabel  = "";
    this->currentFalseLabel = "";

    emitCodeLabel(bodyLabel);
    node.body->accept(*this);

    emitCode("jmp " + startLabel);
    emitCodeLabel(endLabel);

    breakLabels.pop_back();
}

void CodeGenerator::visit(BreakNode& node) {
    emitCode("jmp " + breakLabels.back());
}

void CodeGenerator::visit(BlockNode& node) {
    for (const auto& child : node.children) {
        child->accept(*this);
    }
}

void CodeGenerator::visit(ParameterNode& node) {
    throw std::logic_error("visit(ParameterNode&) should not be called in codeGen");
}

void CodeGenerator::visit(FunctionNode& node) {
    std::vector<std::string> paramTypes(node.parameters.size());
    for (size_t i = 0; i < node.parameters.size(); i++) {
        paramTypes[i] = dynamic_cast<ParameterNode*>(node.parameters[i].get())->type;
    }

    std::string mangledLabel = mangleFunctionName(node.name, paramTypes);

    FunctionData funcData;
    funcData.name       = node.name;
    funcData.label      = mangledLabel;
    funcData.returnType = node.returnType;
    funcData.params.resize(node.parameters.size());

    for (size_t i = 0; i < node.parameters.size(); i++) {
        auto* paramNode    = dynamic_cast<ParameterNode*>(node.parameters[i].get());
        funcData.params[i] = {paramNode->type, paramNode->name};

        std::string argLabel = "arg_" + mangledLabel + "_" + paramNode->name;
        emitData(argLabel + ": 0");
        variables[argLabel] = paramNode->type;
    }

    functions[node.name].push_back(funcData);

    auto previousFunction = currentFunction;
    currentFunction       = std::make_shared<FunctionData>(funcData);

    emitCodeLabel(mangledLabel);

    emitCode("pop");
    emitCode("st temp_ret_addr");

    for (int i = (int)currentFunction->params.size() - 1; i >= 0; i--) {
        const auto& param    = currentFunction->params[i];
        std::string argLabel = "arg_" + currentFunction->label + "_" + param.second;
        emitCode("pop");
        emitCode("st " + argLabel);
    }

    node.body->accept(*this);

    emitCode("");

    // TODO: return is a must
    currentFunction = previousFunction;
}

void CodeGenerator::visit(FunctionCallNode& node) {
    if (reservedFunctions.count(node.name) != 0U) {
        processReservedFunctionCall(node);
    } else {
        processRegularFunctionCall(node);
    }
}

void CodeGenerator::visit(ReturnNode& node) {
    if (node.returnValue != nullptr) {
        node.returnValue->accept(*this);
    }

    emitCode("st temp_right");
    emitCode("ld temp_ret_addr");
    emitCode("push");
    emitCode("ld temp_right");
    emitCode("ret");
}

void CodeGenerator::visitWithLabels(ASTNode* node, const std::string& trueL,
                                    const std::string& falseL) {
    std::string oldTrue  = currentTrueLabel;
    std::string oldFalse = currentFalseLabel;

    currentTrueLabel  = trueL;
    currentFalseLabel = falseL;

    node->accept(*this);

    currentTrueLabel  = oldTrue;
    currentFalseLabel = oldFalse;
}

void CodeGenerator::processReservedFunctionCall(FunctionCallNode& node) {
    std::vector<std::string> argTypes(node.parameters.size());
    for (size_t i = 0; i < node.parameters.size(); i++) {
        argTypes[i] = dynamic_cast<ExpressionNode*>(node.parameters[i].get())->resolvedType;
    }
    std::string expectedReturnType = evalType(&node);

    const FunctionSignature* signature =
        findReservedFunction(node.name, argTypes, expectedReturnType);
    if (signature == nullptr) {
        throw std::logic_error("reserved function signature mismatch");
    }

    if (node.name == "in") {
        if (signature->paramTypes.empty()) {
            emitCode("ldi 0");
        } else {
            node.parameters[0]->accept(*this);
        }
        emitCode("st input_count");

        const std::string returnType = node.resolvedType;

        if (returnType == "int" || returnType == "uint") {
            emitCode("call read_int");
        } else if (returnType == "char") {
            emitCode("call read_char");
        } else if (returnType == "string") {
            emitCode("call read_string");
        } else if (returnType == "int[]") {
            emitCode("call read_arr");
        }

    } else if (node.name == "out") {
        auto* arg                      = node.parameters[0].get();
        const std::string& typeToPrint = dynamic_cast<ExpressionNode*>(arg)->resolvedType;
        arg->accept(*this);

        if (typeToPrint == "int") {
            emitCode("call write_int");
        } else if (typeToPrint == "uint") {
            emitCode("call write_uint");
        } else if (typeToPrint == "char") {
            emitCode("call write_char");
        } else if (typeToPrint == "string") {
            emitCode("call write_string");
        } else if (typeToPrint == "int[]") {
            emitCode("call write_arr");
        }
    }
}

void CodeGenerator::processRegularFunctionCall(FunctionCallNode& node) {
    std::vector<std::string> argTypes(node.parameters.size());
    for (size_t i = 0; i < node.parameters.size(); i++) {
        argTypes[i] = dynamic_cast<ExpressionNode*>(node.parameters[i].get())->resolvedType;
    }
    std::string mangledLabelToCall = mangleFunctionName(node.name, argTypes);

    if (currentFunction) {
        emitCode("ld temp_ret_addr");
        emitCode("push");

        for (const auto& param : currentFunction->params) {
            std::string argLabel = "arg_" + currentFunction->label + "_" + param.second;
            emitCode("ld " + argLabel);
            emitCode("push");
        }
    }

    for (const auto& argExpr : node.parameters) {
        argExpr->accept(*this);
        emitCode("push");
    }

    emitCode("call " + mangledLabelToCall);

    if (currentFunction) {
        emitCode("st temp_right");

        for (int i = (int)currentFunction->params.size() - 1; i >= 0; i--) {
            const auto& param    = currentFunction->params[i];
            std::string argLabel = "arg_" + currentFunction->label + "_" + param.second;
            emitCode("pop");
            emitCode("st " + argLabel);
        }

        emitCode("pop");
        emitCode("st temp_ret_addr");

        emitCode("ld temp_right");
    }
}

std::string CodeGenerator::evalType(ASTNode* node) {
    switch (node->nodeType) {
        case ASTNodeType::NumberLiteral:
            return "int";
        case ASTNodeType::CharLiteral:
            return "char";
        case ASTNodeType::StringLiteral:
            return "string";
        case ASTNodeType::BooleanLiteral:
            return "bool";
        case ASTNodeType::VoidLiteral:
            return "void";
        case ASTNodeType::IntArrayLiteral:
            return "int[]";
        case ASTNodeType::ArrayGet:
            return "int";
        case ASTNodeType::Identifier: {
            auto* identifierNode = dynamic_cast<IdentifierNode*>(node);
            return variables[getVarLabel(identifierNode->name)];
        }
        case ASTNodeType::BinaryOp: {
            auto* binaryOpNode = dynamic_cast<BinaryOpNode*>(node);
            std::string opr    = binaryOpNode->op;
            if (opr == "+" || opr == "-" || opr == "*" || opr == "/" || opr == "%") {
                return "int";
            }
            if (opr == "==" || opr == "!=" || opr == ">" || opr == ">=" || opr == "<" ||
                opr == "<=" || opr == "&&" || opr == "||") {
                return "bool";
            }
            throw std::runtime_error("Unknown op " + opr);
        }
        case ASTNodeType::UnaryOp: {
            auto* unaryOpNode = dynamic_cast<UnaryOpNode*>(node);
            std::string opr   = unaryOpNode->op;
            if (opr == "!") {
                return "bool";
            }
            if (opr == "-") {
                return "int";
            }
            throw std::runtime_error("Unknown op " + opr);
        }
        case ASTNodeType::FunctionCall: {
            auto* functionCallNode = dynamic_cast<FunctionCallNode*>(node);

            std::vector<std::string> paramTypes(functionCallNode->parameters.size());
            for (size_t i = 0; i < functionCallNode->parameters.size(); i++) {
                paramTypes[i] = evalType(functionCallNode->parameters[i].get());
            }

            if (reservedFunctions.count(functionCallNode->name) != 0U) {
                const FunctionSignature* funcSig =
                    findReservedFunction(functionCallNode->name, paramTypes, "");
                return funcSig->returnType;
            }

            FunctionData* funcData = findFunction(functionCallNode->name, paramTypes);
            return funcData->returnType;
        }
        case ASTNodeType::MethodCall: {
            auto* methodCallNode = dynamic_cast<MethodCallNode*>(node);

            return methodCallNode->resolvedType;
        }
        default:
            throw std::runtime_error("Node is not expression");
    }
}

CodeGenerator::FunctionData* CodeGenerator::findFunction(const std::string& name,
                                                         std::vector<std::string> paramTypes) {
    for (auto& funcData : functions[name]) {
        bool match = true;

        if (funcData.params.size() != paramTypes.size()) {
            continue;
        }
        for (size_t i = 0; i < paramTypes.size(); i++) {
            if (funcData.params[i].first != paramTypes[i]) {
                match = false;
                break;
            }
        }

        if (match) {
            return &funcData;
        }
    }

    return nullptr;
}

const CodeGenerator::FunctionSignature* CodeGenerator::findReservedFunction(
    const std::string& name, const std::vector<std::string>& paramTypes,
    const std::string& expectedReturnType) {
    if (reservedFunctions.find(name) == reservedFunctions.end()) {
        return nullptr;
    }

    for (const auto& sig : reservedFunctions.at(name)) {
        if (sig.paramTypes != paramTypes) {
            continue;
        }

        if (expectedReturnType.empty() || sig.returnType == expectedReturnType) {
            return &sig;
        }
    }

    return nullptr;
}

std::string CodeGenerator::getNewLabel() {
    return "L" + std::to_string(labelCounter++);
}

std::string CodeGenerator::mangleFunctionName(const std::string& name,
                                              const std::vector<std::string>& paramTypes) {
    std::string mangledName = "func_" + name;
    for (const auto& type : paramTypes) {
        mangledName += "_";
        if (type == "int") {
            mangledName += "i";
        } else if (type == "string") {
            mangledName += "s";
        } else if (type == "bool") {
            mangledName += "b";
        } else if (type == "int[]") {
            mangledName += "ai";
        }
    }
    return mangledName;
}

void CodeGenerator::emitCode(const std::string& line) {
    if (currentFunction) {
        funcSection.push_back("  " + line);
    } else {
        codeSection.push_back("  " + line);
    }
}

void CodeGenerator::emitCodeLabel(const std::string& label) {
    if (currentFunction) {
        funcSection.push_back(label + ":");
    } else {
        codeSection.push_back(label + ":");
    }
}

void CodeGenerator::emitData(const std::string& line) {
    dataSection.push_back("  " + line);
}

std::string CodeGenerator::getVarLabel(const std::string& varName) {
    if (currentFunction) {
        auto& funcData = functions[currentFunction->name];
        bool isArg     = std::any_of(currentFunction->params.begin(), currentFunction->params.end(),
                                     [&varName](const auto& param) { return param.second == varName; });

        if (isArg) {
            return "arg_" + currentFunction->label + "_" + varName;
        }

        if (variables.find("var_" + varName) != variables.end()) {
            return "var_" + varName;
        }

        return "var_" + currentFunction->label + "_" + varName;
    }

    return "var_" + varName;
}

std::string CodeGenerator::assembleCode() {
    std::stringstream result;

    result << data << "\n";

    for (const auto& line : dataSection) {
        result << line << "\n";
    }

    result << "\n.text\n";
    result << ".org 0x20\n";
    result << interrupts;
    result << read_char;
    result << read_int;
    result << write_to_buf;
    result << read_string;
    result << read_arr;
    result << write_char;
    result << write_int;
    result << write_uint;
    result << write_string;
    result << write_arr;
    result << arr_size;

    for (const auto& line : funcSection) {
        result << line << "\n";
    }

    for (const auto& line : codeSection) {
        result << line << "\n";
    }

    return result.str();
}