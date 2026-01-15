#include "headers/parser.h"
#include "headers/lexer.h"
#include "libraries/simdjson/simdjson.h"
#include <memory>

const Token &Parser::peek() const { return tokens.at(current); }
// bool Parser::isAtEnd() const { return peek().type == TokenType::EndOfFile; }
bool Parser::isAtEnd() const { return current >= tokens.size(); }
void Parser::advance() { current++; }
void Parser::retract() { current--; }
bool Parser::check(TokenType token) const { return peek().type == token; }
void Parser::skipNewLines() {
  while (!isAtEnd() && peek().type == TokenType::Newline) {
    advance();
  }
}
void Parser::retractNewLines() {
  while (peek().type == TokenType::Newline) {
    retract();
  }
}

std::unique_ptr<ProgramNode> Parser::parse() {
  auto program = std::make_unique<ProgramNode>();

  if (tokens.empty()) {
    return program;
  }

  Token token = peek();

  // std::cout << "current = " << current;

  while (!isAtEnd()) {

    token = peek();

    // std::cout << "current = " << current;

    if (token.type == TokenType::Newline || token.type == TokenType::Unknown) {
      advance();
      // token = peek();
      continue;
    }

    // std::cout << "current = " << current;

    program->items.push_back(parseTopLevelToken());
    advance();
    // token = peek();
  }

  return program;
}

// Parses the current token which is top level
std::unique_ptr<ASTNode> Parser::parseTopLevelToken() {

  const Token token = peek();

  switch (token.type) {

  case TokenType::AtFunc:
    return parseOuterFunc();
  case TokenType::HashVar:
    return parseOuterData();
  default:
    return nullptr;
  }

  return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseScene() {

  auto scene = std::make_unique<SceneNode>();

  advance(); // next should be identifier

  skipNewLines();

  // After skipNewLines() invokes, there are 2 cases
  // 1: The current token is NOT NewLine
  // 2: The file has ended

  if (isAtEnd())
    return error("Invalid scene declaration", -1);

  const Token identifier = peek();

  // No need to handle EndOfFile
  if (identifier.type != TokenType::Identifier) {
    return error("Invalid scene declaration", identifier.line);
  }

  scene->name = identifier.text;

  advance(); // next should be {

  skipNewLines();

  if (isAtEnd())
    return error("Invalid scene declaration", -1);

  Token token = peek();

  if (token.type != TokenType::LBrace) {
    return error("Invalid scene declaration", token.line);
  }

  advance();

  // Now proceed until }

  while (!isAtEnd() && token.type != TokenType::RBrace) {

    // token = peek();

    skipNewLines();

    if (isAtEnd()) {
      return error("Invalid scene declaration", token.line);
    }

    token = peek();

    if (token.type == TokenType::RBrace) {
      break;
    }

    // Currently scenes can have @func and #var inside it
    if (token.type == TokenType::AtFunc) {
      scene->statements.push_back(parseInnerFunc());
    } else if (token.type == TokenType::HashVar) {
      scene->statements.push_back(parseInnerData());
    }

    advance();

    // token = peek();
  }

  return scene;
}

std::unique_ptr<ASTNode> Parser::parseInnerFunc() {

  const Token token =
      peek(); // Current inner function, ex: jump, bg, text, etc..

  if (token.text == "bg") {
    return parseBg();
  } else if (token.text == "text") {
    return parseDialogue();
  } else if (token.text == "jump") {
    return parseJump();
  } else {
    return error("Error parsing inner @function", token.line);
  }
}

std::unique_ptr<ASTNode> Parser::parseOuterFunc() {

  const Token token = peek();
  std::string cmd = token.text;

  if (cmd == "scene") {
    return parseScene();
  }

  return error("Error parsing outer @function", token.line);
}

std::unique_ptr<ASTNode> Parser::parseOuterData() {

  const Token token = peek();

  std::string cmd = token.text;

  if (cmd == "project") {
    return parseProjectData();
  }

  return error("Error parsing outer #data", token.line);
}

std::unique_ptr<ASTNode> Parser::parseProjectData() {

  // const Token token = peek();

  advance(); // Advance to next json block

  skipNewLines();

  if (isAtEnd())
    return error("Invalid #project data declaration", -1);

  const Token json = peek();

  if (json.type != TokenType::JsonBlock) {
    return error("Error parsing outer data #project json{}", json.line);
  }

  simdjson::padded_string pjson =
      json.text; // Avoid this allocation maybe? Instead of multiple times, it
                 // should only happen a single time
                 // And then reuse that buffer

  ondemand::document doc;

  try {
    doc = jparser.iterate(pjson);
  } catch (...) {
    return error("Error parsing outer data #project json{}", json.line);
  }

  auto projectNode = std::make_unique<ProjectDataNode>();
  std::string_view temp;

  try {
    temp = doc["name"]->get_string();
    projectNode->name = temp;
    temp = doc["start"]->get_string();
    projectNode->start = temp;
  } catch (...) {
    return error("Error parsing outer data #project json{}", json.line);
  }

  return projectNode;
}

std::unique_ptr<ASTNode> Parser::parseInnerData() { return nullptr; }

// Convenience function for returning error nodes
std::unique_ptr<ASTNode> Parser::error(std::string message, int line) {

  auto error = std::make_unique<ErrorNode>();
  error->message = message;
  error->line = line;

  return error;
}

std::unique_ptr<ASTNode> Parser::parseBg() {

  advance(); // Next should be identifier, ex: @bg bg1, next should be bg1

  skipNewLines();

  if (isAtEnd())
    return error("Invalid @bg function declaration", -1);

  const Token token = peek();

  if (token.type != TokenType::Identifier) {
    return error("Invalid inner function", token.line);
  }

  auto bg = std::make_unique<BgNode>();
  bg->backgroundId = token.text;

  return bg;
}

std::unique_ptr<ASTNode> Parser::parseDialogue() {

  advance(); // Advance to first token after @text

  skipNewLines();

  // Formats allowed
  // @text "John" "Hello there"
  // @text "Hello there" // This is for the player's inner voice
  // @text j "Hello there" // This is for future implementation where j is a
  // variable

  if (isAtEnd())
    return error("Invalid @text function declaration", -1);

  const Token token1 = peek();

  if (token1.type != TokenType::Identifier &&
      token1.type != TokenType::String) {
    return error("Invalid internal function", token1.line);
  }

  advance(); // Advance to second token

  skipNewLines();

  if (isAtEnd())
    return error("Invalid @text declaration", -1);

  const Token token2 = peek();

  auto dialogue = std::make_unique<TextNode>();

  if (token1.type == TokenType::String) {
    if (token2.type == TokenType::String) {
      dialogue->characterId = "";
      dialogue->characterName = token1.text;
      dialogue->text = token2.text;
    } else {
      retract();
      retractNewLines();

      dialogue->characterId = "0"; // Special id to denote ourselves in game
      dialogue->characterName = "";
      dialogue->text = token1.text;
    }
  } else if (token1.type == TokenType::Identifier) {
    if (token2.type != TokenType::String) {
      return error("Invalid internal function", token2.line);
    } else {
      dialogue->characterId = token1.text;
      dialogue->characterName = "";
      dialogue->text = token2.text;
    }
  } else {
    return error("Invalid internal function", token1.line);
  }

  // return error("Invalid internal function", token1.line);

  return dialogue;
}

std::unique_ptr<ASTNode> Parser::parseJump() {

  advance();

  skipNewLines();

  if (isAtEnd())
    return error("Invalid @jump declaration", -1);

  const Token token = peek();

  if (token.type != TokenType::Identifier) {
    return error("Invalid inner function", token.line);
  }

  auto jump = std::make_unique<JumpNode>();
  jump->targetScene = token.text;

  return jump;
}