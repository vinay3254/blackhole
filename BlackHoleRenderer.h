#ifndef BLACK_HOLE_RENDERER_H
#define BLACK_HOLE_RENDERER_H

#include <cmath>
#include <algorithm>

// Simple, self-contained 3D Vector structure for pure C++ ray-tracing
struct Vector3 {
    double x, y, z;

    Vector3() : x(0.0), y(0.0), z(0.0) {}
    Vector3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*(double s) const { return Vector3(x * s, y * s, z * s); }
    Vector3 operator/(double s) const { return Vector3(x / s, y / s, z / s); }

    Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vector3& operator*=(double s) { x *= s; y *= s; z *= s; return *this; }

    double length() const { return std::sqrt(x * x + y * y + z * z); }
    Vector3 normalized() const {
        double len = length();
        if (len > 0.0) return Vector3(x / len, y / len, z / len);
        return Vector3(0.0, 0.0, 0.0);
    }
};

// Inline operators for Vector3 scaling
inline Vector3 operator*(double s, const Vector3& v) { return Vector3(v.x * s, v.y * s, v.z * s); }
inline double dot(const Vector3& u, const Vector3& v) { return u.x * v.x + u.y * v.y + u.z * v.z; }
inline Vector3 cross(const Vector3& u, const Vector3& v) {
    return Vector3(
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    );
}

// Rendering parameters configured by sliders/UI
struct RenderParams {
    double M = 1.0;                     // Black hole mass
    double cam_distance = 15.0;         // Camera distance from origin
    double cam_pitch = 0.15;            // Camera pitch angle in radians (tilt)
    double cam_yaw = 0.0;               // Camera yaw angle in radians (orbit rotation)
    
    bool enable_disk = true;            // Enable accretion disk rendering
    double disk_inner = 3.0;            // Inner radius of the accretion disk
    double disk_outer = 12.0;           // Outer radius of the accretion disk
    double disk_bright = 1.5;           // Intensity factor for the disk brightness
    double disk_temp = 1.4;             // Base temperature scale for the disk
    
    bool enable_beaming = true;         // Enable relativistic Doppler beaming
    double dt_base = 0.08;              // Base step size for Verlet integration
    double time = 0.0;                  // Time value to animate disk gas rotation
};

class BlackHoleRenderer {
public:
    BlackHoleRenderer() = default;
    ~BlackHoleRenderer() = default;

    // Renders a frame to a raw RGB buffer (3 bytes per pixel: Red, Green, Blue)
    // Runs in parallel using OpenMP if compiled with -fopenmp
    static void RenderFrame(unsigned char* rgb_buffer, int width, int height, const RenderParams& params);

private:
    // Helper mathematical and procedural functions
    static double Hash(const Vector3& p);
    static double Noise(const Vector3& x);
    static double Fbm(const Vector3& p);
    static Vector3 Blackbody(double temp);
    static Vector3 GetStarfield(const Vector3& rd);
};

#endif // BLACK_HOLE_RENDERER_H
