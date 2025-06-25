#include "spatial_partitionning.hpp"
#include <unordered_set>


/*______________ Frustum Culling Utility Functions ______________*/

bool is_entity_visible(const vec3& position, float radius, const mat4& view_proj) {
    vec4 clip = view_proj * vec4(position, 1.0f);
    if (clip.w <= 0.0f) return false; // Behind camera

    vec3 ndc = vec3(clip.x / clip.w, clip.y / clip.w, clip.z / clip.w);

    float augmented_radius = radius + 1.0f; // in case of shaders being on the frustum culling

    // Add radius to account for entity size
    return ndc.x >= -1.0f - augmented_radius && ndc.x <= 1.0f + augmented_radius &&
        ndc.y >= -1.0f - augmented_radius && ndc.y <= 1.0f + augmented_radius &&
        ndc.z >= -1.0f - augmented_radius && ndc.z <= 1.0f + augmented_radius;
}

/*______________ Spatial Grid System ______________*/

int SpatialGrid::getCellKey(float x, float y) const {
    int cellX = static_cast<int>(std::floor(x / cellSize));
    int cellY = static_cast<int>(std::floor(y / cellSize));
    return (cellX >= cellY) ? (cellX * cellX + cellX + cellY) : (cellY * cellY + cellX);
}

void SpatialGrid::build(const std::vector<entity*>& allEntities) {
    grid.clear();
    for (entity* e : allEntities) {
        int key = getCellKey(e->position.x, e->position.y);
        if (e->type == "tree")
            grid[key].treeEntities.push_back(e);
        else if (e->type == "cactus")
            grid[key].cactusEntities.push_back(e);
        else if (e->type == "grass")
            grid[key].grassEntities.push_back(e);
        else if (e->type == "checkpoint")
            grid[key].checkpointEntities.push_back(e);
    }
}

void SpatialGrid::query(const vec3& pos, float radius,
    std::vector<entity*>& trees,
    std::vector<entity*>& cactus,
    std::vector<entity*>& grass,
    std::vector<entity*>& checkpoints) const {
    trees.clear(); cactus.clear(); grass.clear(); checkpoints.clear();
    int minX = int(std::floor((pos.x - radius) / cellSize));
    int maxX = int(std::floor((pos.x + radius) / cellSize));
    int minY = int(std::floor((pos.y - radius) / cellSize));
    int maxY = int(std::floor((pos.y + radius) / cellSize));
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            int key = getCellKey(x * cellSize, y * cellSize);
            auto it = grid.find(key);
            if (it == grid.end()) continue;
            const Cell& c = it->second;
            trees.insert(trees.end(), c.treeEntities.begin(), c.treeEntities.end());
            cactus.insert(cactus.end(), c.cactusEntities.begin(), c.cactusEntities.end());
            grass.insert(grass.end(), c.grassEntities.begin(), c.grassEntities.end());
            checkpoints.insert(checkpoints.end(), c.checkpointEntities.begin(), c.checkpointEntities.end());
        }
    }
}

void SpatialGrid::getVisibleInstances(const vec3& pos, float radius,
    std::vector<int>& visibleTreeIndices,
    std::vector<int>& visibleCactusIndices,
    std::vector<int>& visibleGrassIndices,
    std::vector<int>& visibleCheckpointIndices,
    const numarray<vec3>& treePositions,
    const numarray<vec3>& cactusPositions,
    const numarray<vec3>& grassPositions,
    const numarray<vec3>& checkpointPositions, mat4 view_proj) const {

    visibleTreeIndices.clear();
    visibleCactusIndices.clear();
    visibleGrassIndices.clear();
    visibleCheckpointIndices.clear();

    int minX = int(std::floor((pos.x - radius) / cellSize));
    int maxX = int(std::floor((pos.x + radius) / cellSize));
    int minY = int(std::floor((pos.y - radius) / cellSize));
    int maxY = int(std::floor((pos.y + radius) / cellSize));

    // Process each cell in the query area
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            int key = getCellKey(x * cellSize, y * cellSize);
            auto it = grid.find(key);
            if (it == grid.end()) continue;

            // Process trees
            for (entity* e : it->second.treeEntities) {
                // Check if the entity is within the view frustum before adding
                if (!is_entity_visible(e->position, e->box.radius, view_proj)) {
                    continue;
                }

                // Find the index by matching position
                for (int i = 0; i < treePositions.size(); ++i) {
                    if (norm(e->position - treePositions[i]) < 0.1f) {  // Small epsilon for floating point comparison
                        visibleTreeIndices.push_back(i);
                        break;
                    }
                }
            }

            // Process cactuss
            for (entity* e : it->second.cactusEntities) {
                // Check if the entity is within the view frustum before adding
                if (!is_entity_visible(e->position, e->box.radius, view_proj)) {
                    continue;
                }

                // Find the index by matching position
                for (int i = 0; i < cactusPositions.size(); ++i) {
                    if (norm(e->position - cactusPositions[i]) < 0.1f) {  // Small epsilon for floating point comparison
                        visibleCactusIndices.push_back(i);
                        break;
                    }
                }
            }

            // Process grass
            for (entity* e : it->second.grassEntities) {
                // Check if the entity is within the view frustum before adding
                if (!is_entity_visible(e->position, e->box.radius, view_proj)) {
                    continue;
                }

                // Find the index by matching position
                for (int i = 0; i < grassPositions.size(); ++i) {
                    if (norm(e->position - grassPositions[i]) < 0.1f) {
                        visibleGrassIndices.push_back(i);
                        break;
                    }
                }
            }

            // Process checkpoints
            for (entity* e : it->second.checkpointEntities) {
                // Check if the entity is within the view frustum before adding
                if (!is_entity_visible(e->position, e->box.radius, view_proj)) {
                    continue;
                }

                // Find the index by matching position
                for (int i = 0; i < checkpointPositions.size(); ++i) {
                    if (norm(e->position - checkpointPositions[i]) < 0.1f) {
                        visibleCheckpointIndices.push_back(i);
                        break;
                    }
                }
            }
        }
    }
}


/*______________ Zone-Based System (Custom Zone) ______________*/

bool CustomZone::vec3_less(const vec3& a, const vec3& b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    return a.z < b.z;
}

int CustomZone::identifyZoneFromColor(const vec3& color) const {
    auto it = colorToZone.find(color);
    return (it != colorToZone.end()) ? it->second : -1;
}

void CustomZone::build(const std::vector<entity*>& allEntities) {
    zones.clear();
    adjacencyList.clear();

    for (entity* e : allEntities) {
        int zone = identifyZoneFromColor(e->position);
        if (zone < 0) continue;
        if (e->type == "tree")
            zones[zone].treeEntities.push_back(e);
        else if (e->type == "cactus")
            zones[zone].cactusEntities.push_back(e);
        else if (e->type == "grass")
            zones[zone].grassEntities.push_back(e);
        else if (e->type == "checkpoint")
            zones[zone].checkpointEntities.push_back(e);
    }
    for (const auto& z : zones) {
        int zid = z.first;
        adjacencyList[zid] = { zid - 1, zid + 1 };
    }
}

void CustomZone::query(const vec3& position, float radius,
    std::vector<entity*>& trees,
    std::vector<entity*>& cactus,
    std::vector<entity*>& grass,
    std::vector<entity*>& checkpoints) const {
    int zone = identifyZoneFromColor(position);

    // First add entities from current zone
    auto it = zones.find(zone);
    if (it != zones.end()) {
        trees = it->second.treeEntities;
        cactus = it->second.cactusEntities;
        grass = it->second.grassEntities;
        checkpoints = it->second.checkpointEntities;
    }

    // Then check adjacent zones if we're near borders (only if radius > 0)
    if (radius > 0) {
        auto adjIt = adjacencyList.find(zone);
        if (adjIt != adjacencyList.end()) {
            for (int adjZone : adjIt->second) {
                auto zoneIt = zones.find(adjZone);
                if (zoneIt == zones.end()) continue;

                trees.insert(trees.end(), zoneIt->second.treeEntities.begin(), zoneIt->second.treeEntities.end());
                cactus.insert(cactus.end(), zoneIt->second.cactusEntities.begin(), zoneIt->second.cactusEntities.end());
                grass.insert(grass.end(), zoneIt->second.grassEntities.begin(), zoneIt->second.grassEntities.end());
                checkpoints.insert(checkpoints.end(), zoneIt->second.checkpointEntities.begin(), zoneIt->second.checkpointEntities.end());
            }
        }
    }
}

void CustomZone::getVisibleInstances(const vec3& position, float radius, std::vector<int>& visibleTreeIndices,std::vector<int>& visibleCactusIndices, std::vector<int>& visibleGrassIndices, std::vector<int>& visibleCheckpointIndices,
    const numarray<vec3>& treePositions, const numarray<vec3>& cactusPositions, const numarray<vec3>& grassPositions, const numarray<vec3>& checkpointPositions, mat4 view_proj) const{
    
    visibleTreeIndices.clear();
    visibleCactusIndices.clear();
    visibleGrassIndices.clear();
    visibleCheckpointIndices.clear();

    int currentZone = identifyZoneFromColor(position);

    std::unordered_set<int> toVisit = { currentZone };
    auto itAdj = adjacencyList.find(currentZone);
    if (itAdj != adjacencyList.end()) {
        toVisit.insert(itAdj->second.begin(), itAdj->second.end());
    }

    for (int zid : toVisit) {
        auto zit = zones.find(zid);
        if (zit == zones.end()) continue;

        // Process trees
        for (entity* e : zit->second.treeEntities) {
            // Check if the entity is within the view frustum before adding
            if (!is_entity_visible(e->position, e->box.radius, view_proj)) {
                continue;
            }

            // Find the index by matching position
            for (int i = 0; i < treePositions.size(); ++i) {
                if (norm(e->position - treePositions[i]) < 0.1f) {
                    visibleTreeIndices.push_back(i);
                    break;
                }
            }
        }
        // Process cactuss
        for (entity* e : zit->second.cactusEntities) {
            // Check if the entity is within the view frustum before adding
            if (!is_entity_visible(e->position, e->box.radius, view_proj)) {
                continue;
            }

            // Find the index by matching position
            for (int i = 0; i < cactusPositions.size(); ++i) {
                if (norm(e->position - cactusPositions[i]) < 0.1f) {
                    visibleCactusIndices.push_back(i);
                    break;
                }
            }
        }

        // Process grass
        for (entity* e : zit->second.grassEntities) {
            // Check if the entity is within the view frustum before adding
            if (!is_entity_visible(e->position, e->box.radius, view_proj)) {
                continue;
            }

            // Find the index by matching position
            for (int i = 0; i < grassPositions.size(); ++i) {
                if (norm(e->position - grassPositions[i]) < 0.1f) {
                    visibleGrassIndices.push_back(i);
                    break;
                }
            }
        }

        // Process checkpoints
        for (entity* e : zit->second.checkpointEntities) {
            // Check if the entity is within the view frustum before adding
            if (!is_entity_visible(e->position, e->box.radius, view_proj)) {
                continue;
            }
            // Find the index by matching position
            for (int i = 0; i < checkpointPositions.size(); ++i) {
                if (norm(e->position - checkpointPositions[i]) < 0.1f) {
                    visibleCheckpointIndices.push_back(i);
                    break;
                }
            }
        }
    }
}
