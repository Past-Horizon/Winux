#pragma once

#include <Winux/Core/Results.h>

#include <array>
#include <cstddef>
#include <span>

namespace Winux::Contracts {

/*
    @summary
    Provides cryptographically secure random bytes from the platform crypto provider.
*/
class ICrypto {
public:
    /*
        @summary
        Stores a 32-byte SHA-256 digest.
    */
    using Sha256Digest = std::array<std::byte, 32>;

    virtual ~ICrypto() = default;

    /*
        @summary
        Fills the supplied buffer with cryptographically secure random bytes.

        @param output
        Buffer to fill; an empty buffer succeeds without modification.

        @returns
        Success when the entire buffer is filled, otherwise a failure describing the provider error.
    */
    virtual Core::Result<void> fill_random(std::span<std::byte> output) = 0;

    /*
        @summary
        Calculates the SHA-256 digest of the supplied data.

        @param input
        Data to hash.
    */
    virtual Core::Result<Sha256Digest> sha256(std::span<const std::byte> input) = 0;

    /*
        @summary
        Calculates an HMAC-SHA-256 authentication tag.

        @param key
        Secret key used to authenticate the data.

        @param input
        Data to authenticate.
    */
    virtual Core::Result<Sha256Digest> hmac_sha256(
        std::span<const std::byte> key,
        std::span<const std::byte> input) = 0;

    /*
        @summary
        Derives key material using HKDF-SHA-256.

        @param input_key_material
        Input keying material.

        @param salt
        Optional non-secret salt; an empty span selects HKDF's zero-filled default salt.

        @param info
        Optional application-specific context.

        @param output
        Destination for derived key material; it can contain at most 8160 bytes.
    */
    virtual Core::Result<void> hkdf_sha256(
        std::span<const std::byte> input_key_material,
        std::span<const std::byte> salt,
        std::span<const std::byte> info,
        std::span<std::byte> output) = 0;
};

}