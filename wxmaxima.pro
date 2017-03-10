# -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-

TEMPLATE = app
QT += widgets
CONFIG += c++14
qt:DEFINES += \
  QT_NO_KEYWORDS \
  Q_FORWARD_DECLARE_OBJC_CLASS=DECLARE_WXCOCOA_OBJC_CLASS # Workaround for clash with WxWidgets defs.h


macx {
  lessThan(QT_MINOR_VERSION, 8): QMAKE_MAC_SDK = macosx10.12
  # port install wxWidgets-3.0
  WX_CONFIG=/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.0/bin/wx-config
  QMAKE_CXXFLAGS += $$system($$WX_CONFIG --cxxflags)
  QMAKE_LFLAGS += $$system($$WX_CONFIG --libs)
  QMAKE_CXXFLAGS_WARN_ON += \
    -Wno-potentially-evaluated-expression \
    -Wno-ignored-qualifiers \
    -Wno-unused-parameter \
    -Wno-overloaded-virtual \
    -Wno-missing-field-initializers \
    -Wno-parentheses-equality \
    -Wno-unused-private-field
  LIBS += -lwx_osx_cocoau_aui-3.0

  ART_CONFIG.files = $$files(art/config/*.png)
  ART_CONFIG.path = Contents/Resources/config
  ART_STATUSBAR.files = $$files(art/statusbar/*.png)
  ART_STATUSBAR.path = Contents/Resources/statusbar
  ART_TOOLBAR.files = $$files(art/toolbar/*.png)
  ART_TOOLBAR.path = Contents/Resources/toolbar
  DATA.files = \
    data/wxmaxima.png
  DATA.path = Contents/Resources/data
  ROOT.files = \
    data/autocomplete.txt \
    data/tips.txt \
    data/wxmathml.lisp
  ROOT.path = Contents/Resources
  INFO.files = $$files(info/*.jpg) $$files(info/*.png) $$files(info/wxmaxima.h*)
  INFO.path = Contents/Resources/help
  QMAKE_BUNDLE_DATA += ART_CONFIG ART_STATUSBAR ART_TOOLBAR DATA ROOT INFO
  ICON = art/wxmac.icns
}

INCLUDEPATH += .

DISTFILES += \
    src/Resources.rc \
    data/wxmathml.lisp

HEADERS += \
    Setup.h \
    src/AbsCell.h \
    src/AtCell.h \
    src/Autocomplete.h \
    src/AutocompletePopup.h \
    src/BC2Wiz.h \
    src/Bitmap.h \
    src/BTextCtrl.h \
    src/ConfigDialogue.h \
    src/Configuration.h \
    src/ConjugateCell.h \
    src/ContentAssistantPopup.h \
    src/DiffCell.h \
    src/Dirstructure.h \
    src/EditorCell.h \
    src/EvaluationQueue.h \
    src/ExptCell.h \
    src/FindReplaceDialog.h \
    src/FindReplacePane.h \
    src/FracCell.h \
    src/FunCell.h \
    src/Gen1Wiz.h \
    src/Gen2Wiz.h \
    src/Gen3Wiz.h \
    src/Gen4Wiz.h \
    src/GroupCell.h \
    src/History.h \
    src/Image.h \
    src/ImgCell.h \
    src/IntCell.h \
    src/IntegrateWiz.h \
    src/LimitCell.h \
    src/LimitWiz.h \
    src/MarkDown.h \
    src/MathCell.h \
    src/MathCtrl.h \
    src/MathParser.h \
    src/MathPrintout.h \
    src/MatrCell.h \
    src/MatWiz.h \
    src/MyTipProvider.h \
    src/ParenCell.h \
    src/Plot2dWiz.h \
    src/Plot3dWiz.h \
    src/PlotFormatWiz.h \
    src/SeriesWiz.h \
    src/SlideShowCell.h \
    src/SqrtCell.h \
    src/StatusBar.h \
    src/SubCell.h \
    src/SubstituteWiz.h \
    src/SubSupCell.h \
    src/SumCell.h \
    src/SumWiz.h \
    src/SystemWiz.h \
    src/TableOfContents.h \
    src/TextCell.h \
    src/TextStyle.h \
    src/ToolBar.h \
    src/Utilities.h \
    src/wxMaxima.h \
    src/wxMaximaFrame.h \
    src/XmlInspector.h

SOURCES += \
    src/AbsCell.cpp \
    src/AtCell.cpp \
    src/Autocomplete.cpp \
    src/AutocompletePopup.cpp \
    src/BC2Wiz.cpp \
    src/Bitmap.cpp \
    src/BTextCtrl.cpp \
    src/ConfigDialogue.cpp \
    src/Configuration.cpp \
    src/ConjugateCell.cpp \
    src/ContentAssistantPopup.cpp \
    src/DiffCell.cpp \
    src/Dirstructure.cpp \
    src/EditorCell.cpp \
    src/EvaluationQueue.cpp \
    src/ExptCell.cpp \
    src/FindReplaceDialog.cpp \
    src/FindReplacePane.cpp \
    src/FracCell.cpp \
    src/FunCell.cpp \
    src/Gen1Wiz.cpp \
    src/Gen2Wiz.cpp \
    src/Gen3Wiz.cpp \
    src/Gen4Wiz.cpp \
    src/GroupCell.cpp \
    src/History.cpp \
    src/Image.cpp \
    src/ImgCell.cpp \
    src/IntCell.cpp \
    src/IntegrateWiz.cpp \
    src/LimitCell.cpp \
    src/LimitWiz.cpp \
    src/main.cpp \
    src/MarkDown.cpp \
    src/MathCell.cpp \
    src/MathCtrl.cpp \
    src/MathParser.cpp \
    src/MathPrintout.cpp \
    src/MatrCell.cpp \
    src/MatWiz.cpp \
    src/MyTipProvider.cpp \
    src/ParenCell.cpp \
    src/Plot2dWiz.cpp \
    src/Plot3dWiz.cpp \
    src/PlotFormatWiz.cpp \
    src/SeriesWiz.cpp \
    src/SlideShowCell.cpp \
    src/SqrtCell.cpp \
    src/StatusBar.cpp \
    src/SubCell.cpp \
    src/SubstituteWiz.cpp \
    src/SubSupCell.cpp \
    src/SumCell.cpp \
    src/SumWiz.cpp \
    src/SystemWiz.cpp \
    src/TableOfContents.cpp \
    src/TextCell.cpp \
    src/ToolBar.cpp \
    src/Utilities.cpp \
    src/wxMaxima.cpp \
    src/wxMaximaFrame.cpp \
    src/XmlInspector.cpp
