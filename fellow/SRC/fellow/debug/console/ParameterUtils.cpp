#include "fellow/debug/console/ParameterUtils.h"
#include <charconv>
#include <sstream>

using namespace std;

namespace ParameterUtils
{
  vector<string> SplitArguments(const string &args)
  {
    vector<string> tokens;
    istringstream is(args);
    string token;

    while (!is.eof())
    {
      is >> token;
      if (!token.empty())
      {
        tokens.push_back(token);
      }
    }

    return tokens;
  }

  void ValidateIsEvenAddress(ULO address, ValidationResult &validationResult)
  {
    if (address & 1)
    {
      validationResult.Errors.emplace_back("Odd address not allowed");
    }
  }
}
