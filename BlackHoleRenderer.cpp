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

void BlackHoleRenderer::GetDerivatives(const Vector3& pos, const Vector3& mom, double M, double a,
                                       Vector3& dpos, Vector3& dmom, double& r_out) {
    double x = pos.x;
    double y = pos.y;
    double z = pos.z;

    double rho2 = x * x + y * y + z * z;
    double r2 = 0.5 * ((rho2 - a * a) + std::sqrt(std::max(0.0, (rho2 - a * a) * (rho2 - a * a) + 4.0 * a * a * z * z)));
    double r = std::sqrt(r2);
    r_out = r;

    // Avoid division by zero at the singularity/ring
    if (r < 1e-6) r = 1e-6;

    // 1. Compute derivatives of r with respect to x, y, z
    double den_r = r * r * r * r + a * a * z * z;
    if (den_r < 1e-9) den_r = 1e-9;
    
    double dr_dx = (r * r * r * x) / den_r;
    double dr_dy = (r * r * r * y) / den_r;
    double dr_dz = (r * (r * r + a * a) * z) / den_r;
    Vector3 grad_r(dr_dx, dr_dy, dr_dz);

    // 2. Compute H_KS = M * r^3 / (r^4 + a^2 * z^2)
    double H_KS = (M * r * r * r) / den_r;

    // 3. Compute gradient of H_KS
    double dH_dr = M * r * r * (3.0 * a * a * z * z - r * r * r * r) / (den_r * den_r);
    double dH_dz_explicit = -2.0 * a * a * M * r * r * r * z / (den_r * den_r);
    Vector3 grad_H = dH_dr * grad_r;
    grad_H.z += dH_dz_explicit;

    // 4. Compute covariant null vector l_mu
    double D = r * r + a * a;
    if (D < 1e-9) D = 1e-9;
    
    Vector3 l((r * x + a * y) / D, (r * y - a * x) / D, z / r);

    // 5. Compute derivatives of l_mu
    Vector3 grad_lx;
    grad_lx.x = ((dr_dx * x + r) * D - (r * x + a * y) * 2.0 * r * dr_dx) / (D * D);
    grad_lx.y = ((dr_dy * x + a) * D - (r * x + a * y) * 2.0 * r * dr_dy) / (D * D);
    grad_lx.z = ((dr_dz * x) * D - (r * x + a * y) * 2.0 * r * dr_dz) / (D * D);

    Vector3 grad_ly;
    grad_ly.x = ((dr_dx * y - a) * D - (r * y - a * x) * 2.0 * r * dr_dx) / (D * D);
    grad_ly.y = ((dr_dy * y + r) * D - (r * y - a * x) * 2.0 * r * dr_dy) / (D * D);
    grad_ly.z = ((dr_dz * y) * D - (r * y - a * x) * 2.0 * r * dr_dz) / (D * D);

    Vector3 grad_lz;
    grad_lz.x = -z * dr_dx / (r * r);
    grad_lz.y = -z * dr_dy / (r * r);
    grad_lz.z = (r - z * dr_dz) / (r * r);

    // 6. Compute S = p_0 - l . p (where p_0 = -1.0)
    double p_0 = -1.0;
    double S = p_0 - dot(l, mom);

    // 7. Compute dpos = dx/dlambda
    dpos = mom + (2.0 * H_KS * S) * l;

    // 8. Compute dmom = dp/dlambda
    Vector3 grad_l_p(
        mom.x * grad_lx.x + mom.y * grad_ly.x + mom.z * grad_lz.x,
        mom.x * grad_lx.y + mom.y * grad_ly.y + mom.z * grad_lz.y,
        mom.x * grad_lx.z + mom.y * grad_ly.z + mom.z * grad_lz.z
    );

    dmom = S * S * grad_H - (2.0 * H_KS * S) * grad_l_p;
}

void BlackHoleRenderer::RenderFrame(unsigned char* rgb_buffer, int width, int height, const RenderParams& params) {
    double M = params.M;
    double a = params.a;
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

    // Outer event horizon radius for Kerr black hole
    double r_horizon = M + std::sqrt(std::max(0.0, M * M - a * a));

    // Run parallel ray tracing loop across all pixels
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Screen mapping to [-1, 1]
            double px = (static_cast<double>(x) / width - 0.5) * 2.0;
            double py = (static_cast<double>(y) / height - 0.5) * 2.0;
            px *= aspect;

            // Generate ray direction
            Vector3 rd = (cam_dir + px * cam_right + py * cam_up).normalized();

            // Setup camera coordinates in Kerr-Schild
            Vector3 p = cam_pos;

            // Satisfy the null condition g^uv p_u p_v = 0 exactly at the camera position
            double px_cam = cam_pos.x;
            double py_cam = cam_pos.y;
            double pz_cam = cam_pos.z;
            double rho2_cam = px_cam * px_cam + py_cam * py_cam + pz_cam * pz_cam;
            double r2_cam = 0.5 * ((rho2_cam - a * a) + std::sqrt(std::max(0.0, (rho2_cam - a * a) * (rho2_cam - a * a) + 4.0 * a * a * pz_cam * pz_cam)));
            double r_cam = std::sqrt(r2_cam);
            if (r_cam < 1e-6) r_cam = 1e-6;
            double den_cam = r_cam * r_cam * r_cam * r_cam + a * a * pz_cam * pz_cam;
            if (den_cam < 1e-9) den_cam = 1e-9;
            double H_KS_cam = (M * r_cam * r_cam * r_cam) / den_cam;
            double D_cam = r_cam * r_cam + a * a;
            if (D_cam < 1e-9) D_cam = 1e-9;
            Vector3 l_cam((r_cam * px_cam + a * py_cam) / D_cam, (r_cam * py_cam - a * px_cam) / D_cam, pz_cam / r_cam);
            
            double L_d = dot(l_cam, rd);
            double p_0 = -1.0;
            double A_k = 1.0 - 2.0 * H_KS_cam * L_d * L_d;
            double B_k = 4.0 * H_KS_cam * p_0 * L_d;
            double C_k = -1.0 - 2.0 * H_KS_cam * p_0 * p_0;
            double disc = B_k * B_k - 4.0 * A_k * C_k;
            double k = 1.0;
            if (disc >= 0.0) {
                k = (-B_k + std::sqrt(disc)) / (2.0 * A_k);
            }
            Vector3 v = k * rd; // Contravariant spatial momentum

            Vector3 acc_color(0.0, 0.0, 0.0);
            double ray_transmission = 1.0;

            // Coupled RK4 Geodesic Integration Loop
            for (int step = 0; step < 180; ++step) {
                double r_cur;
                Vector3 dx_dummy, dp_dummy;
                GetDerivatives(p, v, M, a, dx_dummy, dp_dummy, r_cur);

                // Captured: Event horizon crossed (in Kerr-Schild, event horizon is at r_horizon)
                if (r_cur < r_horizon + 0.01) {
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
                double dt = params.dt_base * (r_cur - 0.95 * r_horizon);
                if (dt < 0.005) dt = 0.005; // clamp step size for stability

                // 4th-order Runge-Kutta coupled integration step
                Vector3 k1_x, k1_p;
                GetDerivatives(p, v, M, a, k1_x, k1_p, r_cur);

                Vector3 k2_x, k2_p;
                GetDerivatives(p + 0.5 * dt * k1_x, v + 0.5 * dt * k1_p, M, a, k2_x, k2_p, r_cur);

                Vector3 k3_x, k3_p;
                GetDerivatives(p + 0.5 * dt * k2_x, v + 0.5 * dt * k2_p, M, a, k3_x, k3_p, r_cur);

                Vector3 k4_x, k4_p;
                GetDerivatives(p + dt * k3_x, v + dt * k3_p, M, a, k4_x, k4_p, r_cur);

                Vector3 p_prev = p;
                p += (dt / 6.0) * (k1_x + 2.0 * k2_x + 2.0 * k3_x + k4_x);
                v += (dt / 6.0) * (k1_p + 2.0 * k2_p + 2.0 * k3_p + k4_p);

                // Accretion disk equatorial crossing check
                if (p_prev.z * p.z < 0.0 && params.enable_disk) {
                    double t_cross = -p_prev.z / (p.z - p_prev.z);
                    Vector3 p_cross = p_prev + t_cross * (p - p_prev);
                    
                    // In Kerr-Schild, the coordinate distance at z=0 is:
                    // x^2 + y^2 = r_cross^2 + a^2 => r_cross = sqrt(x^2 + y^2 - a^2)
                    double r_cross = std::sqrt(std::max(0.0, p_cross.x * p_cross.x + p_cross.y * p_cross.y - a * a));

                    if (r_cross >= params.disk_inner && r_cross <= params.disk_outer) {
                        // Keplerian orbital angular velocity in Kerr spacetime
                        double omega = std::sqrt(M) / (std::pow(r_cross, 1.5) + a * std::sqrt(M));
                        double speed = omega * std::sqrt(r_cross * r_cross + a * a); // coordinate speed in plane
                        
                        // Disk velocity vector
                        Vector3 disk_vel = Vector3(-p_cross.y, p_cross.x, 0.0) / std::sqrt(std::max(1e-6, p_cross.x * p_cross.x + p_cross.y * p_cross.y)) * speed;

                        // Doppler beaming factor (momentum v in Kerr-Schild)
                        Vector3 ray_dx, ray_dp; double r_dummy;
                        GetDerivatives(p_cross, v, M, a, ray_dx, ray_dp, r_dummy);
                        Vector3 ray_dir = ray_dx.normalized();
                        
                        double v_dot_rd = dot(disk_vel, ray_dir);
                        double beta2 = dot(disk_vel, disk_vel);
                        double gamma = 1.0 / std::sqrt(std::max(0.01, 1.0 - beta2));
                        double doppler = 1.0 / (gamma * (1.0 - v_dot_rd));

                        // Gravitational redshift in Kerr-Schild:
                        // g00 = 1 - 2H_KS (covariant metric)
                        double H_KS_cross = (M * r_cross * r_cross * r_cross) / (r_cross * r_cross * r_cross * r_cross + a * a * 0.0);
                        double g00_cross = 1.0 - 2.0 * H_KS_cross;
                        if (g00_cross < 0.01) g00_cross = 0.01; // Clamp near horizon
                        double grav_redshift = std::sqrt(g00_cross);

                        // Rotate the 3D position vector around Z to animate gas flow without seams
                        double angle = std::atan2(p_cross.y, p_cross.x);
                        double rot_angle = angle - params.time * omega * 3.5;
                        double cos_a = std::cos(rot_angle);
                        double sin_a = std::sin(rot_angle);
                        Vector3 p_rot(
                            p_cross.x * cos_a - p_cross.y * sin_a,
                            p_cross.x * sin_a + p_cross.y * cos_a,
                            0.0
                        );

                        // Spiral pattern coordinate
                        double spiral = std::sin(r_cross * 1.8 - rot_angle * 2.0);

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
