#pragma once
#include <cmath>
#include <cstring>
#include <algorithm>

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float length() const { return std::sqrt(x*x + y*y + z*z); }
    
    Vec3 normalized() const {
        float len = length();
        if (len > 0.0001f) return *this / len;
        return {0.0f, 0.0f, 0.0f};
    }

    static float dot(const Vec3& a, const Vec3& b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a + (b - a) * t;
    }
};

struct Vec2 {
    float x = 0.0f, y = 0.0f;
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
};

struct Mat4 {
    float m[16] = {0.0f};

    Mat4() {
        setIdentity();
    }

    void setIdentity() {
        std::memset(m, 0, sizeof(m));
        m[0] = 1.0f;
        m[5] = 1.0f;
        m[10] = 1.0f;
        m[15] = 1.0f;
    }

    static Mat4 identity() {
        Mat4 res;
        return res;
    }

    static Mat4 translation(const Vec3& v) {
        Mat4 res;
        res.m[12] = v.x;
        res.m[13] = v.y;
        res.m[14] = v.z;
        return res;
    }

    static Mat4 scaling(const Vec3& v) {
        Mat4 res;
        res.m[0] = v.x;
        res.m[5] = v.y;
        res.m[10] = v.z;
        return res;
    }

    static Mat4 rotationY(float angleRad) {
        Mat4 res;
        float c = std::cos(angleRad);
        float s = std::sin(angleRad);
        res.m[0] = c;
        res.m[2] = -s;
        res.m[8] = s;
        res.m[10] = c;
        return res;
    }

    static Mat4 rotationX(float angleRad) {
        Mat4 res;
        float c = std::cos(angleRad);
        float s = std::sin(angleRad);
        res.m[5] = c;
        res.m[6] = s;
        res.m[9] = -s;
        res.m[10] = c;
        return res;
    }

    static Mat4 perspective(float fovRad, float aspect, float nearVal, float farVal) {
        Mat4 res;
        std::memset(res.m, 0, sizeof(res.m));
        float f = 1.0f / std::tan(fovRad / 2.0f);
        res.m[0] = f / aspect;
        res.m[5] = f;
        res.m[10] = (farVal + nearVal) / (nearVal - farVal);
        res.m[11] = -1.0f;
        res.m[14] = (2.0f * farVal * nearVal) / (nearVal - farVal);
        return res;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = Vec3::cross(f, up).normalized();
        Vec3 u = Vec3::cross(s, f);

        Mat4 res;
        res.m[0] = s.x;
        res.m[4] = s.y;
        res.m[8] = s.z;
        res.m[12] = -Vec3::dot(s, eye);

        res.m[1] = u.x;
        res.m[5] = u.y;
        res.m[9] = u.z;
        res.m[13] = -Vec3::dot(u, eye);

        res.m[2] = -f.x;
        res.m[6] = -f.y;
        res.m[10] = -f.z;
        res.m[14] = Vec3::dot(f, eye);

        res.m[15] = 1.0f;
        return res;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                float val = 0.0f;
                for (int i = 0; i < 4; ++i) {
                    val += m[row + i * 4] * o.m[i + col * 4];
                }
                r.m[row + col * 4] = val;
            }
        }
        return r;
    }
};
