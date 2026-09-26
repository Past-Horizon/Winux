#pragma once

#include <Winux/Core/Results.h>

#include <any>
#include <functional>
#include <memory>
#include <string>

namespace Winux::Contracts {

/*
    @summary
    Represents a cross-platform terminal surface that can execute commands and other things.
*/
class ITerminal {
public:
    class ICommand {
    public:
        virtual ~ICommand() = default;

        /*
            @summary
            Reports whether the command can currently execute.
        */
        virtual bool can_execute(const std::any& parameter = {}) const = 0;

        /*
            @summary
            Executes the command with an optional payload.
        */
        virtual Core::Result<void> execute(const std::any& parameter = {}) = 0;

        /*
            @summary
            Registers a handler that is raised when the command availability changes.
        */
        virtual void add_changed(const std::function<void()>& handler) = 0;

        /*
            @summary
            Unregisters a handler that was previously added.
        */
        virtual void remove_changed(const std::function<void()>& handler) = 0;

        /*
            @summary
            Raises a re-evaluation notification for subscribers.
        */
        virtual void raise_changed() = 0;
    };

    using ExecuteHandler = std::function<Core::Result<void>(const std::any&)>;
    using CanExecuteHandler = std::function<bool(const std::any&)>;

    virtual ~ITerminal() = default;

    /*
        @summary
        Executes a command line and returns the text emitted by the process.
    */
    virtual Core::Result<std::string> execute_command(const std::wstring& command_line) = 0;

    /*
        @summary
        Creates a command object that can be reused across callers.
    */
    virtual std::shared_ptr<ICommand> create_command(
        ExecuteHandler execute_handler,
        CanExecuteHandler can_execute_handler = {}) = 0;
};

}
