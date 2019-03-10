/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
  #include <BRep_Builder.hxx>
  #include <TopoDS_Compound.hxx>
  # include <TopoDS_Shape.hxx>
  # include <TopoDS_Edge.hxx>
  # include <TopoDS.hxx>
  # include <BRepAdaptor_Curve.hxx>
  # include <Precision.hxx>

  # include <QGraphicsScene>
  # include <QGraphicsSceneMouseEvent>
  # include <QPainter>
  # include <QPaintDevice>
  # include <QSvgGenerator>

  # include <math.h>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>
#include <Gui/Command.h>
#include <string>

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "QGIArrow.h"
#include "QGIDimLines.h"
#include "QGIViewBalloon.h"
#include "ViewProviderBalloon.h"
#include "DrawGuiUtil.h"
#include "QGIViewPart.h"
#include "QGIViewDimension.h"
#include "QGVPage.h"
#include "MDIViewPage.h"

//TODO: hide the Qt coord system (+y down).  

using namespace TechDraw;
using namespace TechDrawGui;

//**************************************************************
QGIViewBalloon::QGIViewBalloon() :
    hasHover(false),
    m_lineWidth(0.0)
{
    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setCacheMode(QGraphicsItem::NoCache);

    datumLabel = new QGIDatumLabel();
    addToGroup(datumLabel);
    datumLabel->setColor(getNormalColor());
    datumLabel->setPrettyNormal();

    dimLines = new QGIDimLines();
    addToGroup(dimLines);

    balloonShape = new QGIDimLines();
    addToGroup(balloonShape);

    aHead1 = new QGIArrow();
    addToGroup(aHead1);

    datumLabel->setZValue(ZVALUE::LABEL);
    aHead1->setZValue(ZVALUE::DIMENSION);
    dimLines->setZValue(ZVALUE::DIMENSION);
    dimLines->setStyle(Qt::SolidLine);

    balloonShape->setZValue(ZVALUE::DIMENSION);
    balloonShape->setStyle(Qt::SolidLine);

    oldLabelCenter = new QPointF;
    oldLabelCenter->setX(0.0);
    oldLabelCenter->setY(0.0);

    datumLabel->setPosFromCenter(0, 0);

    // connecting the needed slots and signals
    QObject::connect(
        datumLabel, SIGNAL(dragging(bool)),
        this  , SLOT  (datumLabelDragged(bool)));

    QObject::connect(
        datumLabel, SIGNAL(dragFinished()),
        this  , SLOT  (datumLabelDragFinished()));

    QObject::connect(
        datumLabel, SIGNAL(selected(bool)),
        this  , SLOT  (select(bool)));

    QObject::connect(
        datumLabel, SIGNAL(hover(bool)),
        this  , SLOT  (hover(bool)));

    toggleBorder(false);
    setZValue(ZVALUE::DIMENSION);                    //note: this won't paint dimensions over another View if it stacks
                                                     //above this Dimension's parent view.   need Layers?

}

void QGIViewBalloon::connect(QGIView *parent)
{
    auto bnd = boost::bind(&QGIViewBalloon::parentViewMousePressed, this, _1, _2);
    parent->signalSelectPoint.connect(bnd);
}

void QGIViewBalloon::parentViewMousePressed(QGIView *view, QPointF pos)
{
    Q_UNUSED(view);
    //Base::Console().Message("%s::parentViewMousePressed from %s\n", this->getViewName(), view->getViewName());

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    auto balloon( dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()) );
    if( balloon == nullptr )
        return;

    if (balloon->originIsSet.getValue() == false) {
        balloon->originX.setValue(pos.x());
        balloon->originY.setValue(pos.y());
        balloon->originIsSet.setValue(true);

        MDIViewPage* mdi = getMDIViewPage();
        QGVPage* page;
        if (mdi != nullptr) {
            page = mdi->getQGVPage();
        }

        QString labelText = QString::fromUtf8(std::to_string(page->balloonIndex).c_str());
        balloon->Text.setValue(std::to_string(page->balloonIndex++).c_str());

        QFont font = datumLabel->getFont();
        font.setPointSizeF(Rez::guiX(vp->Fontsize.getValue()));
        font.setFamily(QString::fromUtf8(vp->Font.getValue()));
        datumLabel->setFont(font);
        prepareGeometryChange();

        datumLabel->setPosFromCenter(pos.x() + 200, pos.y() -200);
        datumLabel->setDimString(labelText);
    }

    draw();
}


void QGIViewBalloon::setViewPartFeature(TechDraw::DrawViewBalloon *balloon)
{
    if(balloon == 0)
        return;

    setViewFeature(static_cast<TechDraw::DrawView *>(balloon));

    float x = Rez::guiX(balloon->X.getValue());
    float y = Rez::guiX(-balloon->Y.getValue());

    datumLabel->setPosFromCenter(x, y);

    QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
    datumLabel->setDimString(labelText);

    updateDim();

    draw();
}

void QGIViewBalloon::select(bool state)
{
    setSelected(state);
    draw();
}

void QGIViewBalloon::hover(bool state)
{
    hasHover = state;
    draw();
}

void QGIViewBalloon::updateView(bool update)
{
    Q_UNUSED(update);
    auto balloon( dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()) );
    if( balloon == nullptr )
        return;

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    if (update) {
        QString labelText = QString::fromUtf8(balloon->Text.getStrValue().data());
        datumLabel->setDimString(labelText);
    }
/*
    if (update||
        dim->X.isTouched() ||
        dim->Y.isTouched()) {
        float x = Rez::guiX(dim->X.getValue());
        float y = Rez::guiX(dim->Y.getValue());
    Base::Console().Log("2X = %f\n",x);
    Base::Console().Log("2Y = %f\n",y);
        datumLabel->setPosFromCenter(x,-y);
        updateDim();
     }
     else if(vp->Fontsize.isTouched() ||
               vp->Font.isTouched()) {
         QFont font = datumLabel->getFont();
         font.setPointSizeF(Rez::guiX(vp->Fontsize.getValue()));
         font.setFamily(QString::fromLatin1(vp->Font.getValue()));
         datumLabel->setFont(font);
         updateDim();
    } else if (vp->LineWidth.isTouched()) {           //never happens!!
        m_lineWidth = vp->LineWidth.getValue();
        updateDim();
    } else {
        updateDim();
    }
*/
    updateDim();
    draw();
}

void QGIViewBalloon::updateDim(bool obtuse)
{
    (void) obtuse;
    const auto balloon( dynamic_cast<TechDraw::DrawViewBalloon *>(getViewObject()) );
    if( balloon == nullptr ) {
        return;
    }
    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    if (balloon->originIsSet.getValue() == false)
        return;

    QFont font = datumLabel->getFont();
    font.setPointSizeF(Rez::guiX(vp->Fontsize.getValue()));
    font.setFamily(QString::fromUtf8(vp->Font.getValue()));
    datumLabel->setFont(font);
    //prepareGeometryChange();
    //datumLabel->setTolString();
    //datumLabel->setPosFromCenter(datumLabel->X(),datumLabel->Y());
}

void QGIViewBalloon::datumLabelDragged(bool ctrl)
{
    draw_modifier(ctrl);
}

void QGIViewBalloon::datumLabelDragFinished()
{
    auto dim( dynamic_cast<TechDraw::DrawViewBalloon *>(getViewObject()) );

    if( dim == nullptr ) {
        return;
    }

    double x = Rez::appX(datumLabel->X()),
           y = Rez::appX(datumLabel->Y());
    Gui::Command::openCommand("Drag Balloon");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.X = %f", dim->getNameInDocument(), x);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Y = %f", dim->getNameInDocument(), -y);
    Gui::Command::commitCommand();
}

void QGIViewBalloon::draw()
{
    draw_modifier(false);
}

void QGIViewBalloon::draw_modifier(bool modifier)
{
    if (!isVisible()) {                                                //should this be controlled by parent ViewPart?
        return;
    }

    TechDraw::DrawViewBalloon *balloon = dynamic_cast<TechDraw::DrawViewBalloon *>(getViewObject());
    if((!balloon) ||                                                       //nothing to draw, don't try
       (!balloon->isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId()))) {
        datumLabel->hide();
        hide();
        return;
    }

    if (balloon->originIsSet.getValue() == false) {
        return;
    }

    datumLabel->show();
    show();

    const TechDraw::DrawViewPart *refObj = balloon->getViewPart();
    if(!refObj->hasGeometry()) {                                       //nothing to draw yet (restoring)
        datumLabel->hide();
        hide();
        return;
    }

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    m_colNormal = getNormalColor();
    datumLabel->setColor(m_colNormal);

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());

    double textWidth = datumLabel->getDimText()->boundingRect().width();
    double textHeight = datumLabel->getDimText()->boundingRect().height();
    QRectF  mappedRect = mapRectFromItem(datumLabel, datumLabel->boundingRect());
    Base::Vector3d lblCenter = Base::Vector3d(mappedRect.center().x(), mappedRect.center().y(), 0.0);

    if (balloon->isLocked()) {
        lblCenter.x = (oldLabelCenter->x());
        lblCenter.y = (oldLabelCenter->y());
        datumLabel->setFlag(QGraphicsItem::ItemIsMovable, false);
    } else
        datumLabel->setFlag(QGraphicsItem::ItemIsMovable, true);

    Base::Vector3d dLineStart;
    Base::Vector3d kinkPoint;
    double kinkLength = Rez::guiX(5.0);                                //sb % of horizontal dist(lblCenter,curveCenter)???

    float orginX = balloon->originX.getValue();
    float orginY = balloon->originY.getValue();

    QPainterPath balloonPath;
    double ballonRadius = sqrt(pow((textHeight / 2.0), 2) + pow((textWidth / 2.0), 2));
    balloonPath.moveTo(lblCenter.x, lblCenter.y);
    balloonPath.addEllipse(lblCenter.x - ballonRadius,lblCenter.y - ballonRadius, ballonRadius * 2, ballonRadius * 2);

    double offset = ballonRadius;
    offset = (lblCenter.x < orginX) ? offset : -offset;
    dLineStart.y = lblCenter.y;
    dLineStart.x = lblCenter.x + offset;                                     //start at right or left of label
    kinkLength = (lblCenter.x < orginX) ? kinkLength : -kinkLength;
    kinkPoint.y = dLineStart.y;
    kinkPoint.x = dLineStart.x + kinkLength;

    QPainterPath dLinePath;                                                 //radius dimension line path
    dLinePath.moveTo(dLineStart.x, dLineStart.y);
    dLinePath.lineTo(kinkPoint.x, kinkPoint.y);

    if (modifier) {
        balloon->originX.setValue(orginX + lblCenter.x - oldLabelCenter->x());
        balloon->originY.setValue(orginY + lblCenter.y - oldLabelCenter->y());
    }

    orginX = balloon->originX.getValue();
    orginY = balloon->originY.getValue();

    dLinePath.lineTo(orginX, orginY);

    oldLabelCenter->setX(lblCenter.x);
    oldLabelCenter->setY(lblCenter.y);

    dimLines->setPath(dLinePath);
    balloonShape->setPath(balloonPath);

    aHead1->setStyle(QGIArrow::getPrefArrowStyle());
    aHead1->setSize(QGIArrow::getPrefArrowSize());
    aHead1->draw();

    Base::Vector3d orign(orginX, orginY, 0.0);
    Base::Vector3d dirArrowLine = (orign - kinkPoint).Normalize();
    float arAngle = atan2(dirArrowLine.y, dirArrowLine.x) * 180 / M_PI;

    aHead1->setPos(orginX, orginY);
    aHead1->setRotation(arAngle);
    aHead1->show();

    // redraw the Dimension and the parent View
    if (hasHover && !isSelected()) {
        aHead1->setPrettyPre();
        dimLines->setPrettyPre();
        balloonShape->setPrettyPre();
    } else if (isSelected()) {
        aHead1->setPrettySel();
        dimLines->setPrettySel();
        balloonShape->setPrettySel();
    } else {
        aHead1->setPrettyNormal();
        dimLines->setPrettyNormal();
        balloonShape->setPrettyNormal();
    }

    update();
    if (parentItem()) {
        //TODO: parent redraw still required with new frame/label??
        parentItem()->update();
    } else {
        Base::Console().Log("INFO - QGIVD::draw - no parent to update\n");
    }

}

void QGIViewBalloon::drawBorder(void)
{
//Dimensions have no border!
//    Base::Console().Message("TRACE - QGIViewDimension::drawBorder - doing nothing!\n");
}

QVariant QGIViewBalloon::itemChange(GraphicsItemChange change, const QVariant &value)
{
   if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            datumLabel->setSelected(true);
        } else {
            datumLabel->setSelected(false);
        }
        draw();
    }
    return QGIView::itemChange(change, value);
}

void QGIViewBalloon::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    QPaintDevice* hw = painter->device();
    QSvgGenerator* svg = dynamic_cast<QSvgGenerator*>(hw);
    setPens();
    //double arrowSaveWidth = aHead1->getWidth();
    if (svg) {
        setSvgPens();
    } else {
        setPens();
    }
    QGIView::paint (painter, &myOption, widget);
    setPens();
}

void QGIViewBalloon::setSvgPens(void)
{
    double svgLineFactor = 3.0;                     //magic number.  should be a setting somewhere.
    dimLines->setWidth(m_lineWidth/svgLineFactor);
    balloonShape->setWidth(m_lineWidth/svgLineFactor);
    aHead1->setWidth(aHead1->getWidth()/svgLineFactor);
}

void QGIViewBalloon::setPens(void)
{
    dimLines->setWidth(m_lineWidth);
    balloonShape->setWidth(m_lineWidth);
    aHead1->setWidth(m_lineWidth);
}

QColor QGIViewBalloon::getNormalColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    m_colNormal = fcColor.asValue<QColor>();

    auto balloon( dynamic_cast<TechDraw::DrawViewBalloon*>(getViewObject()) );
    if( balloon == nullptr )
        return m_colNormal;

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return m_colNormal;
    }

    m_colNormal = vp->Color.getValue().asValue<QColor>();
    return m_colNormal;
}

#include <Mod/TechDraw/Gui/moc_QGIViewBalloon.cpp>
