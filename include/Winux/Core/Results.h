#pragma once

#include <optional>
#include <string>
#include <utility>

namespace Winux::Core {

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
    static Result success(Value value, std::string message = {})
    {
        return Result(ResultStatus::Success, std::move(value), std::move(message));
    }

    static Result failure(std::string message)
    {
        return Result(ResultStatus::Failure, std::nullopt, std::move(message));
    }

    static Result info(Value value, std::string message = {})
    {
        return Result(ResultStatus::Info, std::move(value), std::move(message));
    }

    static Result warning(Value value, std::string message = {})
    {
        return Result(ResultStatus::Warning, std::move(value), std::move(message));
    }

    constexpr ResultStatus status() const noexcept
    {
        return status_;
    }

    constexpr bool succeeded() const noexcept
    {
        return status_ == ResultStatus::Success;
    }

    constexpr bool failed() const noexcept
    {
        return status_ == ResultStatus::Failure;
    }

    const Value& value() const
    {
        return value_.value();
    }

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
    static Result success(std::string message = {})
    {
        return Result(ResultStatus::Success, std::move(message));
    }

    static Result failure(std::string message)
    {
        return Result(ResultStatus::Failure, std::move(message));
    }

    static Result info(std::string message)
    {
        return Result(ResultStatus::Info, std::move(message));
    }

    static Result warning(std::string message)
    {
        return Result(ResultStatus::Warning, std::move(message));
    }

    constexpr ResultStatus status() const noexcept
    {
        return status_;
    }

    constexpr bool succeeded() const noexcept
    {
        return status_ == ResultStatus::Success;
    }

    constexpr bool failed() const noexcept
    {
        return status_ == ResultStatus::Failure;
    }

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
