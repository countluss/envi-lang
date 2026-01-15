#ifndef LEXER_H
#define LEXER_H

#include <cstddef>
#include <string>
#include <vector>

enum class TokenType {
  AtFunc,    // @Scene
  HashVar,   // #character
  String,    // "Hello"
  LBrace,    // {
  RBrace,    // }
  Newline,   // \n
  EndOfFile, // This Token is likely never created
  Unknown,
  Identifier,
  JsonBlock
};

enum class LexerState {
  Script,
  Json,
};

typedef struct Token {
  TokenType type;
  std::string text;
  int line;

  Token(TokenType type, std::string text, int line)
      : type(type), text(text), line(line) {}
} Token;

class Lexer {

private:
  const std::string &src;
  size_t pos;
  int line;
  LexerState state;

  char peek() const;

  void advance();

  Token readAtFunc();
  Token readHashVar();
  Token readString();
  Token readIdentifier();
  Token readJsonBlock();

  Token nextToken();

public:
  Lexer(const std::string &src)
      : src(src), pos(0), line(1), state(LexerState::Script) {}

  std::vector<Token> tokenize();
};

std::string tokenTypeToString(TokenType type);

#endif