#pragma once

#include <Winux/Contracts/ICrypto.h>
#include <Winux/Core/Results.h>

#include <cstddef>
#include <span>

namespace Winux::Platform {

Core::Result<void> FillRandom(std::span<std::byte> output);
Core::Result<Contracts::ICrypto::Sha256Digest> Sha256(std::span<const std::byte> input);
Core::Result<Contracts::ICrypto::Sha256Digest> HmacSha256(
	std::span<const std::byte> key,
	std::span<const std::byte> input);
Core::Result<void> HkdfSha256(
	std::span<const std::byte> input_key_material,
	std::span<const std::byte> salt,
	std::span<const std::byte> info,
	std::span<std::byte> output);

}