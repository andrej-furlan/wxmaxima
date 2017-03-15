// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2017 Kuba Ober <kuba@mareimbrium.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef UTILITIES_H
#define UTILITIES_H

#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/string.h>
#include <QCoreApplication>
#include <QImage>
#include <QString>

#define Q$ QStringLiteral
QString $$(const wxString &);
wxString $$(const QString &);
wxBitmap $bitmap(const QImage &);
wxImage $image(const QImage &);
QImage $image(const wxImage &);
QImage $image(const wxBitmap &);
wxSize $size(const QSize &);
QSize $size(const wxSize &);
inline wxPoint $point(const QPoint &p) { return {p.x(), p.y()}; }
inline QPoint $point(const wxPoint &p) { return {p.x, p.y}; }
inline wxColour $color(const QColor &c) { return {(uchar)c.red(), (uchar)c.green(), (uchar)c.blue(), (uchar)c.alpha()}; }
inline QColor $color(const wxColour &c) { return {c.Red(), c.Green(), c.Blue(), c.Alpha()}; }

inline QRect offsetIn(const QRect &rect, int o) { return rect.adjusted(o, o, -o, -o); }

void appendHex(QString &dst, const QByteArray &src);
void appendHex(wxString &dst, const QByteArray &src);

static auto trFor = [](const char *context){
  return [context](const char *source, const char *disambiguation = nullptr, int n = -1){
    return QCoreApplication::translate(context, source, disambiguation, n);
  };
};

#endif // UTILITIES_H
