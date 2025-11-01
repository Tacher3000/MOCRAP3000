#include "dxfreader.h"
#include <libdxfrw.h>

DxfReader::DxfReader() {
    geometry.parts.emplace_back();
    geometry.parts.back().id = ++currentPartId;
    geometry.parts.back().name = "Part_" + std::to_string(currentPartId);
}

Part& DxfReader::getCurrentPart() {
    if (geometry.parts.empty()) {
        geometry.parts.emplace_back();
        geometry.parts.back().id = ++currentPartId;
        geometry.parts.back().name = "Part_" + std::to_string(currentPartId);
    }
    return geometry.parts.back();
}

void DxfReader::read(const std::string& filePath) {
    dxfRW dxf(filePath.c_str());
    if (dxf.read(this, true)) {
    } else {
        std::cerr << "Warning: Failed to read DXF file: " << filePath << std::endl;
    }
}

Geometry DxfReader::getGeometry() const {
    return geometry;
}


void DxfReader::addLine(const DRW_Line& line) {
    getCurrentPart().lines.push_back({
        {line.basePoint.x, line.basePoint.y, line.basePoint.z},
        {line.secPoint.x, line.secPoint.y, line.secPoint.z}
    });
}

void DxfReader::addArc(const DRW_Arc& arc) {
    getCurrentPart().arcs.push_back({
        {arc.basePoint.x, arc.basePoint.y, arc.basePoint.z},
        arc.radius,
        arc.staangle,
        arc.endangle,
        arc.isccw,
        arc.thickness
    });
}

void DxfReader::addCircle(const DRW_Circle& circle) {
    getCurrentPart().circles.push_back({
        {circle.basePoint.x, circle.basePoint.y, circle.basePoint.z},
        circle.radius,
        circle.thickness
    });
}

void DxfReader::addLWPolyline(const DRW_LWPolyline& lwpoly) {
    LWPolyline poly;
    poly.elevation = lwpoly.elevation;
    poly.thickness = lwpoly.thickness;
    poly.flags = lwpoly.flags;

    for (const auto& v : lwpoly.vertlist) {
        if (v) {
            poly.vertices.push_back({
                v->x, v->y, v->bulge, v->stawidth, v->endwidth
            });
        }
    }
    getCurrentPart().lwpolylines.push_back(poly);
}

void DxfReader::addEllipse(const DRW_Ellipse& ellipse) {
    getCurrentPart().ellipses.push_back({
        {ellipse.basePoint.x, ellipse.basePoint.y, ellipse.basePoint.z},
        {ellipse.secPoint.x, ellipse.secPoint.y, ellipse.secPoint.z},
        ellipse.ratio,
        ellipse.staparam,
        ellipse.endparam,
        ellipse.isccw,
        ellipse.thickness
    });
}

void DxfReader::addInsert(const DRW_Insert& insert) {

    if (!geometry.parts.empty() && getCurrentPart().lines.empty() && getCurrentPart().arcs.empty() &&
        getCurrentPart().circles.empty() && getCurrentPart().lwpolylines.empty() && getCurrentPart().ellipses.empty()) {
        getCurrentPart().name = insert.name;
    } else {
        geometry.parts.emplace_back();
        geometry.parts.back().id = ++currentPartId;
        geometry.parts.back().name = insert.name.empty() ? ("Part_" + std::to_string(currentPartId)) : insert.name;
    }
}
