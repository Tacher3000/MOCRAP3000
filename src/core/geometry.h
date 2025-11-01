#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <string>
#include <stdexcept>

struct Coord {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Point {
    double x = 0.0;
    double y = 0.0;
};

struct Contour {
    std::vector<Point> points;
    bool isHole = false;
};

struct Polygon {
    std::vector<Contour> contours;
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
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

struct Part {
    std::vector<Line> lines;
    std::vector<Arc> arcs;
    std::vector<Circle> circles;
    std::vector<LWPolyline> lwpolylines;
    std::vector<Ellipse> ellipses;
    std::string name;
    int id = 0;
    Polygon polygon;
};


struct Geometry {
    std::vector<Part> parts;

    const Part& getSinglePart() const {
        if (parts.empty()) {
            throw std::runtime_error("No parts loaded in geometry.");
        }
        return parts[0];
    }
};

#endif // GEOMETRY_H
