#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ASTVisitor.hpp"

enum class ASTNodeType : uint8_t {
    VarDecl,

    NumberLiteral,
    CharLiteral,
    StringLiteral,
    BooleanLiteral,
    VoidLiteral,
    IntArrayLiteral,
    ArrayGet,
    ArraySize,
    MethodCall,

    Identifier,
    Assignment,

    BinaryOp,
    UnaryOp,

    If,
    While,
    Break,

    Block,
    Parameter,
    Function,
    CallParameter,
    FunctionCall,
    Expression,
    Return
};

struct ASTNode {
    ASTNodeType nodeType;

    ASTNode(ASTNodeType nodeType) : nodeType(nodeType) {
    }

    virtual void accept(ASTVisitor& visitor) = 0;

    virtual ~ASTNode() = default;

    ASTNode(const ASTNode&)            = delete;
    ASTNode& operator=(const ASTNode&) = delete;

    ASTNode(ASTNode&&)            = delete;
    ASTNode& operator=(ASTNode&&) = delete;
};

struct ExpressionNode : ASTNode {
    std::string resolvedType;

    ExpressionNode(ASTNodeType nodeType) : ASTNode(nodeType) {
    }
};

struct VarDeclNode : ASTNode {
    std::string type;
    std::string name;
    std::unique_ptr<ASTNode> value;

    VarDeclNode(std::string type, std::string name, std::unique_ptr<ASTNode> value)
        : ASTNode(ASTNodeType::VarDecl),
          type(std::move(type)),
          name(std::move(name)),
          value(std::move(value)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct NumberLiteralNode : ExpressionNode {
    long number;

    NumberLiteralNode(long number) : ExpressionNode(ASTNodeType::NumberLiteral), number(number) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct CharLiteralNode : ExpressionNode {
    char value;

    CharLiteralNode(char value) : ExpressionNode(ASTNodeType::CharLiteral), value(value) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct StringLiteralNode : ExpressionNode {
    std::string value;

    StringLiteralNode(std::string value)
        : ExpressionNode(ASTNodeType::StringLiteral), value(std::move(value)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct BooleanLiteralNode : ExpressionNode {
    bool value;

    BooleanLiteralNode(bool value) : ExpressionNode(ASTNodeType::BooleanLiteral), value(value) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct VoidLiteralNode : ExpressionNode {
    VoidLiteralNode() : ExpressionNode(ASTNodeType::VoidLiteral) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct IntArrayLiteralNode : ExpressionNode {
    std::vector<std::unique_ptr<ASTNode>> values;

    IntArrayLiteralNode(std::vector<std::unique_ptr<ASTNode>> values)
        : ExpressionNode(ASTNodeType::IntArrayLiteral), values(std::move(values)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct ArrayGetNode : ExpressionNode {
    std::unique_ptr<ASTNode> object;
    std::unique_ptr<ASTNode> index;

    ArrayGetNode(std::unique_ptr<ASTNode> object, std::unique_ptr<ASTNode> index)
        : ExpressionNode(ASTNodeType::ArrayGet),
          object(std::move(object)),
          index(std::move(index)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct MethodCallNode : ExpressionNode {
    std::unique_ptr<ASTNode> object;
    std::string methodName;
    std::vector<std::unique_ptr<ASTNode>> arguments;

    MethodCallNode(std::unique_ptr<ASTNode> object, std::string methodName,
                   std::vector<std::unique_ptr<ASTNode>> arguments)
        : ExpressionNode(ASTNodeType::MethodCall),
          object(std::move(object)),
          methodName(std::move(methodName)),
          arguments(std::move(arguments)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct IdentifierNode : ExpressionNode {
    std::string name;

    IdentifierNode(std::string name)
        : ExpressionNode(ASTNodeType::Identifier), name(std::move(name)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct AssignNode : ASTNode {
    std::unique_ptr<ASTNode> var1;
    std::unique_ptr<ASTNode> var2;

    AssignNode(std::unique_ptr<ASTNode> var1, std::unique_ptr<ASTNode> var2)
        : ASTNode(ASTNodeType::Assignment), var1(std::move(var1)), var2(std::move(var2)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct BinaryOpNode : ExpressionNode {
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;

    BinaryOpNode(std::string operation, std::unique_ptr<ASTNode> left,
                 std::unique_ptr<ASTNode> right)
        : ExpressionNode(ASTNodeType::BinaryOp),
          op(std::move(operation)),
          left(std::move(left)),
          right(std::move(right)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct UnaryOpNode : ExpressionNode {
    std::string op;
    std::unique_ptr<ASTNode> operand;

    UnaryOpNode(std::string operation, std::unique_ptr<ASTNode> operand)
        : ExpressionNode(ASTNodeType::UnaryOp),
          op(std::move(operation)),
          operand(std::move(operand)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct IfNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch = nullptr;

    IfNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> thenBranch,
           std::unique_ptr<ASTNode> elseBranch = nullptr)
        : ASTNode(ASTNodeType::If),
          condition(std::move(condition)),
          thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct WhileNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;

    WhileNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body)
        : ASTNode(ASTNodeType::While), condition(std::move(condition)), body(std::move(body)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct BreakNode : ASTNode {
    BreakNode() : ASTNode(ASTNodeType::Break) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct BlockNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> children;

    void addChild(std::unique_ptr<ASTNode> child) {
        if (child == nullptr) {
            return;
        }
        children.push_back(std::move(child));
    }

    BlockNode() : ASTNode(ASTNodeType::Block) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct ParameterNode : ASTNode {
    std::string name;
    std::string type;

    ParameterNode(std::string name, std::string type)
        : ASTNode(ASTNodeType::Parameter), name(std::move(name)), type(std::move(type)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct FunctionNode : ASTNode {
    std::string returnType;
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;

    FunctionNode(std::string returnType, std::string name,
                 std::vector<std::unique_ptr<ASTNode>> parameters, std::unique_ptr<ASTNode> body)
        : ASTNode(ASTNodeType::Function),
          returnType(std::move(returnType)),
          name(std::move(name)),
          parameters(std::move(parameters)),
          body(std::move(body)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct FunctionCallNode : ExpressionNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;

    FunctionCallNode(std::string name, std::vector<std::unique_ptr<ASTNode>> parameters)
        : ExpressionNode(ASTNodeType::FunctionCall),
          name(std::move(name)),
          parameters(std::move(parameters)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

struct ReturnNode : ASTNode {
    std::unique_ptr<ASTNode> returnValue;

    ReturnNode(std::unique_ptr<ASTNode> returnValue)
        : ASTNode(ASTNodeType::Return), returnValue(std::move(returnValue)) {
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};