#include "fellow/debug/console/HelpHandler.h"
#include "fellow/debug/console/DebugWriter.h"

using namespace std;

string HelpHandler::ToString(const vector<HelpText> &helpTexts) const
{
  DebugWriter writer;
  size_t maxCommandLength = writer.GetMaxLength<HelpText>(helpTexts, [](const HelpText &helpText) -> const std::string & { return helpText.GetCommand(); });
  size_t maxDescriptionLength = writer.GetMaxLength<HelpText>(helpTexts, [](const HelpText &helpText) -> const std::string & { return helpText.GetDescription(); });

  for (const auto &helpText : helpTexts)
  {
    writer.StringLeft(helpText.Command, maxCommandLength).String(" - ").String(helpText.Description).Endl();
  }

  writer.StringLeft("exit", maxCommandLength).String(" - ").String("Exit the debugger").Endl();

  return writer.GetStreamString();
}

vector<HelpText> HelpHandler::GetHelpTexts() const
{
  vector<HelpText> helpTexts;

  for (auto *handler : _commandHandlers)
  {
    helpTexts.emplace_back(handler->Help());
  }

  return helpTexts;
}

HelpText HelpHandler::Help() const
{
  return HelpText{.Command = GetCommandToken(), .Description = "Command summary"};
}

ConsoleCommandHandlerResult HelpHandler::Handle(const vector<string> &tokens)
{
  return ConsoleCommandHandlerResult{.Success = true, .ExecuteResult = ToString(GetHelpTexts())};
}

HelpHandler::HelpHandler(const vector<ConsoleCommandHandler *> &commandHandlers) : ConsoleCommandHandler("h"), _commandHandlers(commandHandlers)
{
}
