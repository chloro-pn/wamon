#pragma once

#include "wamon/ast.h"
#include "wamon/scanner.h"

namespace wamon {

void TryToParseFunctionDeclaration(const std::vector<WamonToken> &tokens, size_t& begin);

void TryToParseStructDeclaration(const std::vector<WamonToken> &tokens, size_t& begin);

void TryToParseVariableDeclaration(const std::vector<WamonToken> &tokens, size_t& begin);

void Parse(const std::vector<WamonToken> &tokens);

}  // namespace wamon