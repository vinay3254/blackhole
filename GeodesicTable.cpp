#include "GeodesicTable.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GeodesicTable::GeodesicTable()
    : m_M(1.0)
    , m_bMin(0.0)
    , m_bMax(100.0)
    , m_bCrit(0.0)
    , m_stepsB(0)
{
}

void GeodesicTable::GenerateTable(double M, double b_min, double b_max, int steps_b) {
    m_M = M;
    m_bMin = b_min;
    m_bMax = b_max;
    m_stepsB = steps_b;
    m_bCrit = 3.0 * std::sqrt(3.0) * M; // Analytic Schwarzschild critical b
    m_table.resize(steps_b);

    // Using OpenMP if available, otherwise runs sequentially
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < steps_b; ++i) {
        double t = static_cast<double>(i) / (steps_b - 1);
        double b = b_min + t * (b_max - b_min);
        m_table[i] = IntegrateRay(b, M);
    }
}

GeodesicEntry GeodesicTable::Lookup(double b) const {
    if (m_table.empty()) {
        GeodesicEntry dummy;
        dummy.b = b;
        dummy.captured = true;
        dummy.deflection = 0.0;
        dummy.r_min = 2.0 * m_M;
        return dummy;
    }

    // Direct check against critical impact parameter
    if (b <= m_bCrit) {
        GeodesicEntry entry;
        entry.b = b;
        entry.captured = true;
        entry.deflection = 0.0;
        entry.r_min = 2.0 * m_M;
        return entry;
    }

    if (b >= m_bMax) {
        // For very large b, return the last entry or approximate with weak-field deflection
        GeodesicEntry last = m_table.back();
        if (!last.captured) {
            double weak_deflection = 4.0 * m_M / b;
            last.deflection = weak_deflection;
            last.r_min = b; // approximately
        }
        last.b = b;
        return last;
    }

    // Linear interpolation
    double idx_d = (b - m_bMin) / (m_bMax - m_bMin) * (m_stepsB - 1);
    int idx_low = static_cast<int>(std::floor(idx_d));
    int idx_high = std::min(idx_low + 1, m_stepsB - 1);
    double t = idx_d - idx_low;

    const GeodesicEntry& E_low = m_table[idx_low];
    const GeodesicEntry& E_high = m_table[idx_high];

    GeodesicEntry E;
    E.b = b;
    E.captured = false;

    if (E_low.captured) {
        // b is escaping, but the low index is captured.
        // We are very close to the critical boundary. Return high values.
        E.deflection = E_high.deflection;
        E.r_min = E_high.r_min;
    } else {
        // Standard interpolation between two escaping rays
        E.deflection = (1.0 - t) * E_low.deflection + t * E_high.deflection;
        E.r_min = (1.0 - t) * E_low.r_min + t * E_high.r_min;
    }

    return E;
}

GeodesicEntry GeodesicTable::IntegrateRay(double b, double M, double dphi, int maxSteps) {
    GeodesicEntry entry;
    entry.b = b;

    // Handle b = 0 or near zero: direct radial fall
    if (b < 1e-9) {
        entry.captured = true;
        entry.deflection = 0.0;
        entry.r_min = 2.0 * M;
        return entry;
    }

    // Set starting radius (far away from the black hole)
    // Needs to be larger than b to avoid complex initial velocity
    double r_start = std::max(1000.0 * M, 10.0 * b);
    double u_start = 1.0 / r_start;

    // Initial velocity from the first integral:
    // w^2 + u^2 - 2M u^3 = 1/b^2
    double w2_init = 1.0 / (b * b) - u_start * u_start + 2.0 * M * u_start * u_start * u_start;
    double w_start = (w2_init > 0.0) ? std::sqrt(w2_init) : 0.0;

    double u = u_start;
    double w = w_start;
    double phi = 0.0;
    double u_max = u;

    double u_prev = u;
    double phi_prev = phi;

    bool captured = false;
    bool escaped = false;

    double u_horizon = 1.0 / (2.0 * M);

    for (int step = 0; step < maxSteps; ++step) {
        u_prev = u;
        phi_prev = phi;

        // Runge-Kutta 4th order integration step
        // u' = w
        // w' = -u + 3M u^2
        double k1_u = dphi * w;
        double k1_w = dphi * (-u + 3.0 * M * u * u);

        double u2 = u + 0.5 * k1_u;
        double w2 = w + 0.5 * k1_w;
        double k2_u = dphi * w2;
        double k2_w = dphi * (-u2 + 3.0 * M * u2 * u2);

        double u3 = u + 0.5 * k2_u;
        double w3 = w + 0.5 * k2_w;
        double k3_u = dphi * w3;
        double k3_w = dphi * (-u3 + 3.0 * M * u3 * u3);

        double u4 = u + k3_u;
        double w4 = w + k3_w;
        double k4_u = dphi * w4;
        double k4_w = dphi * (-u4 + 3.0 * M * u4 * u4);

        u += (k1_u + 2.0 * k2_u + 2.0 * k3_u + k4_u) / 6.0;
        w += (k1_w + 2.0 * k2_w + 2.0 * k3_w + k4_w) / 6.0;
        phi += dphi;

        if (u > u_max) {
            u_max = u;
        }

        // Check captured (crossed the event horizon)
        if (u >= u_horizon) {
            captured = true;
            break;
        }

        // Check escaped (w < 0 indicates receding, u <= u_start means returned to starting distance)
        if (w < 0.0 && u <= u_start) {
            escaped = true;
            break;
        }
    }

    // If it ran out of steps without escaping, we treat it as captured/critical
    if (!captured && !escaped) {
        captured = true;
    }

    entry.captured = captured;
    if (captured) {
        entry.deflection = 0.0;
        // Closest approach is undefined or event horizon
        entry.r_min = 2.0 * M;
    } else {
        // Interpolate phi to find exact phi_final where u = u_start
        double phi_final = phi;
        if (u != u_prev) {
            double t = (u_start - u_prev) / (u - u_prev);
            phi_final = phi_prev + t * (phi - phi_prev);
        }

        // Compute total deflection angle:
        // deflection = phi_final - (pi - 2 * arcsin(b / r_start))
        double flat_angle = M_PI - 2.0 * std::asin(b / r_start);
        entry.deflection = phi_final - flat_angle;
        entry.r_min = 1.0 / u_max;
    }

    return entry;
}
