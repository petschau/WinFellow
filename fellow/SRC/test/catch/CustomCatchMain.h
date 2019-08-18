#define CATCH_CONFIG_RUNNER
#include "test/catch/catch.hpp"
#include "test/catch/catch_discover.hpp"

int CatchMain(int argc, char* argv[])
{
  //int result = Catch::Session().run(argc, argv);
  //return result;

  Catch::Session session;

  bool doDiscover = false;

  Catch::addDiscoverOption(session, doDiscover);

  int returnCode = session.applyCommandLine(argc, argv);
  if (returnCode != 0) return returnCode;

  return Catch::runDiscoverSession(session, doDiscover);


}


