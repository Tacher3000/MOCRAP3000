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

struct Ellipse {
    Coord center;
    Coord majorAxisEnd;
    double ratio;
    double startParam;
    double endParam;
    int isCounterClockwise;
    double thickness;
};

struct Vertex {
    Coord location;
    double bulge = 0.0;
    double startWidth = 0.0;
    double endWidth = 0.0;
    double tangentDir = 0.0;
};

struct Polyline {
    std::vector<Vertex> vertices;
    double thickness = 0.0;
    int flags = 0;
};

struct Spline {
    std::vector<Coord> controlPoints;
    std::vector<Coord> fitPoints;
    std::vector<double> knotValues;
    int degree;
    int flags;
    double startTangentX;
    double startTangentY;
    double startTangentZ;
    double endTangentX;
    double endTangentY;
    double endTangentZ;
};

struct Geometry {
    std::vector<Line> lines;
    std::vector<Arc> arcs;
    std::vector<Circle> circles;
    std::vector<LWPolyline> lwpolylines;
    std::vector<Ellipse> ellipses;
    std::vector<Polyline> polylines;
    std::vector<Spline> splines;
};

#endif
