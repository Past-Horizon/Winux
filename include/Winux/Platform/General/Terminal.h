#pragma once

#include <Winux/Contracts/ITerminal.h>

#include <cstdio>
#include <memory>
#include <string>

namespace Winux::Platform {

namespace detail {

std::string read_pipe(std::FILE* pipe);

std::shared_ptr<Contracts::ITerminal::ICommand> make_terminal_command(
    Contracts::ITerminal::ExecuteHandler execute_handler,
    Contracts::ITerminal::CanExecuteHandler can_execute_handler = {});

}

}
