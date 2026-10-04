#pragma once

#include <Winux/Contracts/ITerminal.h>

#include <cstdio>
#include <memory>
#include <string>

namespace Winux::Platform {

std::string ReadPipe(std::FILE* pipe);

std::shared_ptr<Contracts::ITerminal::ICommand> MakeTerminalCommand(
    Contracts::ITerminal::ExecuteHandler execute_handler,
    Contracts::ITerminal::CanExecuteHandler can_execute_handler = {});

}
