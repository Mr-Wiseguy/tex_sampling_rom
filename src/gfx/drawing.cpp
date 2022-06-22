#include <glm/glm.hpp>
#include <types.h>
#include <gfx.h>
#include <platform_gfx.h>
#include <mem.h>
#include <ecs.h>
#include <model.h>
#include <files.h>
#include <interaction.h>
#include <vassert.h>

#include <cmath>

void drawAnimatedModels(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model
    Vec3 *curPos = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_ANIM_MODEL)]);
    Vec3s *curRot = static_cast<Vec3s *>(componentArrays[COMPONENT_INDEX(Rotation, ARCHETYPE_ANIM_MODEL)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_ANIM_MODEL)]);
    AnimState *curAnimState = static_cast<AnimState *>(componentArrays[COMPONENT_INDEX(AnimState, ARCHETYPE_ANIM_MODEL)]);

    while (count)
    {
        Animation *anim = segmentedToVirtual(curAnimState->anim);

        gfx::load_pos_rot(*curPos, *curRot);
        drawModel(*curModel, anim, ANIM_COUNTER_TO_FRAME(curAnimState->counter));

        curAnimState->counter += curAnimState->speed;
#ifdef FPS30
        curAnimState->counter += curAnimState->speed;
#endif
        if (anim)
        {
            if (curAnimState->counter >= (anim->frameCount << (ANIM_COUNTER_SHIFT)))
            {
                if (anim->flags & ANIM_LOOP)
                {
                    curAnimState->counter -= (anim->frameCount << (ANIM_COUNTER_SHIFT));
                }
                else
                {
                    curAnimState->counter = (anim->frameCount - 1) << (ANIM_COUNTER_SHIFT);
                }
            }
        }

        count--;
        curPos++;
        curRot++;
        curModel++;
        curAnimState++;
    }
}

void drawModels(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model
    Vec3 *curPos = static_cast<Vec3 *>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_MODEL)]);
    Vec3s *curRot = static_cast<Vec3s *>(componentArrays[COMPONENT_INDEX(Rotation, ARCHETYPE_MODEL)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_MODEL)]);

    while (count)
    {
        gfx::load_pos_rot(*curPos, *curRot);
        drawModel(*curModel, nullptr, 0);
        count--;
        curPos++;
        curRot++;
        curModel++;
    }
}

void drawModelsNoRotation(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model
    Vec3 *curPos = static_cast<Vec3 *>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_MODEL_NO_ROTATION)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_MODEL_NO_ROTATION)]);

    while (count)
    {
        gfx::load_pos(*curPos);
        drawModel(*curModel, nullptr, 0);
        count--;
        curPos++;
        curModel++;
    }
}

void drawResizableAnimatedModels(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model, Resizable
    Vec3 *curPos = static_cast<Vec3 *>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_SCALED_ANIM_MODEL)]);
    Vec3s *curRot = static_cast<Vec3s *>(componentArrays[COMPONENT_INDEX(Rotation, ARCHETYPE_SCALED_ANIM_MODEL)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_SCALED_ANIM_MODEL)]);
    AnimState *curAnimState = static_cast<AnimState *>(componentArrays[COMPONENT_INDEX(AnimState, ARCHETYPE_SCALED_ANIM_MODEL)]);
    float *curScale = static_cast<float *>(componentArrays[COMPONENT_INDEX(Scale, ARCHETYPE_SCALED_ANIM_MODEL)]);

    while (count)
    {
        Animation *anim = segmentedToVirtual(curAnimState->anim);

        gfx::load_pos_rot(*curPos, *curRot);
        gfx::apply_scale_affine(*curScale, *curScale, *curScale);
        drawModel(*curModel, anim, ANIM_COUNTER_TO_FRAME(curAnimState->counter));

        curAnimState->counter += curAnimState->speed;
#ifdef FPS30
        curAnimState->counter += curAnimState->speed;
#endif
        if (anim)
        {
            if (curAnimState->counter >= (anim->frameCount << (ANIM_COUNTER_SHIFT)))
            {
                if (anim->flags & ANIM_LOOP)
                {
                    curAnimState->counter -= (anim->frameCount << (ANIM_COUNTER_SHIFT));
                }
                else
                {
                    curAnimState->counter = (anim->frameCount - 1) << (ANIM_COUNTER_SHIFT);
                }
            }
        }

        count--;
        curPos++;
        curRot++;
        curModel++;
        curAnimState++;
        curScale++;
    }
}

void drawResizableModels(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model, Resizable
    Vec3 *curPos = static_cast<Vec3 *>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_SCALED_MODEL)]);
    Vec3s *curRot = static_cast<Vec3s *>(componentArrays[COMPONENT_INDEX(Rotation, ARCHETYPE_SCALED_MODEL)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_SCALED_MODEL)]);
    float *curScale = static_cast<float *>(componentArrays[COMPONENT_INDEX(Scale, ARCHETYPE_SCALED_MODEL)]);

    while (count)
    {
        gfx::load_pos_rot(*curPos, *curRot);
        gfx::apply_scale_affine(*curScale, *curScale, *curScale);
        drawModel(*curModel, nullptr, 0);
        count--;
        curPos++;
        curRot++;
        curModel++;
        curScale++;
    }
}

void mtxf_align_camera(MtxF dest, MtxF mtx, Vec3 position, int16_t angle) {
    int16_t xrot;
    int16_t yrot;
    float cx, cy, cz;
    Vec3 colX, colY, colZ; // Column vectors
    float sx, sy, sz; // Scale
    MtxF scaleMat;

    dest[3][0] =
        mtx[0][0] * position[0] + mtx[1][0] * position[1] + mtx[2][0] * position[2] + mtx[3][0];
    dest[3][1] =
        mtx[0][1] * position[0] + mtx[1][1] * position[1] + mtx[2][1] * position[2] + mtx[3][1];
    dest[3][2] =
        mtx[0][2] * position[0] + mtx[1][2] * position[1] + mtx[2][2] * position[2] + mtx[3][2];
    dest[3][3] = 1;
    xrot = -atan2s(dest[3][2], dest[3][0]);
    yrot = atan2s(dest[3][2], dest[3][1]);
    cx = cossf(xrot);
    cy = cossf(yrot);
    cz = cossf(angle);

    colX[0] = mtx[0][0];
    colX[1] = mtx[1][0];
    colX[2] = mtx[2][0];

    colY[0] = mtx[0][1];
    colY[1] = mtx[1][1];
    colY[2] = mtx[2][1];

    colZ[0] = mtx[0][2];
    colZ[1] = mtx[1][2];
    colZ[2] = mtx[2][2];

    sx = std::sqrt(VEC3_DOT(colX, colX));
    sy = std::sqrt(VEC3_DOT(colY, colY));
    sz = std::sqrt(VEC3_DOT(colZ, colZ));

    guScaleF(scaleMat, sx, sy, sz);

    dest[2][0] = sinsf(xrot);
    dest[0][2] = -dest[2][0];
    dest[1][2] = sinsf(yrot);
    dest[2][1] = -dest[1][2];
    dest[0][1] = sinsf(angle);
    dest[1][0] = -dest[0][1];

    dest[0][0] = -cx * cz;
    dest[1][1] = -cy * cz;
    dest[2][2] = -cx * -cy;

    dest[0][3] = 0;
    dest[1][3] = 0;
    dest[2][3] = 0;
    
    mtxfMul(dest, dest, scaleMat);
}

void drawAllEntities()
{
    // Draw all non-resizable entities that have a model and no rotation or animation
    iterateOverEntities(drawModelsNoRotation, nullptr, ARCHETYPE_MODEL_NO_ROTATION, Bit_Rotation | Bit_AnimState | Bit_Scale);
    // Draw all non-resizable entities that have a model and no animation
    iterateOverEntities(drawModels, nullptr, ARCHETYPE_MODEL, Bit_AnimState | Bit_Scale);
    // Draw all non-resizable entities that have a model and an animation
    iterateOverEntities(drawAnimatedModels, nullptr, ARCHETYPE_ANIM_MODEL, Bit_Scale);
    // Draw all resizable entities that have a model and no animation
    iterateOverEntities(drawResizableModels, nullptr, ARCHETYPE_SCALED_MODEL, Bit_AnimState);
    // Draw all resizable entities that have a model and an animation
    iterateOverEntities(drawResizableAnimatedModels, nullptr, ARCHETYPE_SCALED_ANIM_MODEL, 0);
}

Model* head_model = nullptr;
Model* player_head_model = nullptr;

bool model_is_valid(Model* model) {
    if (model == nullptr) {
        return false;
    }
    if (model->num_joints == 0) {
        return false;
    }
    if (model->num_materials == 0) {
        return false;
    }
    return true;
}
