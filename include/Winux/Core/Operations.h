#pragma once

#include <functional>
#include <utility>

namespace Winux::Core {

template <typename ResultType, typename Options>
class Operation
{
protected:
    using action = std::function<ResultType(Options)>;

    Operation(action action, Options options = {})
        : action_(std::move(action)), options_(std::move(options))
    {
    }

public:
    /*
        @summary
        Executes the stored operation using the configured options.
    */
    ResultType start()
    {
        return action_(options_);
    }

protected:
    /*
        @summary
        Returns the mutable options bag used by the operation.
    */
    Options& options() noexcept
    {
        return options_;
    }

private:
    action action_;
    Options options_;
};

}
