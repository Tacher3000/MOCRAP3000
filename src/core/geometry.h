#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>

struct Coord {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Line {
    Coord start;
    Coord end;
};

struct Arc {
    Coord center;
    double radius;
    double startAngle;
    double endAngle;
    int isCounterClockwise;
    double thickness;
};

struct Circle {
    Coord center;
    double radius;
    double thickness;
};

struct Vertex2D {
    double x;
    double y;
    double bulge = 0.0;
    double startWidth = 0.0;
    double endWidth = 0.0;
};

struct LWPolyline {
    std::vector<Vertex2D> vertices;
    double elevation = 0.0;
    double thickness = 0.0;
    int flags = 0;
};

struct Geometry {
    std::vector<Line> lines;
    std::vector<Arc> arcs;
    std::vector<Circle> circles;
    std::vector<LWPolyline> lwpolylines;
};

#endif
