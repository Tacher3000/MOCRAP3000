#ifndef GEOMETRYNORMALIZER_H
#define GEOMETRYNORMALIZER_H

#include "geometry.h"
#include <vector>

namespace geometry {
    Contour toContour(const LWPolyline& poly);
    Polygon normalizePart(const Part& part);
}

#endif // GEOMETRYNORMALIZER_H
