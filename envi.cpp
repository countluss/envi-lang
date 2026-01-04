#include "headers/lexer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

std::string readFile(std::string path);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: envi -f <file> [-t]" << std::endl;
    return 1;
  }

  std::string filename = "";
  bool printTokens = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-f") {
      if (i + 1 < argc) {
        filename = argv[++i];
      } else {
        std::cerr << "Error: No file specified for -f" << std::endl;
        return 1;
      }
    } else if (arg == "-t") {
      printTokens = true;
    }
  }

  if (filename.empty()) {
    std::cerr << "Error: No input file specified" << std::endl;
    return 1;
  }

  std::string src = readFile(filename);

  if (src == "error000") {
    return 1;
  }

  Lexer lexer(src);
  std::vector<Token> tokens = lexer.tokenize();

  if (printTokens) {
    for (const auto &token : tokens) {
      std::cout << "Type: " << tokenTypeToString(token.type) << ", Text: '"
                << token.text << "', Line: " << token.line << std::endl;
    }
  }

  return 0;
}

std::string readFile(std::string path) {
  std::ifstream file(path);

  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << path << std::endl;
    return "error000";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string src = buffer.str();

  return src;
}