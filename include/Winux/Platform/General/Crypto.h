#pragma once

#include <Winux/Contracts/ICrypto.h>
#include <Winux/Core/Results.h>

#include <cstddef>
#include <span>

namespace Winux::Platform {

Core::Result<void> fill_random(std::span<std::byte> output);
Core::Result<Contracts::ICrypto::Sha256Digest> sha256(std::span<const std::byte> input);
Core::Result<Contracts::ICrypto::Sha256Digest> hmac_sha256(
	std::span<const std::byte> key,
	std::span<const std::byte> input);
Core::Result<void> hkdf_sha256(
	std::span<const std::byte> input_key_material,
	std::span<const std::byte> salt,
	std::span<const std::byte> info,
	std::span<std::byte> output);

}