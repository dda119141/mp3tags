#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int argc, char *argv[])
{
  Catch::Session session; // There must be exactly one instance
  int directory = 0;      // Some user variable you want to be able to set

  // Build a new parser on top of Catch's
  using namespace Catch::clara;
  auto cli =
      session.cli() // Get Catch's composite command line parser
      | Opt(directory,
            "directory") // bind variable to a new option, with a hint string
            ["-g"]["--directory"] // the option names it will respond to
      ("how high?");              // description string for the help output

  // Now pass the new composite back to Catch so it uses that
  session.cli(cli);

  // Let Catch (using Clara) parse the command line
  int returnCode = session.applyCommandLine(argc, argv);
  if (returnCode != 0) // Indicates a command line error
    return returnCode;

  // if set on the command line then 'directory' is now set at this point
  if (directory > 0)
    std::cout << "directory: " << directory << std::endl;

  return session.run();
}
