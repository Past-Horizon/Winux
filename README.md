# Winux — Cross-Platform Tooling

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Winux is a lightweight C++ library for common cross-platform application tooling.

It provides a simple API over platform-specific things such as processes and file systems, so applications can use the same interface across supported platforms.

## Design Goals

Winux is built around a few simple goals:

* **Cross-platform** : provide a consistent API across operating systems.
* **Lightweight** : keep the library small without unnecessary dependencies or features.
* **Embeddable** : make Winux easy to include in larger projects.
* **Simple** : provide straightforward APIs without hiding everything behind excessive abstractions.

## Platform Support

| Platform | Status     | Notes                      |
| -------- | ---------- | -------------------------- |
| Windows  | ✅ Mature   | Primary development target |
| Linux    | 🟡 Less Mature | Under active development   |


## Features

Winux currently provides tooling for:

* Process management
* File system operations
* Result/error handling
* And more as development continues

The goal is to keep adding useful application-level functionality without turning Winux into a large framework.

## Quick Example

```cpp
#include <Winux/Winux.h>

#include <filesystem>
#include <iostream>
#include <string>

template <typename Value>
bool Check(const char* operation, const Winux::Core::Result<Value>& result)
{
	if (result.succeeded())
	{
		return true;
	}

	std::cerr << operation << " failed: " << result.message() << '\n';
	return false;
}

int main()
{
	auto platform = Winux::Platform::create(); // creates for current platform (for example Windows)

	auto& fileSystem = platform->file_system();
	auto& environment = platform->environment();
	auto& process = platform->process();
	auto& system = platform->system();

	const auto temporaryDirectory = fileSystem.temp();
	if (!Check("Finding the temporary directory", temporaryDirectory))
	{
		return 1;
	}

	const std::filesystem::path source =
		temporaryDirectory.value() / "winux-example-source.txt";
	const std::filesystem::path destination =
		temporaryDirectory.value() / "winux-example-destination.txt";
	std::error_code cleanupError;
	std::filesystem::remove(source, cleanupError);
	std::filesystem::remove(destination, cleanupError);

	if (!Check("Writing a file", fileSystem.write_file(source, "Hello from Winux")))
	{
		return 1;
	}

	const auto contents = fileSystem.read_file(source);
	if (!Check("Reading a file", contents))
	{
		return 1;
	}
	std::cout << contents.value() << '\n';

	if (!Check("Moving a file", fileSystem.move_file(source, destination)))
	{
		return 1;
	}

	const auto username = system.get_username();
	if (Check("Finding the current user", username))
	{
		std::wcout << L"User: " << username.value() << L'\n';
	}

	constexpr auto variableName = L"WINUX_EXAMPLE_VARIABLE";
	if (!Check("Setting an environment variable",
		environment.set_env(variableName, L"cross-platform")))
	{
		return 1;
	}

	const auto variable = environment.get_env(variableName);
	if (Check("Reading an environment variable", variable))
	{
		std::wcout << L"Variable: " << variable.value() << L'\n';
	}
	environment.unset_env(variableName);

	const auto executableDirectory = process.get_executable_directory();
	if (Check("Finding the executable directory", executableDirectory))
	{
		std::cout << "Executable directory: "
				  << executableDirectory.value().string() << '\n';
	}

	std::filesystem::remove(destination, cleanupError);
	return 0;
}
```

The same exact code also runs on Linux, or Windows; needing no changes to be able to run it on other platforms.

## Why Winux?

Cross-platform C++ applications often end up with platform-specific code scattered throughout the project.

Winux provides a common layer for frequently needed system functionality, keeping platform-specific implementation inside Winux while application code works with a consistent API.

## History

Winux started as a small utility layer for my other projects.

As more platform-specific functionality was needed, it grew into its own reusable project so other applications could use the same tooling stuff.

## Vision

Winux aims to provide a small, reliable foundation for common system-level tasks in cross-platform C++ applications.

It isn't intended to replace large platform frameworks. The goal is to handle the repetitive platform-specific parts while staying lightweight and easy to embed.

## Documentation

You can review project documentation inside given header files, following DocStandard.md format.

## License

Winux uses the MIT license. See [LICENSE](LICENSE) for details.
