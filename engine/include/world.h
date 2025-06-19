#pragma once
#include <vector>

#include "types.h"
#include "model.h"

namespace voxel {
    class world {
    public:
        struct placed_model {
            model model;
            ivec3 position;
        };

        void add_model(const model& model, const ivec3& position);
        void remove_model(size_t index);
        
        // Методы для рендеринга
        const std::vector<placed_model>& get_models() const { return models_; }
        size_t get_model_count() const { return models_.size(); }
        const placed_model& get_model(size_t index) const { return models_[index]; }
        
        // Очистка мира
        void clear() { models_.clear(); }
        
    private:
        std::vector<placed_model> models_;
    };
} 