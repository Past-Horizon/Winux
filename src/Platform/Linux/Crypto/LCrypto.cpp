#include <Winux/Platform/General/Crypto.h>
#include <Winux/Platform/Linux/Linux.h>

namespace Winux::Platform::Linux {

Contracts::ICrypto& Linux::crypto()
{
    return *this;
}

Core::Result<void> Linux::fill_random(std::span<std::byte> output)
{
    return Winux::Platform::fill_random(output);
}

Core::Result<Contracts::ICrypto::Sha256Digest> Linux::sha256(std::span<const std::byte> input)
{
    return Winux::Platform::sha256(input);
}

Core::Result<Contracts::ICrypto::Sha256Digest> Linux::hmac_sha256(
    std::span<const std::byte> key,
    std::span<const std::byte> input)
{
    return Winux::Platform::hmac_sha256(key, input);
}

Core::Result<void> Linux::hkdf_sha256(
    std::span<const std::byte> input_key_material,
    std::span<const std::byte> salt,
    std::span<const std::byte> info,
    std::span<std::byte> output)
{
    return Winux::Platform::hkdf_sha256(input_key_material, salt, info, output);
}

}