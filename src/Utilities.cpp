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

/*!\file
  This file contains utilities used in the Qt porting. It should be removed once the
  port is finished.
 */

#include "Utilities.h"

QString $$(const wxString & src) {
  return QString::fromStdWString(src.ToStdWstring());
}

wxString $$(const QString & src) {
  return wxString(src.toStdWString());
}

wxBitmap $bitmap(const QImage & src) {
  return {$image(src)};
}

wxImage $image(const QImage & src) {
  auto const image = src.convertToFormat(QImage::Format_RGB888);
  auto const bytesPerLine = 3 * src.width(); // not src.bytesPerLine() due to padding
  auto const data = reinterpret_cast<uchar*>(malloc(bytesPerLine * src.height()));
  auto ptr = data;
  for (int y = 0; y < src.height(); ++y) {;
    memcpy(ptr, image.scanLine(y), bytesPerLine);
    ptr += bytesPerLine;
  }
  return {src.width(), src.height(), data};
}

QImage $image(const wxImage & src) {
  QImage image(src.GetWidth(), src.GetHeight(), QImage::Format_RGB888);
  auto const bytesPerLine = 3 * src.GetWidth();
  auto ptr = src.GetData();
  for (int y = 0; y < image.height(); ++y) {
    memcpy(image.scanLine(y), ptr, bytesPerLine);
    ptr += bytesPerLine;
  }
  return image;
}

QImage $image(const wxBitmap & src) {
  return $image(src.ConvertToImage());
}

wxSize $size(const QSize & src) {
  if (!src.isNull())
    return {src.width(), src.height()};
  return {-1, -1};
}

QSize $size(const wxSize & src) {
  if (src.x >= 0 && src.y >= 0)
    return {src.x, src.y};
  return {};
}

void appendHex(wxString & dst, const QByteArray & src) {
  dst.reserve(dst.size() + 2*src.size());
  for (auto c : src)
    dst += wxString::Format("%02x", (uchar)c);
}

void appendHex(QString & dst, const QByteArray & src) {
  dst.append(QString::fromLatin1(src.toHex()));
}
