#define CATCH_CONFIG_RUNNER
#include "test/catch/catch.hpp"

int CatchMain(int argc, char* argv[])
{
  int result = Catch::Session().run(argc, argv);
  return result;
}