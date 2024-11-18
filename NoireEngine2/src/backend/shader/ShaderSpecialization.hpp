#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Specialization
{
public:
    void Add(uint32_t constantID, int32_t value)
    {
        spec_values.push_back(value);
        VkSpecializationMapEntry entry;
        entry.constantID = constantID;
        entry.size = sizeof(int32_t);
        entry.offset = static_cast<uint32_t>(spec_entries.size() * sizeof(int32_t));
        spec_entries.emplace_back(entry);
    }

    void Add(const std::vector<std::pair<uint32_t, int32_t>>& const_values)
    {
        for (const auto& v : const_values)
            Add(v.first, v.second);
    }

    VkSpecializationInfo* GetSpecialization()
    {
        spec_info.dataSize = static_cast<uint32_t>(spec_values.size() * sizeof(int32_t));
        spec_info.pData = spec_values.data();
        spec_info.mapEntryCount = static_cast<uint32_t>(spec_entries.size());
        spec_info.pMapEntries = spec_entries.data();
        return &spec_info;
    }

private:
    std::vector<int32_t>                  spec_values;
    std::vector<VkSpecializationMapEntry> spec_entries;
    VkSpecializationInfo                  spec_info;
};