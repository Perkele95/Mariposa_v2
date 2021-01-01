#pragma once

#include <stdint.h>
#include <math.h>
#include <xmmintrin.h>

// ---------------------
// Vectors
// ---------------------

struct vec2
{
    float x, y;
};

struct vec3
{
    float x, y, z;
};

inline vec3 operator*(float a, vec3 b)
{
    vec3 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;

    return result;
}

inline vec3 operator*(vec3 b, float a)
{
    return a * b;
}

inline vec3 operator/(float a, vec3 b)
{
    vec3 result;

    result.x = b.x / a;
    result.y = b.y / a;
    result.z = b.z / a;

    return result;
}

inline vec3 operator/(vec3 b, float a)
{
    return a / b;
}

inline vec3& operator*=(vec3 &a, float b)
{
    a = b * a;

    return a;
}

inline vec3 operator+(vec3 a)
{
    vec3 result;

    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

inline vec3 operator+(vec3 a, vec3 b)
{
    vec3 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

inline vec3 operator+(float b, vec3 a)
{
    vec3 result;

    result.x = a.x + b;
    result.y = a.y + b;
    result.z = a.z + b;

    return result;
}

inline vec3 operator+(vec3 a, float b)
{
    vec3 result;

    result.x = a.x + b;
    result.y = a.y + b;
    result.z = a.z + b;

    return result;
}

inline vec3& operator+=(vec3 &a, vec3 b)
{
    a = a + b;

    return a;
}

inline vec3 operator-(vec3 a, vec3 b)
{
    vec3 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

inline vec3 operator-(vec3 a, float b)
{
    vec3 result;

    result.x = a.x - b;
    result.y = a.y - b;
    result.z = a.z - b;

    return result;
}

inline vec3 operator-(float b, vec3 a)
{
    vec3 result;

    result.x = a.x - b;
    result.y = a.y - b;
    result.z = a.z - b;

    return result;
}

inline vec3& operator-=(vec3 &a, vec3 b)
{
    a = a - b;

    return a;
}

inline float vec3Length(vec3 vec)
{
    float result = sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    return result;
}

inline vec3 normalise(vec3 vec)
{
    float length = vec3Length(vec);
    vec3 result;

    result.x = vec.x / length;
    result.y = vec.y / length;
    result.z = vec.z / length;

    return result;
}

inline float vec3Dot(vec3 a, vec3 b)
{
    float result = a.x * b.x + a.y * b.y + a.z * b.z;

    return result;
}

inline vec3 vec3Cross(vec3 a, vec3 b)
{
    vec3 result;

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return result;
}

struct vec4
{
    float x, y, z, w;
};

inline vec4 operator*(float a, vec4 b)
{
    vec4 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;
    result.w = a * b.w;

    return result;
}

inline vec4 operator*(vec4 b, float a)
{
    return a * b;
}

inline vec4& operator*=(vec4 &a, float b)
{
    a = b * a;

    return a;
}

inline vec4 operator-(vec4 a)
{
    vec4 result;

    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;

    return result;
}

inline vec4 operator+(vec4 a, vec4 b)
{
    vec4 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;

    return result;
}

inline vec4 operator+(float b, vec4 a)
{
    vec4 result;

    result.x = a.x + b;
    result.y = a.y + b;
    result.z = a.z + b;
    result.w = a.w + b;

    return result;
}

inline vec4 operator+(vec4 a, float b)
{
    vec4 result;

    result.x = a.x + b;
    result.y = a.y + b;
    result.z = a.z + b;
    result.w = a.w + b;

    return result;
}

inline vec4& operator+=(vec4 &a, vec4 b)
{
    a = a + b;

    return a;
}

inline vec4 operator-(vec4 a, vec4 b)
{
    vec4 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;

    return result;
}

inline vec4 operator-(vec4 a, float b)
{
    vec4 result;

    result.x = a.x - b;
    result.y = a.y - b;
    result.z = a.z - b;
    result.w = a.w - b;

    return result;
}

inline vec4 operator-(float b, vec4 a)
{
    vec4 result;

    result.x = a.x - b;
    result.y = a.y - b;
    result.z = a.z - b;
    result.w = a.w - b;

    return result;
}

inline vec4& operator-=(vec4 &a, vec4 b)
{
    a = a - b;

    return a;
}
// TODO: simd this
inline vec4 normalise(vec4 vec)
{
    float length = sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w);
    vec4 result;

    result.x = vec.x / length;
    result.y = vec.y / length;
    result.z = vec.z / length;
    result.w = vec.z / length;

    return result;
}

struct vec3Int
{
    int32_t x, y, z;
};

inline vec3Int operator+(vec3Int a, vec3Int b)
{
    vec3Int result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

inline vec3Int mpVec3ToVec3Int(vec3 a)
{
    vec3Int result = {
        static_cast<int32_t>(a.x),
        static_cast<int32_t>(a.y),
        static_cast<int32_t>(a.z)
    };
    return result;
}

inline vec3 mpVec3IntToVec3(vec3Int a)
{
    vec3 result = {
        static_cast<float>(a.x),
        static_cast<float>(a.y),
        static_cast<float>(a.z)
    };
    return result;
}

// ---------------------
// 4x4 Matrix
// ---------------------

struct mat4x4
{
    float data[4][4];
};

inline constexpr mat4x4 Mat4x4Identity()
{
    mat4x4 result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return result;
}

inline mat4x4 operator*(mat4x4 a, mat4x4 b)
{
    mat4x4 result = {};

    float dataB0[4] = {b.data[0][0], b.data[1][0], b.data[2][0], b.data[3][0]};
    float dataB1[4] = {b.data[0][1], b.data[1][1], b.data[2][1], b.data[3][1]};
    float dataB2[4] = {b.data[0][2], b.data[1][2], b.data[2][2], b.data[3][2]};
    float dataB3[4] = {b.data[0][3], b.data[1][3], b.data[2][3], b.data[3][3]};
    __m128 wideDataB0 = _mm_load_ps(dataB0);
    __m128 wideDataB1 = _mm_load_ps(dataB1);
    __m128 wideDataB2 = _mm_load_ps(dataB2);
    __m128 wideDataB3 = _mm_load_ps(dataB3);
    float addArrayA[4];
    float addArrayB[4];
    float addArrayC[4];
    float addArrayD[4];

    for(uint32_t i = 0; i < 4; i++)
    {
        float dataA[4] = {a.data[i][0], a.data[i][1], a.data[i][2], a.data[i][3]};
        __m128 wideDataA = _mm_load_ps(dataA);

        __m128 data0 = _mm_mul_ps(wideDataA, wideDataB0);
        __m128 data1 = _mm_mul_ps(wideDataA, wideDataB1);
        __m128 data2 = _mm_mul_ps(wideDataA, wideDataB2);
        __m128 data3 = _mm_mul_ps(wideDataA, wideDataB3);

        _mm_store_ps(addArrayA, data0);
        _mm_store_ps(addArrayB, data1);
        _mm_store_ps(addArrayC, data2);
        _mm_store_ps(addArrayD, data3);

        result.data[i][0] = addArrayA[0] + addArrayA[1] + addArrayA[2] + addArrayA[3];
        result.data[i][1] = addArrayB[0] + addArrayB[1] + addArrayB[2] + addArrayB[3];
        result.data[i][2] = addArrayC[0] + addArrayC[1] + addArrayC[2] + addArrayC[3];
        result.data[i][3] = addArrayD[0] + addArrayD[1] + addArrayD[2] + addArrayD[3];
    }

    return result;
}

inline mat4x4 Mat4Translate(vec3 t)
{
    mat4x4 result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        t.x, t.y, t.z, 1.0f
    };
    return result;
}

inline mat4x4 Mat4RotateX(float rad)
{
    float c = cosf(rad);
    float s = sinf(rad);

    mat4x4 result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, -s, 0.0f,
        0.0f, s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return result;
}

inline mat4x4 Mat4RotateY(float rad)
{
    float c = cosf(rad);
    float s = sinf(rad);

    mat4x4 result = {
        c, 0.0f, s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return result;
}

inline mat4x4 Mat4RotateZ(float rad)
{
    float c = cosf(rad);
    float s = sinf(rad);

    mat4x4 result = {
        c, -s, 0.0f, 0.0f,
        s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return result;
}

inline mat4x4 LookAt(vec3 eye, vec3 center, vec3 up)
{
    vec3 X, Y, Z;

    Z = eye - center;
    Z = normalise(Z);
    Y = up;
    X = vec3Cross(Y, Z);
    Y = vec3Cross(Z, X);

    X = normalise(X);
    Y = normalise(Y);

    mat4x4 result;

    result.data[0][0] = X.x;
    result.data[1][0] = X.y;
    result.data[2][0] = X.z;
    result.data[3][0] = -vec3Dot(X, eye);

    result.data[0][1] = Y.x;
    result.data[1][1] = Y.y;
    result.data[2][1] = Y.z;
    result.data[3][1] = -vec3Dot(Y, eye);

    result.data[0][2] = Z.x;
    result.data[1][2] = Z.y;
    result.data[2][2] = Z.z;
    result.data[3][2] = -vec3Dot(Z, eye);

    result.data[0][3] = 0.0f;
    result.data[1][3] = 0.0f;
    result.data[2][3] = 0.0f;
    result.data[3][3] = 1.0f;

    return result;
}

// fov is in radians, aspectRatio = width / height, zNear and zFar are clipping plane z-positions
inline mat4x4 Perspective(float fov, float aspectRatio, float zNear, float zFar)
{
    mat4x4 result = {};
    float tanHalfAngle = tanf(fov / 2.0f);

    result.data[0][0] = 1.0f / (aspectRatio * tanHalfAngle);
    result.data[1][1] = -1.0f / (tanHalfAngle);
    result.data[2][2] = -(zFar + zNear) / (zFar - zNear);
    result.data[2][3] = -1.0f;
    result.data[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);

    return result;
}

// -----------------------------------------
// PERLIN NOISE

inline float _lerp(float a0, float a1, float weight)
{
    return a0 + (a1 - a0) * weight;
}

inline vec2 _randomGradient(int32_t ix, int32_t iy)
{
    float ixf = static_cast<float>(ix);
    float iyf = static_cast<float>(iy);
    float random = 2920.0f * sinf(ixf * 21942.0f + iyf * 171324.0f + 8912.0f) * cosf(ixf * 23157.0f * 217832.0f + 9758.0f);
    vec2 result = {cosf(random), sinf(random)};
    return result;
}

inline float _dotGridGradient(int32_t ix, int32_t iy, float x, float y)
{
    vec2 gradient = _randomGradient(ix, iy);
    float dx = x - ix;
    float dy = y - iy;

    return dx * gradient.x + dy * gradient.y;
}

inline float perlin(float x, float y)
{
    int32_t x0 = static_cast<int32_t>(x);
    int32_t x1 = x0 + 1;
    int32_t y0 = static_cast<int32_t>(y);
    int32_t y1 = y0 + 1;

    // Could also use higher order polynomial/s-curve here to determine lerp weights
    float sx = x - static_cast<float>(x0);
    float sy = y - static_cast<float>(y0);

    float n0 = _dotGridGradient(x0, y0, x, y);
    float n1 = _dotGridGradient(x1, y0, x, y);
    float ix0 = _lerp(n0, n1, sx);

    n0 = _dotGridGradient(x0, y1, x, y);
    n1 = _dotGridGradient(x1, y1, x, y);
    float ix1 = _lerp(n0, n1, sx);

    float result = _lerp(ix0, ix1, sy);
    return result;
}

inline float mapNoise(vec3 position)
{
    float map = perlin(position.x, position.y);
    float result = perlin(map, position.z);

    return result;
}

inline uint32_t uint32Clamp(uint32_t value, uint32_t min, uint32_t max)
{
    if(value < min)
        value = min;
    else if(value > max)
        value = max;

    return value;
}