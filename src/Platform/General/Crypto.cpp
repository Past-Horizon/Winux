#include <Winux/Platform/General/Crypto.h>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace Winux::Platform {
namespace {

class RandomContexts
{
public:
    RandomContexts()
    {
        mbedtls_entropy_init(&entropy_);
        mbedtls_ctr_drbg_init(&generator_);
    }

    ~RandomContexts()
    {
        mbedtls_ctr_drbg_free(&generator_);
        mbedtls_entropy_free(&entropy_);
    }

    RandomContexts(const RandomContexts&) = delete;
    RandomContexts& operator=(const RandomContexts&) = delete;

    int Seed()
    {
        if (seeded_)
        {
            return 0;
        }

        static constexpr unsigned char personalization[] = "Winux ICrypto";
        const int result = mbedtls_ctr_drbg_seed(
            &generator_,
            mbedtls_entropy_func,
            &entropy_,
            personalization,
            sizeof(personalization) - 1);
        seeded_ = result == 0;
        return result;
    }

    mbedtls_entropy_context entropy_{};
    mbedtls_ctr_drbg_context generator_{};

private:
    bool seeded_ = false;
};

template <typename Value>
Core::Result<Value> ProviderFailure(const char* operation, int error)
{
    return Core::Result<Value>::Failure(
        std::string("Mbed TLS ") + operation + " Failed (error " + std::to_string(error) + ")");
}

const unsigned char* InputData(std::span<const std::byte> input)
{
    static constexpr unsigned char empty_input = 0;
    return input.empty()
        ? &empty_input
        : reinterpret_cast<const unsigned char*>(input.data());
}

const mbedtls_md_info_t* Sha256Info()
{
    return mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
}

}

Core::Result<void> FillRandom(std::span<std::byte> output)
{
    if (output.empty())
    {
        return Core::Result<void>::Success();
    }

    static thread_local RandomContexts contexts;
    const int seed_result = contexts.Seed();
    if (seed_result != 0)
    {
        return ProviderFailure<void>("entropy initialization", seed_result);
    }

    for (std::size_t offset = 0; offset < output.size();)
    {
        const std::size_t chunk_size = std::min<std::size_t>(
            MBEDTLS_CTR_DRBG_MAX_REQUEST,
            output.size() - offset);
        const int random_result = mbedtls_ctr_drbg_random(
            &contexts.generator_,
            reinterpret_cast<unsigned char*>(output.data() + offset),
            chunk_size);
        if (random_result != 0)
        {
            return ProviderFailure<void>("random generation", random_result);
        }
        offset += chunk_size;
    }

    return Core::Result<void>::Success();
}

Core::Result<Contracts::ICrypto::Sha256Digest> Sha256(std::span<const std::byte> input)
{
    const mbedtls_md_info_t* info = Sha256Info();
    if (info == nullptr)
    {
        return Core::Result<Contracts::ICrypto::Sha256Digest>::Failure(
            "Mbed TLS returned no SHA-256 metadata.");
    }

    Contracts::ICrypto::Sha256Digest digest{};
    const int result = mbedtls_md(
        info,
        InputData(input),
        input.size(),
        reinterpret_cast<unsigned char*>(digest.data()));
    if (result != 0)
    {
        return ProviderFailure<Contracts::ICrypto::Sha256Digest>("SHA-256", result);
    }

    return Core::Result<Contracts::ICrypto::Sha256Digest>::Success(digest);
}

Core::Result<Contracts::ICrypto::Sha256Digest> HmacSha256(
    std::span<const std::byte> key,
    std::span<const std::byte> input)
{
    const mbedtls_md_info_t* info = Sha256Info();
    if (info == nullptr)
    {
        return Core::Result<Contracts::ICrypto::Sha256Digest>::Failure(
            "Mbed TLS returned no SHA-256 metadata.");
    }

    Contracts::ICrypto::Sha256Digest digest{};
    const int result = mbedtls_md_hmac(
        info,
        InputData(key),
        key.size(),
        InputData(input),
        input.size(),
        reinterpret_cast<unsigned char*>(digest.data()));
    if (result != 0)
    {
        return ProviderFailure<Contracts::ICrypto::Sha256Digest>("HMAC-SHA-256", result);
    }

    return Core::Result<Contracts::ICrypto::Sha256Digest>::Success(digest);
}

Core::Result<void> HkdfSha256(
    std::span<const std::byte> input_key_material,
    std::span<const std::byte> salt,
    std::span<const std::byte> info,
    std::span<std::byte> output)
{
    constexpr std::size_t maximum_output_size = 255 * 32;
    if (output.size() > maximum_output_size)
    {
        return Core::Result<void>::Failure("HKDF-SHA-256 output cannot exceed 8160 bytes.");
    }
    if (output.empty())
    {
        return Core::Result<void>::Success();
    }

    const mbedtls_md_info_t* md_info = Sha256Info();
    if (md_info == nullptr)
    {
        return Core::Result<void>::Failure("Mbed TLS returned no SHA-256 metadata.");
    }

    const int result = mbedtls_hkdf(
        md_info,
        InputData(salt),
        salt.size(),
        InputData(input_key_material),
        input_key_material.size(),
        InputData(info),
        info.size(),
        reinterpret_cast<unsigned char*>(output.data()),
        output.size());
    if (result != 0)
    {
        return ProviderFailure<void>("HKDF-SHA-256", result);
    }

    return Core::Result<void>::Success();
}

}

namespace Winux::Contracts {

Core::Result<void> ICrypto::FillRandom(std::span<std::byte> output)
{
    return Platform::FillRandom(output);
}

Core::Result<ICrypto::Sha256Digest> ICrypto::Sha256(std::span<const std::byte> input)
{
    return Platform::Sha256(input);
}

Core::Result<ICrypto::Sha256Digest> ICrypto::HmacSha256(
    std::span<const std::byte> key,
    std::span<const std::byte> input)
{
    return Platform::HmacSha256(key, input);
}

Core::Result<void> ICrypto::HkdfSha256(
    std::span<const std::byte> input_key_material,
    std::span<const std::byte> salt,
    std::span<const std::byte> info,
    std::span<std::byte> output)
{
    return Platform::HkdfSha256(input_key_material, salt, info, output);
}

}