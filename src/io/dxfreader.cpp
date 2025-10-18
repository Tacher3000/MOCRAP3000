#include "dxfreader.h"
#include <libdxfrw.h>

void DxfReader::read(const std::string& filePath) {
    dxfRW dxf(filePath.c_str());
    dxf.read(this, true);
}

Geometry DxfReader::getGeometry() const {
    return geometry;
}

void DxfReader::addLine(const DRW_Line& line) {
    geometry.lines.push_back({{line.basePoint.x, line.basePoint.y, line.basePoint.z}, {line.secPoint.x, line.secPoint.y, line.secPoint.z}});
}

void DxfReader::addArc(const DRW_Arc& arc) {
    geometry.arcs.push_back({{arc.basePoint.x, arc.basePoint.y, arc.basePoint.z}, arc.radius, arc.staangle, arc.endangle, arc.isccw, arc.thickness});
}

void DxfReader::addCircle(const DRW_Circle& circle) {
    geometry.circles.push_back({{circle.basePoint.x, circle.basePoint.y, circle.basePoint.z}, circle.radius, circle.thickness});
}

void DxfReader::addLWPolyline(const DRW_LWPolyline& lwpoly) {
    LWPolyline poly;
    poly.elevation = lwpoly.elevation;
    poly.thickness = lwpoly.thickness;
    poly.flags = lwpoly.flags;
    for (const auto& v : lwpoly.vertlist) {
        const DRW_Vertex2D* vert = v.get();
        poly.vertices.push_back({vert->x, vert->y, vert->bulge, vert->stawidth, vert->endwidth});
    }
    geometry.lwpolylines.push_back(poly);
}
