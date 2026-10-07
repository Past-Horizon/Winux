/*
MIT License — Copyright (c) 2026 Past-Horizon
See LICENSE file in the project root for full license text.

============================================================

██╗    ██╗██╗███╗   ██╗██╗   ██╗██╗  ██╗
██║    ██║██║████╗  ██║██║   ██║╚██╗██╔╝
██║ █╗ ██║██║██╔██╗ ██║██║   ██║ ╚███╔╝
██║███╗██║██║██║╚██╗██║██║   ██║ ██╔██╗
╚███╔███╔╝██║██║ ╚████║╚██████╔╝██╔╝ ██╗
 ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═╝

Cross-platform C++ system APIs for Windows and Linux.

============================================================
*/

#pragma once

#ifdef _WIN32
#include <Winux/Platform/Windows/WindowsMacroCleanup.h> // Undef common windows.h macros so it doesn't clash with our methods
#endif
//////////////////////////////////////
#include <Winux/Contracts/Capabilities.h>
#include <Winux/Contracts/IEnvironment.h>
#include <Winux/Contracts/ICrypto.h>
#include <Winux/Contracts/ILocale.h>
#include <Winux/Contracts/IMutex.h>
#include <Winux/Contracts/IMemory.h>
#include <Winux/Contracts/INetwork.h>
#include <Winux/Contracts/IProcess.h>
#include <Winux/Contracts/ISystem.h>
#include <Winux/Contracts/ITerminal.h>
//////////////////////////////////////
#include <Winux/Platform/Platform.h>
#include <Winux/Utils/Logger.h>
#include <Winux/Utils/Strings.h>
//////////////////////////////////////

#include <type_traits>

// We recommend not including individual Winux headers for public usage.
// Winux reserves the right to change internal headers, paths, names, signatures,
// and other implementation details without preserving compatibility.
//
// Use <Winux/Winux.h> for public usage. This header exposes the public Winux API
// and is the compatibility boundary for applications and libraries using Winux.
//
// Internal APIs may change or be removed without notice.
namespace Winux {

using Crypto = Contracts::ICrypto;

using CapabilitySet = Contracts::CapabilitySet;

using Environment = Contracts::IEnvironment;

using FileSystem = Contracts::IFileSystem;

using Locale = Contracts::ILocale;

using Mutex = Contracts::IMutex;

using Memory = Contracts::IMemory;

using Network = Contracts::INetwork;

using NetworkRequest = Contracts::INetwork::Request;

using NetworkResponse = Contracts::INetwork::Response;

using Process = Contracts::IProcess;

using PlatformContext = Contracts::IPlatform;

using System = Contracts::ISystem;

using Terminal = Contracts::ITerminal;

using AppDataResult = Contracts::IFileSystem::AppDataResult;

using AppDataScope = Contracts::AppDataScope;

using CanExecuteHandler = Contracts::ITerminal::CanExecuteHandler;

using CreateProcessResult = Contracts::IProcess::CreateProcessResult;

using ExecuteHandler = Contracts::ITerminal::ExecuteHandler;

using Sha256Digest = Contracts::ICrypto::Sha256Digest;

using Core::Result;

using Core::ResultStatus;

/*
    @summary
    Retrieves a service from the initialized platform by its flattened type.

    @param Service
    A supported service type exported by Winux.h.

    @returns
    A reference to the service owned by the initialized platform.

    @warning
    Call Platform::Initialize() exactly once before requesting a service.

    @related Platform::Initialize
*/
template <typename Service>
Service& Get()
{
    static_assert(!std::is_same_v<Service, Service>, "Unsupported Winux platform service type");
}

template <> Crypto& Get<Crypto>();
template <> Environment& Get<Environment>();
template <> FileSystem& Get<FileSystem>();
template <> Locale& Get<Locale>();
template <> Memory& Get<Memory>();
template <> Network& Get<Network>();
template <> Process& Get<Process>();
template <> PlatformContext& Get<PlatformContext>();
template <> System& Get<System>();
template <> Terminal& Get<Terminal>();

}