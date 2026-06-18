#ifndef GEODESIC_TABLE_H
#define GEODESIC_TABLE_H

#include <vector>

struct GeodesicEntry {
    double b;           // Impact parameter
    bool captured;      // True if captured (r <= 2M), false if escaped
    double deflection;  // Total bending/deflection angle (radians)
    double r_min;       // Closest approach radius reached (if escaped)
};

class GeodesicTable {
public:
    GeodesicTable();
    ~GeodesicTable() = default;

    // Generates the lookup table for a given mass M, over the range [b_min, b_max] with steps_b divisions
    void GenerateTable(double M, double b_min, double b_max, int steps_b);

    // Looks up the geodesic parameters for a given impact parameter b using linear interpolation
    GeodesicEntry Lookup(double b) const;

    // Runs a single ray integration for a given impact parameter b and mass M
    static GeodesicEntry IntegrateRay(double b, double M, double dphi = 0.001, int maxSteps = 100000);

    // Getters
    double GetM() const { return m_M; }
    double GetBMin() const { return m_bMin; }
    double GetBMax() const { return m_bMax; }
    int GetStepsB() const { return m_stepsB; }
    const std::vector<GeodesicEntry>& GetTable() const { return m_table; }

private:
    double m_M;
    double m_bMin;
    double m_bMax;
    int m_stepsB;
    std::vector<GeodesicEntry> m_table;
};

#endif // GEODESIC_TABLE_H
