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

    dimLines = new QGIDimLines();
    addToGroup(dimLines);

    aHead1 = new QGIArrow();
    addToGroup(aHead1);

    aHead1->setZValue(ZVALUE::DIMENSION);
    dimLines->setZValue(ZVALUE::DIMENSION);
    dimLines->setStyle(Qt::SolidLine);

    origin = new QPointF;
    origin->setX(0.0);
    origin->setY(0.0);

    originPosSet = false;

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
    //Base::Console().Message("%s::parentViewMousePressed from %s\n", this->getViewName(), view->getViewName());

    //Base::Console().Log("X = %f\n",pos.x());
    //Base::Console().Log("Y = %f\n",pos.y());

    if (originPosSet == false) {
        origin->setX(pos.x());
        origin->setY(pos.y());
        originPosSet = true;
    }

    draw();
}


void QGIViewBalloon::setViewPartFeature(TechDraw::DrawViewBalloon *obj)
{
    if(obj == 0)
        return;

    setViewFeature(static_cast<TechDraw::DrawView *>(obj));

    // Set the QGIGroup Properties based on the DrawView
   // float x = Rez::guiX(obj->X.getValue());
    //float y = Rez::guiX(-obj->Y.getValue());

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
    draw();
}

void QGIViewBalloon::draw()
{
    if (!isVisible()) {                                                //should this be controlled by parent ViewPart?
        return;
    }

    if (originPosSet == false) {
        return;
    }

    /*datumLabel->show();*/
    show();

    TechDraw::DrawViewBalloon *balloon = dynamic_cast<TechDraw::DrawViewBalloon *>(getViewObject());
    if((!balloon) ||                                                       //nothing to draw, don't try
       (!balloon->isDerivedFrom(TechDraw::DrawViewBalloon::getClassTypeId()))) {
        //datumLabel->hide();
        hide();
        return;
    }

    const TechDraw::DrawViewPart *refObj = balloon->getViewPart();
    if(!refObj->hasGeometry()) {                                       //nothing to draw yet (restoring)
        //datumLabel->hide();
        hide();
        return;
    }

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return;
    }

    m_lineWidth = Rez::guiX(vp->LineWidth.getValue());

    QPainterPath dLinePath;                                                 //radius dimension line path
    dLinePath.moveTo(origin->x(), origin->y());
    dLinePath.lineTo(origin->x() + 100, origin->y() + 100);
    dLinePath.lineTo(origin->x() + 200, origin->y() + 100);

    dimLines->setPath(dLinePath);

    // redraw the Dimension and the parent View
    if (hasHover && !isSelected()) {
        aHead1->setPrettyPre();
        dimLines->setPrettyPre();
    } else if (isSelected()) {
        aHead1->setPrettySel();
        dimLines->setPrettySel();
    } else {
        aHead1->setPrettyNormal();
        dimLines->setPrettyNormal();
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
  /* if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            datumLabel->setSelected(true);
        } else {
            datumLabel->setSelected(false);
        }
        draw();
    }*/
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
    aHead1->setWidth(aHead1->getWidth()/svgLineFactor);
}

void QGIViewBalloon::setPens(void)
{
    dimLines->setWidth(m_lineWidth);
    aHead1->setWidth(m_lineWidth);
}

QColor QGIViewBalloon::getNormalColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    m_colNormal = fcColor.asValue<QColor>();
/*
    auto dim( dynamic_cast<TechDraw::QGIViewBalloon*>(getViewObject()) );
    if( dim == nullptr )
        return m_colNormal;

    auto vp = static_cast<ViewProviderBalloon*>(getViewProvider(getViewObject()));
    if ( vp == nullptr ) {
        return m_colNormal;
    }

    m_colNormal = vp->Color.getValue().asValue<QColor>();*/
    return m_colNormal;
}

#include <Mod/TechDraw/Gui/moc_QGIViewBalloon.cpp>
