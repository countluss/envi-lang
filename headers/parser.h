#ifndef PARSER_H
#define PARSER_H

#include "headers/lexer.h"
#include "libraries/simdjson/simdjson.h"
#include <cstddef>
#include <memory>
#include <ostream>
#include <vector>

using namespace simdjson;

// Base node for AST, virtual destructor to automatically delete derived classes
// on base class deletion

struct ASTNode {
  virtual ~ASTNode() = default;

  virtual void print(int indent = 0) const = 0;

  void indent(int level) const {
    for (int i = 0; i < level; ++i)
      std::cout << "  ";
  }
};

struct ProgramNode : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> items;

  void print(int indent = 0) const override {

    std::cout << "ProgramNode\n";

    for (auto &item : items) {
      item->print(indent + 1);
    }
  }
};

struct SceneNode : ASTNode {
  std::string name;

  // A scene node can contain all types of nodes within it
  std::vector<std::unique_ptr<ASTNode>> statements;

  void print(int indent = 0) const override {

    this->indent(indent);

    std::cout << "SceneNode(" << name << ")\n";

    for (auto &statement : statements) {
      statement->print(indent + 1);
    }
  }
};

struct JumpNode : ASTNode {
  std::string targetScene;

  void print(int indent = 0) const override {
    this->indent(indent);
    std::cout << "JumpNode(" << targetScene << ")\n";
  }
};

struct BgNode : ASTNode {
  std::string backgroundId;

  void print(int indent = 0) const override {
    this->indent(indent);
    std::cout << "BgNode(" << backgroundId << ")\n";
  }
};

struct TextNode : ASTNode {
  std::string characterId;
  std::string characterName; // Only present if characterId is not present
  std::string text;

  void print(int indent = 0) const override {
    this->indent(indent);
    std::cout << "TextNode(" << characterId << "," << characterName << ","
              << text.substr(0, 4) << "..."
              << ")\n"; // Truncate the text to prevent spam when AST printing
  }
};

struct ErrorNode : ASTNode {
  std::string message;
  int line;

  void print(int indent = 0) const override {
    this->indent(indent);
    std::cout << "ErrorNode(" << message << "," << line << ")\n";
  }
};

struct ProjectDataNode : ASTNode {
  std::string name;  // Name of the project
  std::string start; // Scene from which it the game starts

  void print(int indent = 0) const override {
    this->indent(indent);
    std::cout << "ProjectDataNode(" << name << "," << start << ")\n";
  }
};

class Parser {

private:
  const std::vector<Token> &tokens;
  size_t current;
  ondemand::parser jparser; // Single instance to save memory, also a parser can
                            // only parse a single json doc at a time

  const Token &peek() const;
  bool isAtEnd() const;
  void advance();
  bool check(TokenType token) const;
  void skipNewLines();
  void retractNewLines();
  void retract();

  std::unique_ptr<ASTNode> error(std::string message, int line);

  std::unique_ptr<ASTNode> parseOuterData();
  std::unique_ptr<ASTNode> parseInnerData();
  std::unique_ptr<ASTNode> parseScene();
  std::unique_ptr<ASTNode> parseInnerFunc();
  std::unique_ptr<ASTNode> parseOuterFunc();
  std::unique_ptr<ASTNode> parseTopLevelToken();
  std::unique_ptr<ASTNode> parseBg();
  std::unique_ptr<ASTNode> parseDialogue();
  std::unique_ptr<ASTNode> parseJump();
  std::unique_ptr<ASTNode> parseProjectData();

public:
  Parser(const std::vector<Token> &tokens) : tokens(tokens), current(0) {}

  std::unique_ptr<ProgramNode> parse();
};

#endif