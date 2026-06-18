#include "GeodesicTable.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>

int main() {
    std::cout << "========================================================\n";
    std::cout << " Schwarzschild Geodesic Physics Core Verification Suite\n";
    std::cout << "========================================================\n\n";

    double M = 1.0;
    double theoretical_b_crit = 3.0 * std::sqrt(3.0) * M; // 5.1961524...

    std::cout << "Theoretical critical impact parameter b_crit = " 
              << std::fixed << std::setprecision(7) << theoretical_b_crit << " M\n";

    // ----------------------------------------------------
    // Test 1: Finding b_crit numerically
    // ----------------------------------------------------
    std::cout << "\n[Test 1] Numerical b_crit search...\n";
    double b_start = 5.10 * M;
    double b_end = 5.30 * M;
    double db = 0.0001 * M;
    double numerical_b_crit = 0.0;

    for (double b = b_start; b <= b_end; b += db) {
        GeodesicEntry entry = GeodesicTable::IntegrateRay(b, M);
        if (!entry.captured) {
            numerical_b_crit = b;
            break;
        }
    }

    double b_crit_err_percent = std::abs(numerical_b_crit - theoretical_b_crit) / theoretical_b_crit * 100.0;
    std::cout << "  Numerical b_crit (first escape): " << numerical_b_crit << " M\n";
    std::cout << "  Relative Error: " << std::setprecision(4) << b_crit_err_percent << "%\n";
    if (b_crit_err_percent < 0.1) {
        std::cout << "  --> PASS: Critical capture radius is accurate to within 0.1%!\n";
    } else {
        std::cout << "  --> FAIL: Critical capture radius error exceeds 0.1%.\n";
    }

    // ----------------------------------------------------
    // Test 2: Bending angle vs Weak-Field Bending (4M/b)
    // ----------------------------------------------------
    std::cout << "\n[Test 2] Weak-field deflection angle check...\n";
    double test_b[] = {20.0, 50.0, 100.0};
    bool pass_test2 = true;

    std::cout << "  " << std::setw(10) << "b (M)" 
              << std::setw(15) << "Numerical (rad)" 
              << std::setw(15) << "Weak-Field (rad)" 
              << std::setw(15) << "Error %" << "\n";
    std::cout << "  ---------------------------------------------------------\n";

    for (double b : test_b) {
        GeodesicEntry entry = GeodesicTable::IntegrateRay(b, M);
        double weak_field_angle = 4.0 * M / b;
        double err_percent = std::abs(entry.deflection - weak_field_angle) / weak_field_angle * 100.0;

        std::cout << "  " << std::setw(10) << std::setprecision(2) << b 
                  << std::setw(15) << std::setprecision(6) << entry.deflection 
                  << std::setw(15) << weak_field_angle 
                  << std::setw(14) << std::setprecision(4) << err_percent << "%\n";

        // Weak field approximation gets progressively better for larger b.
        // At b=100, the error should be extremely small (typically less than 1%).
        if (b == 100.0 && err_percent > 1.0) {
            pass_test2 = false;
        }
    }

    if (pass_test2) {
        std::cout << "  --> PASS: Deflection angle matches Einstein's formula at large b!\n";
    } else {
        std::cout << "  --> FAIL: Deflection angle error is too high in the weak field limit.\n";
    }

    // ----------------------------------------------------
    // Test 3: Table generation and lookup verification
    // ----------------------------------------------------
    std::cout << "\n[Test 3] GeodesicTable lookup verification...\n";
    GeodesicTable table;
    int table_steps = 1000;
    table.GenerateTable(M, 0.1, 100.0, table_steps);

    std::cout << "  Table generated with " << table_steps << " steps from b=0.1 to b=100.0\n";

    // Test intermediate lookup values
    double lookup_tests[] = {5.15, 6.0, 7.5, 12.0, 35.0, 75.0};
    bool pass_test3 = true;

    std::cout << "  " << std::setw(10) << "b (M)"
              << std::setw(12) << "Integrate Cap"
              << std::setw(12) << "Lookup Cap"
              << std::setw(12) << "Integrate Def"
              << std::setw(12) << "Lookup Def"
              << "\n";
    std::cout << "  ---------------------------------------------------------\n";

    for (double b : lookup_tests) {
        GeodesicEntry direct = GeodesicTable::IntegrateRay(b, M);
        GeodesicEntry lookup = table.Lookup(b);

        std::cout << "  " << std::setw(10) << std::setprecision(2) << b
                  << std::setw(12) << (direct.captured ? "YES" : "NO")
                  << std::setw(12) << (lookup.captured ? "YES" : "NO")
                  << std::setw(12) << std::setprecision(5) << direct.deflection
                  << std::setw(12) << lookup.deflection
                  << "\n";

        if (direct.captured != lookup.captured) {
            pass_test3 = false;
        }
        if (!direct.captured && !lookup.captured) {
            double def_diff = std::abs(direct.deflection - lookup.deflection);
            // Deflection diverges near b_crit, so we relax tolerance near 5.196
            double tol = (b < 6.0) ? 0.5 : 0.01;
            if (def_diff > tol) {
                pass_test3 = false;
                std::cout << "    * Lookup mismatch at b=" << b << " (diff=" << def_diff << ")\n";
            }
        }
    }

    if (pass_test3) {
        std::cout << "  --> PASS: GeodesicTable lookup matches direct integration within tolerance!\n";
    } else {
        std::cout << "  --> FAIL: GeodesicTable lookup deviates significantly from direct integration.\n";
    }

    // ----------------------------------------------------
    // Additional: Accretion Disk intersection metrics (r_min)
    // ----------------------------------------------------
    std::cout << "\n[Accretion Disk Metrics] Closest approach radius (r_min) vs b:\n";
    std::cout << "  " << std::setw(10) << "b (M)"
              << std::setw(15) << "r_min (M)" 
              << std::setw(25) << "Equatorial disk crossing?" << "\n";
    std::cout << "  ---------------------------------------------------------\n";

    double sample_b[] = {5.5, 6.0, 7.0, 10.0, 15.0, 20.0};
    for (double b : sample_b) {
        GeodesicEntry entry = table.Lookup(b);
        std::cout << "  " << std::setw(10) << std::setprecision(2) << b
                  << std::setw(15) << std::setprecision(5) << entry.r_min
                  << std::setw(25) << (entry.r_min <= 12.0 ? "Yes (crosses inner/outer)" : "No (passes outside)") 
                  << "\n";
    }

    std::cout << "\n========================================================\n";
    std::cout << " Verification Finished.\n";
    std::cout << "========================================================\n";

    return (b_crit_err_percent < 0.1 && pass_test2 && pass_test3) ? 0 : 1;
}
