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

// void DxfReader::addEllipse(const DRW_Ellipse& ellipse) {
//     geometry.ellipses.push_back({
//         {ellipse.basePoint.x, ellipse.basePoint.y, ellipse.basePoint.z},
//         {ellipse.secPoint.x, ellipse.secPoint.y, ellipse.secPoint.z},
//         ellipse.ratio,
//         ellipse.staparam,
//         ellipse.endparam,
//         ellipse.isccw,
//         ellipse.thickness
//     });
// }

// void DxfReader::addPolyline(const DRW_Polyline& polyline) {
//     Polyline poly;
//     poly.thickness = polyline.thickness;
//     poly.flags = polyline.flags;
//     for (const auto& v : polyline.vertlist) {
//         const DRW_Vertex* vert = v.get();
//         poly.vertices.push_back({
//             {vert->basePoint.x, vert->basePoint.y, vert->basePoint.z},
//             vert->bulge,
//             vert->stawidth,
//             vert->endwidth
//         });
//     }
//     geometry.polylines.push_back(poly);
// }

// void DxfReader::addSpline(const DRW_Spline* spline) {
//     Spline spl;
//     spl.degree = spline->degree;
//     spl.flags = spline->flags;
//     spl.startTangentX = spline->tgStart.x;
//     spl.startTangentY = spline->tgStart.y;
//     spl.startTangentZ = spline->tgStart.z;
//     spl.endTangentX = spline->tgEnd.x;
//     spl.endTangentY = spline->tgEnd.y;
//     spl.endTangentZ = spline->tgEnd.z;
//     for (const auto& cp : spline->controllist) {
//         spl.controlPoints.push_back({cp->x, cp->y, cp->z});
//     }
//     for (const auto& fp : spline->fitlist) {
//         spl.fitPoints.push_back({fp->x, fp->y, fp->z});
//     }
//     spl.knotValues = spline->knotslist;
//     geometry.splines.push_back(spl);
// }
