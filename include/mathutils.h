#ifndef __MATHUTILS_H__
#define __MATHUTILS_H__

#include <types.h>
#include <cmath>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define EPSILON 0.0001f

// TODO these can be vectorized with gcc for modern platforms
#define VEC3_DOT(a,b) ((a)[0] * (b)[0] + (a)[1] * (b)[1] + (a)[2] * (b)[2])
#define VEC3_COPY(out,a) \
    {(out)[0] = (a)[0];           (out)[1] = (a)[1];           (out)[2] = (a)[2];}
#define VEC3_SCALE(out,a,scale) \
    {(out)[0] = (a)[0] * (scale); (out)[1] = (a)[1] * (scale); (out)[2] = (a)[2] * (scale);}
#define VEC3_ADD(out,a,b)  \
    {(out)[0] = (a)[0] + (b)[0];  (out)[1] = (a)[1] + (b)[1];  (out)[2] = (a)[2] + (b)[2];}
#define VEC3_DIFF(out,a,b) \
    {(out)[0] = (a)[0] - (b)[0];  (out)[1] = (a)[1] - (b)[1];  (out)[2] = (a)[2] - (b)[2];}
#define ABS(x) ((x) > 0 ? (x) : -(x))

#define POW2(x) ((x) * (x))

// Division in C rounds towards 0, rather than towards negative infinity
// Rounding towards negative infinity is quicker for powers of two (just a right arithmetic shift)
// This also ensure correctness for negative chunk positions (which don't exist currently, but might in the future)
template <size_t D, typename T>
constexpr T round_down_divide(T x)
{
    static_assert(D && !(D & (D - 1)), "Can only round down divide by a power of 2!");
    // This log is evaluated at compile time
    size_t log = static_cast<size_t>(std::log2(D));
    return x >> log;
}

// Same deal as above, but for modulo
template <size_t D, typename T>
constexpr T round_down_modulo(T x)
{
    // Compiler optimizes this to a simple bitwise and
    return x - D * round_down_divide<D>(x);
}

// Same deal above, but rounds towards positive infinity
template <size_t D, typename T>
constexpr T round_up_divide(T x)
{
    static_assert(D && !(D & (D - 1)), "Can only round up divide by a power of 2!");
    // This log is evaluated at compile time
    size_t log = static_cast<size_t>(std::log2(D));
    return ((x - 1) >> log) + 1;
}

constexpr uint16_t degrees_to_angle(float degrees)
{
    return (uint16_t)((degrees / 360.0f) * 0x10000L);
}

#define M_PIf (3.14159265358979323846f)
#define M_PIf_2 (1.57079632679489661923f)

#define M_PI_4_P_0273	(1.05839816339744830962f) //M_PI/4 + 0.273
#ifndef M_PI_4
#define M_PI_4 (M_PIf / 4)
#endif

float fastAtanf(float x);
float fastAtan2f(float y, float x);
int16_t atan2s(float y, float x);

float sinsf(int16_t angle);
#define cossf(x) sinsf((x) + 0x4000)

void mtxfMul(MtxF out, MtxF a, MtxF b);
void mtxfEulerXYZ(MtxF out, int16_t rx, int16_t ry, int16_t rz);
void mtxfEulerXYZInverse(MtxF out, int16_t rx, int16_t ry, int16_t rz);
void mtxfRotateVec(MtxF mat, Vec3 vecIn, Vec3 vecOut);

float approachFloatLinear(float current, float goal, float amount);

#endif
