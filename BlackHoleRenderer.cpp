#include "BlackHoleRenderer.h"
#include <iostream>

// Static noise and hash implementation matching WebGL GLSL logic
double BlackHoleRenderer::Hash(const Vector3& p) {
    double px = p.x * 0.3183099 + 0.1;
    double py = p.y * 0.3183099 + 0.1;
    double pz = p.z * 0.3183099 + 0.1;

    px = px - std::floor(px);
    py = py - std::floor(py);
    pz = pz - std::floor(pz);

    px *= 17.0;
    py *= 17.0;
    pz *= 17.0;

    double term = px * py * pz * (px + py + pz);
    return term - std::floor(term);
}

double BlackHoleRenderer::Noise(const Vector3& x) {
    Vector3 i(std::floor(x.x), std::floor(x.y), std::floor(x.z));
    Vector3 f(x.x - i.x, x.y - i.y, x.z - i.z);

    // Smoothstep interpolation
    Vector3 u(f.x * f.x * (3.0 - 2.0 * f.x),
              f.y * f.y * (3.0 - 2.0 * f.y),
              f.z * f.z * (3.0 - 2.0 * f.z));

    double n000 = Hash(i + Vector3(0.0, 0.0, 0.0));
    double n100 = Hash(i + Vector3(1.0, 0.0, 0.0));
    double n010 = Hash(i + Vector3(0.0, 1.0, 0.0));
    double n110 = Hash(i + Vector3(1.0, 1.0, 0.0));
    double n001 = Hash(i + Vector3(0.0, 0.0, 1.0));
    double n101 = Hash(i + Vector3(1.0, 0.0, 1.0));
    double n011 = Hash(i + Vector3(0.0, 1.0, 1.0));
    double n111 = Hash(i + Vector3(1.0, 1.0, 1.0));

    double nx00 = (1.0 - u.x) * n000 + u.x * n100;
    double nx10 = (1.0 - u.x) * n010 + u.x * n110;
    double nx01 = (1.0 - u.x) * n001 + u.x * n101;
    double nx11 = (1.0 - u.x) * n011 + u.x * n111;

    double nxy0 = (1.0 - u.y) * nx00 + u.y * nx10;
    double nxy1 = (1.0 - u.y) * nx01 + u.y * nx11;

    return (1.0 - u.z) * nxy0 + u.z * nxy1;
}

double BlackHoleRenderer::Fbm(const Vector3& p) {
    double v = 0.0;
    double a = 0.5;
    Vector3 pos = p;
    Vector3 shift(100.0, 100.0, 100.0);
    for (int i = 0; i < 4; ++i) {
        v += a * Noise(pos);
        pos = pos * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

Vector3 BlackHoleRenderer::Blackbody(double temp) {
    Vector3 col;
    // Map a temperature/intensity to red-orange-yellow blackbody spectrum
    col.x = 1.0 - std::exp(-temp * 2.5);
    col.y = 1.0 - std::exp(-temp * temp * 1.5);
    col.z = 1.0 - std::exp(-temp * temp * temp * 4.0);

    col.x = std::max(0.0, std::min(1.0, col.x));
    col.y = std::max(0.0, std::min(1.0, col.y));
    col.z = std::max(0.0, std::min(1.0, col.z));
    return col;
}

Vector3 BlackHoleRenderer::GetStarfield(const Vector3& rd) {
    Vector3 rd_norm = rd.normalized();

    // Rotate to galactic coordinates (tilted by ~25 degrees / 0.436 radians around X)
    double cos_tilt = std::cos(0.436);
    double sin_tilt = std::sin(0.436);
    Vector3 rd_g(
        rd_norm.x,
        rd_norm.y * cos_tilt - rd_norm.z * sin_tilt,
        rd_norm.y * sin_tilt + rd_norm.z * cos_tilt
    );

    // 1. Galactic Bulge/Core (Warm, elliptical central bulge)
    // Centered in direction of positive X axis
    double core_arg = (1.0 - rd_g.x) * 6.0 + rd_g.y * rd_g.y * 30.0;
    double core_intensity = std::exp(-core_arg);
    Vector3 core_color = Vector3(1.6, 1.1, 0.7) * core_intensity;

    // 2. Galactic Plane Band
    double band_arg = rd_g.y * rd_g.y * 18.0;
    double band_val = std::exp(-band_arg);

    // FBM noise for dark interstellar dust lanes blocking galactic plane light
    double dust = Fbm(rd_g * 5.5 + Vector3(17.4, 38.1, 82.3));
    double band_intensity = band_val * std::max(0.0, 1.0 - 0.85 * dust);
    Vector3 band_color = Vector3(0.55, 0.45, 0.85) * band_intensity;

    // 3. Dual-Layer Starfield
    Vector3 star_color(0.0, 0.0, 0.0);

    // Layer A: Bright, sparse stars (will stretch into nice lensed arcs near the shadow)
    Vector3 grid_bright(std::floor(rd_norm.x * 280.0), 
                        std::floor(rd_norm.y * 280.0), 
                        std::floor(rd_norm.z * 280.0));
    double n1 = Hash(grid_bright);
    if (n1 > 0.996) {
        double b = std::pow((n1 - 0.996) / 0.004, 8.0) * 1.5;
        double f5 = Hash(grid_bright * 5.0); f5 = f5 - std::floor(f5);
        double f11 = Hash(grid_bright * 11.0); f11 = f11 - std::floor(f11);
        star_color += Vector3(b * (0.8 + 0.2 * f5), b * (0.85 + 0.15 * f11), b * 1.1);
    }

    // Layer B: Dense, dim background stars
    Vector3 grid_dim(std::floor(rd_norm.x * 550.0), 
                     std::floor(rd_norm.y * 550.0), 
                     std::floor(rd_norm.z * 550.0));
    double n2 = Hash(grid_dim);
    if (n2 > 0.985) {
        double b = std::pow((n2 - 0.985) / 0.015, 6.0) * 0.4;
        double f7 = Hash(grid_dim * 7.0); f7 = f7 - std::floor(f7);
        star_color += Vector3(b * (0.9 + 0.1 * f7), b * 0.9, b * 0.9);
    }

    // 4. Background Nebulae (Faint multi-colored gas clouds)
    double neb1 = Fbm(rd_norm * 2.2);
    double neb2 = Fbm(rd_norm * 4.0 + Vector3(25.0, 10.0, -15.0));
    Vector3 neb_color = Vector3(neb1 * 0.08, neb1 * 0.02, neb1 * 0.14) * 0.25 +
                        Vector3(neb2 * 0.03, neb2 * 0.07, neb2 * 0.03) * 0.25;

    return core_color + band_color + star_color + neb_color;
}

void BlackHoleRenderer::RenderFrame(unsigned char* rgb_buffer, int width, int height, const RenderParams& params) {
    double M = params.M;
    double dist = params.cam_distance;
    double pitch = params.cam_pitch;
    double yaw = params.cam_yaw;

    // Compute Camera Coordinates
    double cx = dist * std::cos(pitch) * std::sin(yaw);
    double cy = dist * std::sin(pitch);
    double cz = dist * std::cos(pitch) * std::cos(yaw);
    Vector3 cam_pos(cx, cy, cz);

    double len = cam_pos.length();
    Vector3 cam_dir = (-1.0 / len) * cam_pos;

    double rx = -std::cos(yaw);
    double rz = std::sin(yaw);
    double rightLen = std::sqrt(rx * rx + rz * rz);
    Vector3 cam_right(rx / rightLen, 0.0, rz / rightLen);

    Vector3 cam_up = cross(cam_right, cam_dir);
    double aspect = static_cast<double>(width) / height;

    // Run parallel ray tracing loop across all pixels
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Screen mapping to [-1, 1]
            double px = (static_cast<double>(x) / width - 0.5) * 2.0;
            double py = (static_cast<double>(y) / height - 0.5) * 2.0;
            px *= aspect;

            // Generate ray
            Vector3 rd = (cam_dir + px * cam_right + py * cam_up).normalized();

            // Isotropic coordinate scale
            double r = cam_pos.length();
            double u = M / (2.0 * r);
            double n_cam = std::pow(1.0 + u, 3.0) / (1.0 - u);

            Vector3 p = cam_pos;
            Vector3 v = rd / n_cam;

            Vector3 acc_color(0.0, 0.0, 0.0);
            double ray_transmission = 1.0;

            // Photon propagation loop
            for (int step = 0; step < 160; ++step) {
                double r_cur = p.length();

                // Captured: Event horizon crossed ( isotropic radius = 0.5M )
                if (r_cur < 0.502 * M) {
                    ray_transmission = 0.0;
                    break;
                }

                // Escaped: Reached far away bounds
                if (r_cur > 40.0 * M) {
                    acc_color += ray_transmission * GetStarfield(v);
                    ray_transmission = 0.0;
                    break;
                }

                // Adaptive step size based on proximity to horizon
                double dt = params.dt_base * (r_cur - 0.495 * M);

                // Verlet step: compute acceleration
                double u_c = M / (2.0 * r_cur);
                double num = -M * std::pow(1.0 + u_c, 5.0) * (2.0 - u_c);
                double den = std::pow(r_cur, 3.0) * std::pow(1.0 - u_c, 3.0);
                Vector3 a = (num / den) * p;

                Vector3 p_prev = p;
                p += v * dt + 0.5 * a * dt * dt;

                double r_next = p.length();
                double u_n = M / (2.0 * r_next);
                double num_n = -M * std::pow(1.0 + u_n, 5.0) * (2.0 - u_n);
                double den_n = std::pow(r_next, 3.0) * std::pow(1.0 - u_n, 3.0);
                Vector3 a_next = (num_n / den_n) * p;

                v += 0.5 * (a + a_next) * dt;

                // Accretion disk equatorial crossing check
                if (p_prev.z * p.z < 0.0 && params.enable_disk) {
                    double t_cross = -p_prev.z / (p.z - p_prev.z);
                    Vector3 p_cross = p_prev + t_cross * (p - p_prev);
                    double r_cross = p_cross.length();

                    if (r_cross >= params.disk_inner && r_cross <= params.disk_outer) {
                        // Keplerian orbit speed in isotropic coordinates
                        double R_cross = r_cross * std::pow(1.0 + M / (2.0 * r_cross), 2.0);
                        double speed = std::sqrt(M / R_cross);
                        Vector3 disk_vel = Vector3(-p_cross.y, p_cross.x, 0.0) / r_cross * speed;

                        // Doppler beaming factor
                        Vector3 ray_dir = v.normalized();
                        double v_dot_rd = dot(disk_vel, ray_dir);
                        double beta2 = dot(disk_vel, disk_vel);
                        double gamma = 1.0 / std::sqrt(1.0 - beta2);
                        double doppler = 1.0 / (gamma * (1.0 - v_dot_rd));

                        // Gravitational redshift
                        double u_cross = M / (2.0 * r_cross);
                        double g00_cross = std::pow((1.0 - u_cross) / (1.0 + u_cross), 2.0);
                        double g00_cam = std::pow((1.0 - u) / (1.0 + u), 2.0);
                        double grav_redshift = std::sqrt(g00_cross / g00_cam);

                        // Rotate the 3D position vector around Z to animate gas flow without seams
                        double rot_angle = - params.time * (speed / r_cross) * 3.5;
                        double cos_a = std::cos(rot_angle);
                        double sin_a = std::sin(rot_angle);
                        Vector3 p_rot(
                            p_cross.x * cos_a - p_cross.y * sin_a,
                            p_cross.x * sin_a + p_cross.y * cos_a,
                            0.0
                        );

                        // Spiral pattern coordinate (sine wraps any 2pi or 4pi jump continuously)
                        double spiral = std::sin(r_cross * 1.8 - std::atan2(p_rot.y, p_rot.x) * 2.0);

                        // Emission temperature (power scale raised slightly for steeper gradient)
                        double temp = std::pow(params.disk_inner / r_cross, 0.85) * params.disk_temp;

                        // Use continuous Cartesian coordinates in FBM noise for primary gas lanes
                        double gas_noise = Fbm(Vector3(p_rot.x * 1.5, p_rot.y * 1.5, spiral * 1.5));

                        // High-frequency magnetic shear turbulence layer
                        double turbulence = Fbm(Vector3(p_rot.x * 6.5, p_rot.y * 6.5, spiral * 3.0));

                        // Combined gas density combining main lanes with fine turbulent structures
                        double combined_gas = (0.15 + 0.85 * gas_noise) * (0.65 + 0.35 * turbulence);

                        // Gas density envelope
                        double radial_envelope = std::exp(-std::pow(r_cross - (params.disk_inner + params.disk_outer) * 0.5, 2.0) /
                                                         (0.18 * std::pow(params.disk_outer - params.disk_inner, 2.0)));
                        double density = combined_gas * radial_envelope;
                        double opacity = 1.0 - std::exp(-density * 1.5);

                        // Enhance Doppler beaming temperature shift to make blue-shifting more distinct
                        double temp_shift = grav_redshift * (params.enable_beaming ? std::pow(doppler, 1.25) : 1.0);
                        Vector3 shifted_color = Blackbody(temp * temp_shift);
                        double beaming = params.enable_beaming ? std::pow(doppler, 3.5) : 1.0;
                        Vector3 emission = shifted_color * beaming * params.disk_bright;

                        acc_color += ray_transmission * emission * opacity;
                        ray_transmission *= (1.0 - opacity);

                        if (ray_transmission < 0.01) {
                            ray_transmission = 0.0;
                            break;
                        }
                    }
                }
            }

            // Populate RGB output buffer
            int index = (y * width + x) * 3;
            rgb_buffer[index]     = static_cast<unsigned char>(std::min(255.0, acc_color.x * 255.0)); // R
            rgb_buffer[index + 1] = static_cast<unsigned char>(std::min(255.0, acc_color.y * 255.0)); // G
            rgb_buffer[index + 2] = static_cast<unsigned char>(std::min(255.0, acc_color.z * 255.0)); // B
        }
    }
}
