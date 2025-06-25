#pragma once
#include <vector>
#include <unordered_map>
#include "cgp/cgp.hpp"
#include "environment.hpp"

using namespace cgp;

/*______________ Spatial Partitioning Abstract Class ______________*/

struct SpatialPartitioning {
    virtual ~SpatialPartitioning() = default;

    virtual void build(const std::vector<entity*>& allEntities) = 0;

    virtual void query(const vec3& position, float radius,
        std::vector<entity*>& trees,
        std::vector<entity*>& cactus,
        std::vector<entity*>& grass,
        std::vector<entity*>& checkpoints) const = 0;

    virtual void getVisibleInstances(const vec3& pos, float radius,
        std::vector<int>& visibleTreeIndices,
        std::vector<int>& visibleCactusIndices,
        std::vector<int>& visibleGrassIndices,
        std::vector<int>& visibleCheckpointIndices,
        const numarray<vec3>& treePositions,
        const numarray<vec3>& cactusPositions,
        const numarray<vec3>& grassPositions,
        const numarray<vec3>& checkpointPositions, mat4 view_proj) const = 0;
};


/*______________ Spatial Grid System ______________*/

struct SpatialGrid : public SpatialPartitioning {
private:
    struct Cell {
        std::vector<entity*> treeEntities;
        std::vector<entity*> cactusEntities;
        std::vector<entity*> grassEntities;
        std::vector<entity*> checkpointEntities;
    };

    std::unordered_map<int, Cell> grid;
    float cellSize;

    int getCellKey(float x, float y) const;

public:
    SpatialGrid(float size = 20.0f) : cellSize(size) {}

    void build(const std::vector<entity*>& allEntities);
    void query(const vec3& pos, float radius, std::vector<entity*>& trees, std::vector<entity*>& cactus, std::vector<entity*>& grass, std::vector<entity*>& checkpoints) const;
    virtual void getVisibleInstances(const vec3& pos, float radius, std::vector<int>& visibleTreeIndices,std::vector<int>& visibleCactusIndices, std::vector<int>& visibleGrassIndices,
        std::vector<int>& visibleCheckpointIndices, const numarray<vec3>& treePositions, const numarray<vec3>& cactusPositions, const numarray<vec3>& grassPositions, const numarray<vec3>& checkpointPositions, mat4 view_proj) const;
    
};

/*______________ Zone-Based System (Custom Zone) ______________*/

struct CustomZone : public SpatialPartitioning {
private:
    struct Zone {
        std::vector<entity*> treeEntities;
        std::vector<entity*> cactusEntities;
        std::vector<entity*> grassEntities;
        std::vector<entity*> checkpointEntities;
    };

    std::unordered_map<int, Zone> zones;
    std::unordered_map<int, std::vector<int>> adjacencyList;

    std::map<vec3, int, bool(*)(const vec3&, const vec3&)> colorToZone;

    static bool vec3_less(const vec3& a, const vec3& b);

public:
    CustomZone(const std::map<vec3, int, bool(*)(const vec3&, const vec3&)>& colorMap): colorToZone(colorMap) {}

    int identifyZoneFromColor(const vec3& color) const;

    void build(const std::vector<entity*>& allEntities);
    void query(const vec3& position, float radius, std::vector<entity*>& trees,std::vector<entity*>& cactus, std::vector<entity*>& grass, std::vector<entity*>& checkpoints) const;
    void getVisibleInstances(const vec3& position, float radius, std::vector<int>& visibleTreeIndices, std::vector<int>& visibleCactusIndices, std::vector<int>& visibleGrassIndices, std::vector<int>& visibleCheckpointIndices,
        const numarray<vec3>& treePositions, const numarray<vec3>& cactusPositions, const numarray<vec3>& grassPositions, const numarray<vec3>& checkpointPositions, mat4 view_proj) const;
};

