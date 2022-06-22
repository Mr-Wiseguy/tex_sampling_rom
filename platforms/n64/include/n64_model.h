#ifndef __N64_MODEL_H__
#define __N64_MODEL_H__

#include <array>
#include <cstdint>
#include <memory>

#include <ultra64.h>
#include <glm/glm.hpp>

#include <gfx.h>
#include <material_flags.h>

template <typename T>
T add_offset(T input, void *base_addr)
{
    return (T)((uintptr_t)input + (uintptr_t)base_addr);
}

// Header before material data
// Contains info about how to interpret the data that follows into a material DL
struct MaterialHeader {
    MaterialFlags flags;
    uint8_t gfx_length;
    // Automatic alignment padding
    Gfx *gfx;

    Gfx *setup_gfx(Gfx *gfx_pos, char const* const* images);
};

// A sequence of vertices to load from a model's vertex list
struct VertexLoad {
    uint16_t start;
    uint8_t count;
    uint8_t buffer_offset;
};

using TriangleIndices = std::array<uint8_t, 3>;

// A vertex load and the corresponding triangles to draw after the load
struct TriangleGroup {
    VertexLoad load;
    uint32_t num_tris;
    TriangleIndices *triangles;

    void adjust_offsets(void *base_addr);
};

// A collection of successive triangle groups that share a material
struct MaterialDraw {
    uint16_t num_groups;
    uint16_t material_index;
    TriangleGroup *groups;
    Gfx* gfx;

    void adjust_offsets(void *base_addr);
    size_t gfx_length() const;
    Gfx *setup_gfx(Gfx* gfx_pos, Vtx *verts) const;
};

// A submesh of a joint for a single draw layer
// Contains some number of material draws
struct JointMeshLayer {
    uint32_t num_draws;
    MaterialDraw *draws;

    void adjust_offsets(void *base_addr);
    size_t gfx_length() const;
};

// One joint (aka bone) of a mesh
// Contains one submesh for each draw layer
struct Joint {
    float posX; // Base positional offset x component (float to save conversion time later on)
    float posY; // Base positional offset y component
    float posZ; // Base positional offset z component
    uint8_t parent;
    uint8_t reserved; // Would be automatically added for alignment
    uint16_t reserved2; // Ditto
    JointMeshLayer layers[gfx::draw_layers];

    void adjust_offsets(void *base_addr);
    size_t gfx_length() const;
};

struct TextureParams {
    uint16_t image_index;
    uint16_t image_width;
    uint16_t image_height;
    uint16_t tmem_word_address;
    uint8_t image_format; // upper 4 bits are type, lower 4 bits are size
    uint8_t clamp_wrap_mirror; // upper 4 bits are t, lower 4 bits are s
    uint8_t mask_shift_s; // upper 4 bits are mask, lower 4 bits are shift
    uint8_t mask_shift_t; // upper 4 bits are mask, lower 4 bits are shift
};

struct Model {
    uint16_t num_joints;
    uint16_t num_materials;
    uint16_t num_images;
    uint16_t padding; // Would be automatically added for alignment
    Joint *joints;
    MaterialHeader **materials; // pointer to array of material pointers
    Vtx *verts; // pointer to all vertices
    char** images; // pointer to array of image paths
    std::unique_ptr<Gfx[]> gfx;

    void adjust_offsets();
    size_t gfx_length() const;
    void setup_gfx();
};

struct JointTable {
    uint32_t flags; // Flags to specify which channels are encoded in this joint's animation data
    int16_t *channels; // Segmented pointer to the array of all channel data

    void adjust_offsets(void *base_addr);
};

struct AnimTrigger {
    uint32_t frame; // The frame at which this trigger should run
    void (*triggerCb)(Model* model, uint32_t frame); // The callback to run at the specified frame
};

struct Animation {
    uint16_t frameCount; // The number of frames of data this animation has
    uint8_t jointCount; // Number of joints this animation has data for
    uint8_t flags; // Flags for this animation
    JointTable *jointTables; // Pointer to the array of joint animation tables
    AnimTrigger *triggers; // Pointer to the array of triggers for this animation

    void adjust_offsets();
};

#endif
