#pragma once

#include <optional>
#include <string>
#include <utility>

namespace Winux::Core {

/*
    @summary
    Describes the outcome of an operation returning a Winux result.
*/
enum class ResultStatus
{
    Success,
    Failure,
    Info,
    Warning
};

template <typename Value>
class Result
{
public:
    /*
        @summary
        Creates a successful result with an attached value.

        @param value
        Value returned by the operation.

        @param message
        Optional detail to accompany the result.
    */
    static Result success(Value value, std::string message = {})
    {
        return Result(ResultStatus::Success, std::move(value), std::move(message));
    }

    /*
        @summary
        Creates a failed result without a value.

        @param message
        Reason the operation failed.
    */
    static Result failure(std::string message)
    {
        return Result(ResultStatus::Failure, std::nullopt, std::move(message));
    }

    /*
        @summary
        Creates an informational result with a value.

        @param value
        Value returned by the operation.

        @param message
        Optional extra context.
    */
    static Result info(Value value, std::string message = {})
    {
        return Result(ResultStatus::Info, std::move(value), std::move(message));
    }

    /*
        @summary
        Creates a warning result with a value.

        @param value
        Value returned by the operation.

        @param message
        Warning text describing the condition.
    */
    static Result warning(Value value, std::string message = {})
    {
        return Result(ResultStatus::Warning, std::move(value), std::move(message));
    }

    /*
        @summary
        Returns the status attached to this result.
    */
    constexpr ResultStatus status() const noexcept
    {
        return status_;
    }

    /*
        @summary
        Checks whether the result indicates a successful outcome.
    */
    constexpr bool succeeded() const noexcept
    {
        return status_ == ResultStatus::Success;
    }

    /*
        @summary
        Checks whether the result indicates a failure.
    */
    constexpr bool failed() const noexcept
    {
        return status_ == ResultStatus::Failure;
    }

    /*
        @summary
        Returns the stored value.
    */
    const Value& value() const &
    {
        return value_.value();
    }

    /*
        @summary
        Moves the stored value out of the result.
    */
    Value&& value() &&
    {
        return std::move(value_.value());
    }

    /*
        @summary
        Returns the message associated with the result.
    */
    const std::string& message() const noexcept
    {
        return message_;
    }

private:
    Result(ResultStatus status, std::optional<Value> value, std::string message)
        : status_(status), value_(std::move(value)), message_(std::move(message))
    {
    }

    ResultStatus status_;
    std::optional<Value> value_;
    std::string message_;
};

template <>
class Result<void>
{
public:
    /*
        @summary
        Creates a successful result without a payload.

        @param message
        Optional status description.
    */
    static Result success(std::string message = {})
    {
        return Result(ResultStatus::Success, std::move(message));
    }

    /*
        @summary
        Creates a failed result without a payload.

        @param message
        Reason the operation failed.
    */
    static Result failure(std::string message)
    {
        return Result(ResultStatus::Failure, std::move(message));
    }

    /*
        @summary
        Creates an informational result without a payload.

        @param message
        Context for the informational result.
    */
    static Result info(std::string message)
    {
        return Result(ResultStatus::Info, std::move(message));
    }

    /*
        @summary
        Creates a warning result without a payload.

        @param message
        Warning text describing the condition.
    */
    static Result warning(std::string message)
    {
        return Result(ResultStatus::Warning, std::move(message));
    }

    /*
        @summary
        Returns the current status of the result.
    */
    constexpr ResultStatus status() const noexcept
    {
        return status_;
    }

    /*
        @summary
        Checks whether the operation completed successfully.
    */
    constexpr bool succeeded() const noexcept
    {
        return status_ == ResultStatus::Success;
    }

    /*
        @summary
        Checks whether the operation failed.
    */
    constexpr bool failed() const noexcept
    {
        return status_ == ResultStatus::Failure;
    }

    /*
        @summary
        Returns the message associated with the result.
    */
    const std::string& message() const noexcept
    {
        return message_;
    }

private:
    Result(ResultStatus status, std::string message)
        : status_(status), message_(std::move(message))
    {
    }

    ResultStatus status_;
    std::string message_;
};

}
