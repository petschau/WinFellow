#pragma once

#include "fellow/api/defs.h"
#include "fellow/debug/console/ConsoleCommandHandler.h"

#include <vector>
#include <string>
#include <charconv>

namespace ParameterUtils
{
  std::vector<std::string> SplitArguments(const std::string &args);
  void ValidateIsEvenAddress(ULO address, ValidationResult &validationResult);

  void ParseHex(const std::string &token, auto &value, ValidationResult &validationResult)
  {
    const char *last = token.data() + token.size();
    auto [ptr, ec]{std::from_chars(token.data(), last, value, 16)};
    if (ec == std::errc())
    {
      if (ptr != last) validationResult.Errors.emplace_back("Invalid number: " + token);
    }
    else if (ec == std::errc::invalid_argument)
    {
      validationResult.Errors.emplace_back("Invalid number: " + token);
    }
    else if (ec == std::errc::result_out_of_range)
    {
      validationResult.Errors.emplace_back("Number too large: " + token);
    }
  }
}
