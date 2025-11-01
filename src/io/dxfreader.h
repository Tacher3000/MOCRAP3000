#ifndef DXFREADER_H
#define DXFREADER_H

#include "../core/geometry.h"
#include <drw_interface.h>
#include <string>
#include <stdexcept>
#include <iostream>

/**
 * @brief Класс DxfReader обрабатывает события парсинга DXF-файла.
 * Наследуется от DRW_Interface из libdxfrw.
 */
class DxfReader : public DRW_Interface {
public:
    DxfReader();
    void read(const std::string& filePath);
    Geometry getGeometry() const;

private:
    Geometry geometry;
    int currentPartId = 0;

    Part& getCurrentPart();

    void addLine(const DRW_Line& line) override;
    void addArc(const DRW_Arc& arc) override;
    void addCircle(const DRW_Circle& circle) override;
    void addEllipse(const DRW_Ellipse& ellipse) override;
    void addLWPolyline(const DRW_LWPolyline& lwpoly) override;
    void addInsert(const DRW_Insert& insert) override;

    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText&) override {}
    void addText(const DRW_Text&) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}

    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeAppId() override {}

};

#endif // DXFREADER_H
