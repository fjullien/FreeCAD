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
# include <sstream>
# include <cstring>
# include <cstdlib>
# include <exception>
# include <QString>
# include <QStringList>
# include <QRegExp>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#endif

#include <QLocale>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Mod/Measure/App/Measurement.h>

#include "Geometry.h"
#include "DrawViewPart.h"
#include "DrawViewBalloon.h"
#include "DrawUtil.h"
#include "LineGroup.h"


//#include <Mod/TechDraw/App/DrawViewBalloonPy.h>  // generated from DrawViewDimensionPy.xml

using namespace TechDraw;

//===========================================================================
// DrawViewDimension
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewBalloon, TechDraw::DrawView)


DrawViewBalloon::DrawViewBalloon(void)
{
    ADD_PROPERTY_TYPE(Text ,     (""),"",App::Prop_None,"The text to be displayed");
    ADD_PROPERTY_TYPE(sourceView,(0),"",(App::PropertyType)(App::Prop_None),"Source view for balloon");
    sourceView.setScope(App::LinkScope::Global);
    sourceView.setStatus(App::Property::Hidden,true);
    Text.setContainer(this);
}

DrawViewBalloon::~DrawViewBalloon()
{

}

void DrawViewBalloon::onChanged(const App::Property* prop)
{
    Base::Console().Log("-------------- DrawViewBalloon::onChanged\n");
}

void DrawViewBalloon::onDocumentRestored()
{

}


short DrawViewBalloon::mustExecute() const
{

}

DrawViewPart* DrawViewBalloon::getViewPart() const
{
    App::DocumentObject* obj = sourceView.getValue();
    DrawViewPart* result = dynamic_cast<DrawViewPart*>(obj);
    return result;
}

App::DocumentObjectExecReturn *DrawViewBalloon::execute(void)
{
    requestPaint();
    return App::DocumentObject::execute();
}
/*
PyObject *DrawViewBallon::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewBalloonPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
*/
