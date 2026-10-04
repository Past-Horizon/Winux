#pragma once

#include <cstdint>
#include <unordered_map>

namespace Winux::Contracts {

using CapabilityId = std::uint64_t;

template <typename T>
struct Capability
{
    static constexpr CapabilityId Id = T::Id;
};

class CapabilitySet
{
public:
    template <typename T>
    bool Has() const noexcept
    {
        return capabilities_.contains(Capability<T>::Id);
    }

    bool ContainsAll(const CapabilitySet& other) const noexcept
    {
        for (const auto& capability : other.capabilities_)
        {
            if (!capabilities_.contains(capability.first))
            {
                return false;
            }
        }

        return true;
    }

    template <typename T>
    void Add()
    {
        capabilities_.try_emplace(Capability<T>::Id, nullptr);
    }

    template <typename T>
    void Add(T* capability)
    {
        capabilities_.insert_or_assign(Capability<T>::Id, capability);
    }

    template <typename T>
    T* TryGet() noexcept
    {
        const auto iterator = capabilities_.find(Capability<T>::Id);
        return iterator == capabilities_.end()
            ? nullptr
            : static_cast<T*>(iterator->second);
    }

    template <typename T>
    const T* TryGet() const noexcept
    {
        const auto iterator = capabilities_.find(Capability<T>::Id);
        return iterator == capabilities_.end()
            ? nullptr
            : static_cast<const T*>(iterator->second);
    }

private:
    std::unordered_map<CapabilityId, void*> capabilities_;
};

}