#include "headers/lexer.h"
#include <cctype>
#include <string>
#include <vector>

char Lexer::peek() const { return src[pos]; }

void Lexer::advance() { pos++; }

// Handles @start, @bg etc
Token Lexer::readAtFunc() {

  // Token currentToken(TokenType::AtFunc, "", line);
  std::string tokenText = "";

  advance();

  while (peek() != ' ' && peek() != '\t' && peek() != '\r' && peek() != '\n' &&
         peek() != '\0') {
    tokenText += peek();
    advance();
  }

  return Token(TokenType::AtFunc, tokenText, line);
}

// Handles #project, #character etc
Token Lexer::readHashVar() {

  std::string tokenText = "";

  advance();

  while (peek() != ' ' && peek() != '\t' && peek() != '\r' && peek() != '\n' &&
         peek() != '\0') {
    tokenText += peek();
    advance();
  }

  state = LexerState::Json;

  return Token(TokenType::HashVar, tokenText, line);
}

// Handles "Hello" etc
Token Lexer::readString() {

  std::string tokenText = "";

  advance();

  while (peek() != '"') {
    tokenText += peek();
    advance();
  }

  advance();

  return Token(TokenType::String, tokenText, line);
}

// Handles identifier in @scene <identifier> etc..
Token Lexer::readIdentifier() {

  std::string tokenText = "";

  char c = peek();

  while (std::isalnum(c)) {
    tokenText += c;
    advance();
    c = peek();
  }

  return Token(TokenType::Identifier, tokenText, line);
}

Token Lexer::readJsonBlock() {
  std::string tokenText = "";

  char c = peek();

  while (c != '}') {

    if (c == '\n') {
      line++;
    }

    tokenText += c;
    advance();
    c = peek();
  }

  tokenText += c;
  advance();
  state = LexerState::Script;

  return Token(TokenType::JsonBlock, tokenText, line);
}

// Returns next token
Token Lexer::nextToken() {

  while (pos < src.length()) {
    char c = peek();

    if (state == LexerState::Json) {

      switch (c) {

      case ' ':
      case '\t':
      case '\r':
        advance();
        break;
      case '\n':
        line++;
        advance();
        return Token(TokenType::Newline, "", line);
      case '\0':
        return Token(TokenType::EndOfFile, "", line);
      case '{':
        return readJsonBlock();
      default:
        advance();
        return Token(TokenType::Unknown, "", line);
      }

    } else if (state == LexerState::Script) {

      switch (c) {

      case ' ':
      case '\t':
      case '\r':
        advance();
        break;
      case '@':
        return readAtFunc();
      case '#':
        return readHashVar();
      case '{':
        advance();
        return Token(TokenType::LBrace, "", line);
      case '}':
        advance();
        return Token(TokenType::RBrace, "", line);
      case '"':
        return readString();
      case '\n':
        line++;
        advance();
        return Token(TokenType::Newline, "", line);
      case '\0':
        return Token(TokenType::EndOfFile, "", line);
      default:

        if (std::isalnum(c)) {
          return readIdentifier();
        }

        advance();
        return Token(TokenType::Unknown, "", line);
      }
    }
  }

  return Token(TokenType::Unknown, "", line);
}

// Returns all tokens
std::vector<Token> Lexer::tokenize() {

  std::vector<Token> tokens;

  while (pos < src.length()) {
    tokens.push_back(nextToken());
  }

  return tokens;
}

// Converts token type to string for printing/debugging
std::string tokenTypeToString(TokenType type) {
  switch (type) {
  case TokenType::AtFunc:
    return "AtFunc";
  case TokenType::HashVar:
    return "HashVar";
  case TokenType::String:
    return "String";
  case TokenType::LBrace:
    return "LBrace";
  case TokenType::RBrace:
    return "RBrace";
  case TokenType::Newline:
    return "Newline";
  case TokenType::EndOfFile:
    return "EndOfFile";
  case TokenType::Unknown:
    return "Unknown";
  case TokenType::Identifier:
    return "Identifier";
  case TokenType::JsonBlock:
    return "JsonBlock";
  default:
    return "Unknown";
  }
}
