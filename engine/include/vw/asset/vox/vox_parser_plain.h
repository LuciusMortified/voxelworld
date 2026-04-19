#pragma once

#ifndef VW_ASSET_VOX_PARSER_PLAIN_H
#define VW_ASSET_VOX_PARSER_PLAIN_H

#include <optional>
#include <sstream>

#include "vw/core/block_registry.h"
#include "vw/asset/vox/vox_parser.h"

namespace vw::asset {

/// Plain text format .vox parser.
class vox_parser_plain final : public vox_parser {
public:
    explicit vox_parser_plain(const block_registry& block_registry);

    auto parse(const std::filesystem::path& filepath)
        -> std::expected<vox_prefab_data, error_type> override;

private:
    void process_root_(std::istringstream& iss);
    void process_entity_(std::istringstream& iss);
    void process_parent_(std::istringstream& iss);
    void process_transform_(std::istringstream& iss);
    void process_target_(std::istringstream& iss);
    void process_sockets_();
    void process_socket_(std::istringstream& iss);
    void process_model_(std::istringstream& iss);
    void process_voxel_(std::istringstream& iss);

    const block_registry* block_registry_;
    vox_prefab_data prefab_;
    vox_entity_data* current_entity_ = nullptr;
    std::optional<error_type> error_;
};

}  // namespace vw::asset

#include "vox_parser_plain.inl.h"

#endif  // VW_ASSET_VOX_PARSER_PLAIN_H
