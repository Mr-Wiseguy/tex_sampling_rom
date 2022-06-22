#include <mathutils.h>

float sintable[] = {
#include "sintable.h"
};

int16_t atan2sTable[0x401] = {
#include "atan2table.h"
};

extern "C" float sinf(float);
extern "C" float cosf(float);

extern "C" float tanf(float x)
{
    return sinf(x) / cosf(x);
}

// http://nghiaho.com/?p=997
float fastAtanf(float x)
{
    return M_PI_4*x - x*(ABS(x) - 1)*(0.2447f + 0.0663f*ABS(x));
}
// atan2approx from https://github.com/ducha-aiki/fast_atan2/blob/master/fast_atan.cpp
float fastAtan2f(float y, float x)
{
  float absx, absy;
  absy = ABS(y);
  absx = ABS(x);
  short octant = ((x<0) << 2) + ((y<0) << 1 ) + (absx <= absy);
  switch (octant) {
    case 0: {
        if (x == 0 && y == 0)
          return 0;
        float val = absy/absx;
        return (M_PI_4_P_0273 - 0.273f*val)*val; //1st octant
        break;
      }
    case 1:{
        if (x == 0 && y == 0)
          return 0.0;
        float val = absx/absy;
        return M_PIf_2 - (M_PI_4_P_0273 - 0.273f*val)*val; //2nd octant
        break;
      }
    case 2: {
        float val =absy/absx;
        return -(M_PI_4_P_0273 - 0.273f*val)*val; //8th octant
        break;
      }
    case 3: {
        float val =absx/absy;
        return -M_PIf_2 + (M_PI_4_P_0273 - 0.273f*val)*val;//7th octant
        break;
      }
    case 4: {
        float val =absy/absx;
        return  M_PIf - (M_PI_4_P_0273 - 0.273f*val)*val;  //4th octant
      }
    case 5: {
        float val =absx/absy;
        return  M_PIf_2 + (M_PI_4_P_0273 - 0.273f*val)*val;//3rd octant
        break;
      }
    case 6: {
        float val =absy/absx;
        return -M_PIf + (M_PI_4_P_0273 - 0.273f*val)*val; //5th octant
        break;
      }
    case 7: {
        float val =absx/absy;
        return -M_PIf_2 - (M_PI_4_P_0273 - 0.273f*val)*val; //6th octant
        break;
      }
    default:
      return 0.0;
    }
}

uint16_t atan2_lookup(float y, float x) {
    uint16_t ret;

    if (x == 0) {
        ret = atan2sTable[0];
    } else {
        ret = atan2sTable[(int)(y / x * 1024 + 0.5f)];
    }
    return ret;
}

int16_t atan2s(float y, float x) {
    uint16_t ret;

    if (x >= 0) {
        if (y >= 0) {
            if (y >= x) {
                ret = atan2_lookup(x, y);
            } else {
                ret = 0x4000 - atan2_lookup(y, x);
            }
        } else {
            y = -y;
            if (y < x) {
                ret = 0x4000 + atan2_lookup(y, x);
            } else {
                ret = 0x8000 - atan2_lookup(x, y);
            }
        }
    } else {
        x = -x;
        if (y < 0) {
            y = -y;
            if (y >= x) {
                ret = 0x8000 + atan2_lookup(x, y);
            } else {
                ret = 0xC000 - atan2_lookup(y, x);
            }
        } else {
            if (y < x) {
                ret = 0xC000 + atan2_lookup(y, x);
            } else {
                ret = -atan2_lookup(x, y);
            }
        }
    }
    return ret;
}

float sinsf(int16_t x)
{
    int index = ((uint16_t)x & 0x3FFF) >> 4;
    if (x & 0x4000) // Check if x is in quadrant 2 or 4
    {
        index = 0x400 - index;
    }
    if (x & 0x8000)
    {
        return -sintable[index];
    }
    return sintable[index];
}

#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gfx.h>

void mtxfMul(MtxF out, MtxF a, MtxF b)
{
    glm::mat4 *a_mat = reinterpret_cast<glm::mat4*>(&a[0][0]);
    glm::mat4 *b_mat = reinterpret_cast<glm::mat4*>(&b[0][0]);
    glm::mat4 *out_mat = reinterpret_cast<glm::mat4*>(&out[0][0]);
    *out_mat = *a_mat * *b_mat;
}

void mtxfEulerXYZ(MtxF out, int16_t rx, int16_t ry, int16_t rz)
{
    float s1 = sinsf(rx);
    float c1 = cossf(rx);
    float s2 = sinsf(ry);
    float c2 = cossf(ry);
    float s3 = sinsf(rz);
    float c3 = cossf(rz);

    out[0][0] = c2 * c3;
    out[0][1] = c2 * s3;
    out[0][2] = -s2;
    out[0][3] = 0.0f;

    out[1][0] = s1 * s2 * c3 - c1 * s3;
    out[1][1] = s1 * s2 * s3 + c1 * c3;
    out[1][2] = s1 * c2;
    out[1][3] = 0.0f;

    out[2][0] = c1 * s2 * c3 + s1 * s3;
    out[2][1] = c1 * s2 * s3 - s1 * c3;
    out[2][2] = c1 * c2;
    out[2][3] = 0.0f;

    out[3][0] = 0.0f;
    out[3][1] = 0.0f;
    out[3][2] = 0.0f;
    out[3][3] = 1.0f;
}

void mtxfEulerXYZInverse(MtxF out, int16_t rx, int16_t ry, int16_t rz)
{
    float s1 = sinsf(rx);
    float c1 = cossf(rx);
    float s2 = sinsf(ry);
    float c2 = cossf(ry);
    float s3 = sinsf(rz);
    float c3 = cossf(rz);

    out[0][0] = c2 * c3;
    out[1][0] = c2 * s3;
    out[2][0] = -s2;
    out[3][0] = 0.0f;

    out[0][1] = s1 * s2 * c3 - c1 * s3;
    out[1][1] = s1 * s2 * s3 + c1 * c3;
    out[2][1] = s1 * c2;
    out[3][1] = 0.0f;

    out[0][2] = c1 * s2 * c3 + s1 * s3;
    out[1][2] = c1 * s2 * s3 - s1 * c3;
    out[2][2] = c1 * c2;
    out[3][2] = 0.0f;

    out[0][3] = 0.0f;
    out[1][3] = 0.0f;
    out[2][3] = 0.0f;
    out[3][3] = 1.0f;
}

// Transforms a given vector by the given matrix, ignoring any translation in the matrix
void mtxfRotateVec(MtxF mat, Vec3 vecIn, Vec3 vecOut)
{
    // float inx = vecIn[0];
    // float iny = vecIn[1];
    // float inz = vecIn[2];
    // vecOut[0] = mat[0][0] * inx + mat[1][0] * iny + mat[2][0] * inz;
    // vecOut[1] = mat[0][1] * inx + mat[1][1] * iny + mat[2][1] * inz;
    // vecOut[2] = mat[0][2] * inx + mat[1][2] * iny + mat[2][2] * inz;
    glm::vec3 *vec_in = reinterpret_cast<glm::vec3*>(&vecIn[0]);
    glm::vec3 *vec_out = reinterpret_cast<glm::vec3*>(&vecOut[0]);
    glm::mat4 *mat_mat = reinterpret_cast<glm::mat4*>(&mat[0][0]);
    *vec_out = glm::mat3(*mat_mat) * *vec_in;
}

float approachFloatLinear(float current, float goal, float amount)
{
    if (goal > current)
    {
        return MIN(current + amount, goal);
    }
    else
    {
        return MAX(current - amount, goal);
    }
}
