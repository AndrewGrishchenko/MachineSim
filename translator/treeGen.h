#ifndef _TREE_GEN_H
#define _TREE_GEN_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ASTNode.hpp"
#include "semanticAnalyzer.h"

enum class TokenType : uint8_t {
    // Keywords
    KeywordIf,
    KeywordElse,
    KeywordWhile,
    KeywordBreak,
    KeywordReturn,
    KeywordVoid,

    // Data types
    KeywordInt,
    KeywordUint,
    KeywordChar,
    KeywordString,
    KeywordBool,
    KeywordIntArr,

    // Utility
    Identifier,
    Equals,
    Number,
    Char,
    String,
    Boolean,

    // Delimeters
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Semicolon,
    Dot,
    Comma,

    // Logic operators
    LogicNot,
    LogicAnd,
    LogicOr,
    LogicEqual,
    LogicNotEqual,
    LogicGreater,
    LogicGreaterEqual,
    LogicLess,
    LogicLessEqual,

    // Operators
    Plus,
    Minus,
    Multiply,
    Divide,
    Rem,

    // Utility
    EndOfFile,
    Unknown
};

struct Token {
    TokenType type;
    std::string value;

    Token(TokenType type, std::string value) : type(type), value(std::move(value)) {
    }
};

class TreeGenerator {
public:
    TreeGenerator() = default;

    std::unique_ptr<ASTNode> makeTree(const std::string& data);

private:
    static std::vector<Token> tokenize(const std::string& input);

    std::unique_ptr<ASTNode> parseVarStatement(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseAssignStatement(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseBlock(std::vector<Token> tokens, size_t& pos);

    std::unique_ptr<ASTNode> parseStatement(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseIf(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseWhile(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseBreak(std::vector<Token> tokens, size_t& pos);

    std::unique_ptr<ASTNode> parseParameter(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseFunction(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseFunctionCall(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseReturn(std::vector<Token> tokens, size_t& pos);

    std::unique_ptr<ASTNode> parseExpression(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseArray(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseArrayGet(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseMethodCall(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseTerm(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseFactor(std::vector<Token> tokens, size_t& pos);

    std::unique_ptr<ASTNode> parseLogicOr(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseLogicAnd(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseEquality(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseComparsion(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parseUnary(std::vector<Token> tokens, size_t& pos);
    std::unique_ptr<ASTNode> parsePrimary(std::vector<Token> tokens, size_t& pos);

    static std::string tokenStr(const Token& token);

    static bool isAlpha(char character) {
        return std::isalpha(static_cast<unsigned char>(character)) != 0 || character == '_';
    }
    static bool isDigit(char character) {
        return std::isdigit(static_cast<unsigned char>(character)) != 0;
    }
    static bool isAlnum(char character) {
        return isAlpha(character) || isDigit(character);
    }
};

#endif