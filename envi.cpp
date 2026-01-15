#include "headers/lexer.h"
#include "headers/parser.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

std::string readFile(std::string path);

int main(int argc, char *argv[]) {

  // Technically nothing happens if only envi -f <file> is run
  if (argc < 2) {
    std::cerr << "Usage: envi -f <file> [-t] [-p]" << '\n';
    return 1;
  }

  std::string filename = "";
  bool printTokens = false;
  bool printAST = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-f") {
      if (i + 1 < argc) { // argv would be 0 = program name, 1 = -f, 2 =
                          // filename argc = 3
        filename = argv[++i];
      } else {
        std::cerr << "Error: No file specified for -f" << '\n';
        return 1;
      }
    } else if (arg == "-t") {
      printTokens = true;
    } else if (arg == "-p") {
      printAST = true;
    }
  }

  if (filename.empty()) {
    std::cerr << "Error: No input file specified" << '\n';
    return 1;
  }

  std::string src = readFile(filename);

  if (src == "error000") {
    return 1;
  }

  Lexer lexer(src);
  std::vector<Token> tokens = lexer.tokenize();
  Parser parser(tokens);

  if (printTokens) {
    for (const auto &token : tokens) {
      std::cout << "Type: " << tokenTypeToString(token.type) << ", Text: '"
                << token.text << "', Line: " << token.line << std::endl;
    }
  }

  if (printAST) {
    auto programNode = parser.parse();
    programNode->print();
  }

  return 0;
}

std::string readFile(std::string path) {
  std::ifstream file(path);

  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << path << std::endl;
    return "error000";
  }

  // Replace with this
  // if (!file.is_open()) {
  //   throw std::runtime_error("File could not be opened");
  // }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string src = buffer.str();

  return src;
}