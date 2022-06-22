#ifndef __GFX_H__
#define __GFX_H__

#include <type_traits>

#include <config.h>
#include <types.h>
#include <mathutils.h>
#include <cstring>
#include <camera.h>

// Draw layers
enum class DrawLayer : unsigned int {
    background,
    opa_surf,
    // opa_inter,
    // opa_line,
    tex_edge,
    opa_decal,
    xlu_decal,
    xlu_surf,
    // xlu_inter,
    // xlu_line,
    count
};

constexpr unsigned int draw_layer_buffer_len = 32;

constexpr unsigned int matf_stack_len = 16;
constexpr unsigned int mat_buffer_len = 128;

extern MtxF *g_curMatFPtr;

void initGfx(void);

void startFrame(void);
void setupCameraMatrices(Camera *camera);
void setLightDirection(Vec3 lightDir);
void endFrame(void);

void drawModel(Model *toDraw, Animation* anim, uint32_t frame);
void drawAABB(DrawLayer layer, AABB *toDraw, uint32_t color);
void drawLine(DrawLayer layer, Vec3 start, Vec3 end, uint32_t color);

void drawAllEntities(void);

void shadeScreen(float alphaPercent);

float get_aspect_ratio();

extern "C" void guLookAtF(float mf[4][4], float xEye, float yEye, float zEye,
		      float xAt,  float yAt,  float zAt,
		      float xUp,  float yUp,  float zUp);
extern "C" void guPositionF(float mf[4][4], float r, float p, float h, float s,
			float x, float y, float z);
extern "C" void guTranslateF(float mf[4][4], float x, float y, float z);
extern "C" void guScaleF(float mf[4][4], float x, float y, float z);
extern "C" void guRotateF(float mf[4][4], float a, float x, float y, float z);
extern "C" void guMtxIdentF(float mf[4][4]);

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gfx
{
    constexpr unsigned int draw_layers = static_cast<unsigned int>(DrawLayer::count);

    inline NOINLINE void copy_mat(MtxF *dst, const MtxF *src)
    {
        float* srcPtr = (float*)(&(*src)[0][0]);
        float* dstPtr = (float*)(&(*dst)[0][0]);

        dstPtr[ 0] = srcPtr[ 0];
        dstPtr[ 1] = srcPtr[ 1];
        dstPtr[ 2] = srcPtr[ 2];
        dstPtr[ 3] = srcPtr[ 3];
        dstPtr[ 4] = srcPtr[ 4];
        dstPtr[ 5] = srcPtr[ 5];
        dstPtr[ 6] = srcPtr[ 6];
        dstPtr[ 7] = srcPtr[ 7];
        dstPtr[ 8] = srcPtr[ 8];
        dstPtr[ 9] = srcPtr[ 9];
        dstPtr[10] = srcPtr[10];
        dstPtr[11] = srcPtr[11];
        dstPtr[12] = srcPtr[12];
        dstPtr[13] = srcPtr[13];
        dstPtr[14] = srcPtr[14];
        dstPtr[15] = srcPtr[15];
    }

    inline NOINLINE void push_mat()
    {
        MtxF *nextMat = g_curMatFPtr + 1;
        gfx::copy_mat(nextMat, g_curMatFPtr);
        g_curMatFPtr++;
    }

    inline NOINLINE void load_mat(const MtxF *src)
    {
        gfx::copy_mat(g_curMatFPtr, src);
    }

    inline NOINLINE void push_load_mat(const MtxF *src)
    {
        g_curMatFPtr++;
        gfx::copy_mat(g_curMatFPtr, src);
    }

    inline NOINLINE void pop_mat()
    {
        g_curMatFPtr--;
    }

    inline NOINLINE void save_mat(MtxF *dst)
    {
        gfx::copy_mat(dst, g_curMatFPtr);
    }

    inline NOINLINE void load_identity()
    {
        guMtxIdentF(*g_curMatFPtr);
    }    

    inline NOINLINE uint16_t calc_perspnorm(float near, float far)
    {
        if (near + far <= 2.0f) {
            return 65535;
        }
        uint16_t ret = static_cast<int16_t>(float{1 << 17} / (near + far));
        if (ret <= 0) {
            ret = 1;
        }
        return ret;
    }

    void load_view_proj(Vec3 eye_pos, Camera *camera, float aspect, float near, float far, UNUSED float scale);

    inline NOINLINE void rotate_axis_angle(float angle, float axisX, float axisY, float axisZ)
    {
        // TODO affine matrix optimization
        MtxF tmp;
        guRotateF(tmp, angle, axisX, axisY, axisZ);
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, tmp);
    }

    inline NOINLINE void rotate_euler_xyz(int16_t rx, int16_t ry, int16_t rz)
    {
        MtxF tmp;
        mtxfEulerXYZ(tmp, rx, ry, rz);
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, tmp);
    }

    inline NOINLINE void apply_matrix(MtxF *mat)
    {
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, *mat);
    }

    inline NOINLINE void apply_translation_affine(float x, float y, float z)
    {
        MtxF& mat = *g_curMatFPtr;
        for (int i = 0; i < 3; i++)
        {
            mat[3][i] += mat[0][i] * x + mat[1][i] * y + mat[2][i] * z;
        }
    }

    inline NOINLINE void load_pos_rot(Vec3& pos, Vec3s& rot)
    {
        MtxF& mat = *g_curMatFPtr;
        mtxfEulerXYZ(mat, rot[0], rot[1], rot[2]);
        mat[3][0] = pos[0] - g_Camera.model_offset[0];
        mat[3][1] = pos[1] - g_Camera.model_offset[1];
        mat[3][2] = pos[2] - g_Camera.model_offset[2];
        mat[3][3] = 1.0f;
    }

    inline NOINLINE void load_pos(Vec3& pos)
    {
        MtxF& mat = *g_curMatFPtr;
        guMtxIdentF(mat);
        mat[3][0] = pos[0] - g_Camera.model_offset[0];
        mat[3][1] = pos[1] - g_Camera.model_offset[1];
        mat[3][2] = pos[2] - g_Camera.model_offset[2];
        mat[3][3] = 1.0f;
    }

    inline NOINLINE void apply_scale_affine(float sx, float sy, float sz)
    {
        MtxF& mat = *g_curMatFPtr;
        for (int i = 0; i < 3; i++)
        {
            mat[0][i] *= sx;
            mat[1][i] *= sy;
            mat[2][i] *= sz;
        }
    }

    inline NOINLINE void apply_position(float pitch, float rx, float ry, float rz, float x, float y, float z)
    {
        MtxF tmp;
        guPositionF(tmp, pitch, rx, ry, rz, x, y, z);
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, tmp);
    }
}

#endif