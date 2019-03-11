#!/usr/bin/env python
# -*- coding: utf-8 -*-

# test script for TechDraw module
# creates a page and 2 views
# adds 1 length dimension to view1
# adds 1 radius dimension to view2
from __future__ import print_function

import FreeCAD
import Part
import Measure
import TechDraw
import os

def DVBalloonTest():
    path = os.path.dirname(os.path.abspath(__file__))
    print ('TDBalloon path: ' + path)
    templateFileSpec = path + '/TestTemplate.svg'

    FreeCAD.newDocument("TDBalloon")
    FreeCAD.setActiveDocument("TDBalloon")
    FreeCAD.ActiveDocument=FreeCAD.getDocument("TDBalloon")

    #make source feature
    box = FreeCAD.ActiveDocument.addObject("Part::Box","Box")
    sphere = FreeCAD.ActiveDocument.addObject("Part::Sphere","Sphere")

    #make a page
    page = FreeCAD.ActiveDocument.addObject('TechDraw::DrawPage','Page')
    FreeCAD.ActiveDocument.addObject('TechDraw::DrawSVGTemplate','Template')
    FreeCAD.ActiveDocument.Template.Template = templateFileSpec
    FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
    page.Scale = 5.0
#    page.ViewObject.show()   # unit tests run in console mode 

    #make Views
    view1 = FreeCAD.ActiveDocument.addObject('TechDraw::DrawViewPart','View')
    FreeCAD.ActiveDocument.View.Source = [FreeCAD.ActiveDocument.Box]
    rc = page.addView(view1)
    view1.X = 30
    view1.Y = 150
    view2 = FreeCAD.activeDocument().addObject('TechDraw::DrawViewPart','View001')
    FreeCAD.activeDocument().View001.Source = [FreeCAD.activeDocument().Sphere]
    rc = page.addView(view2)
    view2.X = 220
    view2.Y = 150
    FreeCAD.ActiveDocument.recompute()

    print("Place balloon")
    balloon1 = FreeCAD.ActiveDocument.addObject('TechDraw::DrawViewBalloon','Balloon1')
    balloon1.sourceView=view1
    balloon1.originIsSet=1
    balloon1.originX=view1.X + 20
    balloon1.originY=view1.Y + 20
    balloon1.Text="1"
    balloon1.Y=balloon1.originX + 20
    balloon1.X=balloon1.originY + 20

    print("adding balloon1 to page")
    rc = page.addView(balloon1)

    balloon2 = FreeCAD.ActiveDocument.addObject('TechDraw::DrawViewBalloon','Balloon2')
    balloon2.sourceView=view2
    balloon2.originIsSet=1
    balloon2.originX=view2.X + 20
    balloon2.originY=view2.Y + 20
    balloon2.Text="2"
    balloon2.Y=balloon2.originX + 20
    balloon2.X=balloon2.originY + 20

    print("adding balloon2 to page")
    rc = page.addView(balloon2)

    print("finished length dimension")

    #make radius dimension
#    print("making radius dimension")
#    dim2 = FreeCAD.ActiveDocument.addObject('TechDraw::DrawViewDimension','Balloon001')
#    dim2.Type = "Radius"
#    dim2.MeasureType = "Projected"
#    dim2.References2D=[(view2, 'Edge0')]
#    rc = page.addView(dim2)

    FreeCAD.ActiveDocument.recompute()

    rc = False
    if ("Up-to-date" in balloon2.State) and ("Up-to-date" in balloon2.State):
        rc = True
    FreeCAD.closeDocument("TDBalloon")
    return rc

if __name__ == '__main__':
    DVBalloonTest()
