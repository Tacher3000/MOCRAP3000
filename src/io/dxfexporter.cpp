#include "dxfexporter.h"
#include "../core/geometryutils.h"
#include <fstream>
#include <cmath>
#include <iomanip>
#include <QTransform>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
void writeLine(std::ofstream& out, double x1, double y1, double x2, double y2, const std::string& layer) {
    out << "  0\nLINE\n  8\n" << layer << "\n";
    out << "100\nAcDbEntity\n100\nAcDbLine\n";
    out << " 10\n" << x1 << "\n 20\n" << y1 << "\n 30\n0.0\n";
    out << " 11\n" << x2 << "\n 21\n" << y2 << "\n 31\n0.0\n";
}

void writeCircle(std::ofstream& out, double cx, double cy, double r, const std::string& layer) {
    out << "  0\nCIRCLE\n  8\n" << layer << "\n";
    out << "100\nAcDbEntity\n100\nAcDbCircle\n";
    out << " 10\n" << cx << "\n 20\n" << cy << "\n 30\n0.0\n";
    out << " 40\n" << r << "\n";
}

void writeArc(std::ofstream& out, double cx, double cy, double r, double startAngle, double endAngle, const std::string& layer) {
    out << "  0\nARC\n  8\n" << layer << "\n";
    out << "100\nAcDbEntity\n100\nAcDbCircle\n100\nAcDbArc\n";
    out << " 10\n" << cx << "\n 20\n" << cy << "\n 30\n0.0\n";
    out << " 40\n" << r << "\n";
    out << " 50\n" << (startAngle * 180.0 / M_PI) << "\n";
    out << " 51\n" << (endAngle * 180.0 / M_PI) << "\n";
}

void writeLwPolyline(std::ofstream& out, const LWPolyline& poly, const QTransform& t, const std::string& layer) {
    if (poly.vertices.empty()) return;
    out << "  0\nLWPOLYLINE\n  8\n" << layer << "\n";
    out << "100\nAcDbEntity\n100\nAcDbPolyline\n";
    out << " 90\n" << poly.vertices.size() << "\n";
    out << " 70\n" << poly.flags << "\n";

    for (const auto& v : poly.vertices) {
        QPointF pt = t.map(QPointF(v.x, v.y));
        out << " 10\n" << pt.x() << "\n 20\n" << pt.y() << "\n";
        if (std::abs(v.bulge) > 1e-6) {
            out << " 42\n" << v.bulge << "\n";
        }
    }
}
}

bool DxfExporter::exportSolution(const NestingSolution& solution, const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) return false;

    out << std::fixed << std::setprecision(6);

    out << "  0\nSECTION\n  2\nHEADER\n  9\n$ACADVER\n  1\nAC1015\n  0\nENDSEC\n";
    out << "  0\nSECTION\n  2\nTABLES\n  0\nENDSEC\n";
    out << "  0\nSECTION\n  2\nBLOCKS\n  0\nENDSEC\n";
    out << "  0\nSECTION\n  2\nENTITIES\n";

    double sheetSpacing = 100.0;
    double currentOffsetX = 0.0;
    std::map<int, double> sheetOffsets;

    for (const auto& sheet : solution.usedSheets) {
        sheetOffsets[sheet.id] = currentOffsetX;

        double x1 = currentOffsetX;
        double y1 = 0;
        double x2 = currentOffsetX + sheet.width;
        double y2 = sheet.height;

        writeLine(out, x1, y1, x2, y1, "SHEETS");
        writeLine(out, x2, y1, x2, y2, "SHEETS");
        writeLine(out, x2, y2, x1, y2, "SHEETS");
        writeLine(out, x1, y2, x1, y1, "SHEETS");

        currentOffsetX += sheet.width + sheetSpacing;
    }

    for (const auto& placedPart : solution.placedParts) {
        auto it = solution.partsMap.find(placedPart.originalPartId);
        if (it == solution.partsMap.end()) continue;

        const Part& originalPart = it->second;

        QPainterPath path = geometry::toQPainterPath(originalPart.polygon);
        QRectF bRect = path.boundingRect();

        QTransform t1;
        t1.translate(-bRect.left(), -bRect.top());

        QTransform r;
        r.rotate(placedPart.rotation);

        QRectF rotatedRect = (t1 * r).map(path).boundingRect();

        QTransform t2;
        t2.translate(-rotatedRect.left(), -rotatedRect.top());

        QTransform t3;
        t3.translate(sheetOffsets[placedPart.sheetId] + placedPart.x, placedPart.y);

        QTransform finalTransform = t1 * r * t2 * t3;

        for (const auto& line : originalPart.lines) {
            QPointF p1 = finalTransform.map(QPointF(line.start.x, line.start.y));
            QPointF p2 = finalTransform.map(QPointF(line.end.x, line.end.y));
            writeLine(out, p1.x(), p1.y(), p2.x(), p2.y(), "PARTS");
        }

        for (const auto& circle : originalPart.circles) {
            QPointF c = finalTransform.map(QPointF(circle.center.x, circle.center.y));
            writeCircle(out, c.x(), c.y(), circle.radius, "PARTS");
        }

        for (const auto& arc : originalPart.arcs) {
            QPointF c = finalTransform.map(QPointF(arc.center.x, arc.center.y));

            QPointF p1(arc.center.x + arc.radius * std::cos(arc.startAngle),
                       arc.center.y + arc.radius * std::sin(arc.startAngle));
            QPointF p2(arc.center.x + arc.radius * std::cos(arc.endAngle),
                       arc.center.y + arc.radius * std::sin(arc.endAngle));

            p1 = finalTransform.map(p1);
            p2 = finalTransform.map(p2);

            double newStart = std::atan2(p1.y() - c.y(), p1.x() - c.x());
            double newEnd = std::atan2(p2.y() - c.y(), p2.x() - c.x());

            if (newStart < 0) newStart += 2 * M_PI;
            if (newEnd < 0) newEnd += 2 * M_PI;

            if (!arc.isCounterClockwise) {
                std::swap(newStart, newEnd);
            }

            writeArc(out, c.x(), c.y(), arc.radius, newStart, newEnd, "PARTS");
        }

        for (const auto& poly : originalPart.lwpolylines) {
            writeLwPolyline(out, poly, finalTransform, "PARTS");
        }
    }

    out << "  0\nENDSEC\n  0\nEOF\n";
    return true;
}
