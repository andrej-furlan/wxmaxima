// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2014-2015 Gunter Königsmann <wxMaxima@physikbuch.de>
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

#ifndef IMGCELL_H
#define IMGCELL_H

#include <memory>

#include "MathCell.h"
#include <wx/image.h>
#include "Image.h"

#include <wx/filesys.h>
#include <wx/fs_arc.h>

class ImgCell : public MathCell
{
public:
  ImgCell();
  ImgCell(const wxMemoryBuffer &image, wxString type);
  ImgCell(wxString image, bool remove = true, wxFileSystem *filesystem = NULL);
  ImgCell(const QImage &image);
  MathCell* Copy() override;
  void SelectInner(wxRect& rect, MathCell** first, MathCell** last) override
  {
    *first = *last = this;
  }
  friend class SlideShow;
  /*! Writes the image to a file

    The image file that is written is either a bit-per-bit copy of the original
    file loaded into the ImgCell - or in the case that there is no original file
    a losslessly compressed png version of the bitmap.

    See also GetExtension().
   */
  QSize ToImageFile(const QString &filename);
  /*! Removes the cached scaled image from memory

    The scaled version of the image will be recreated automatically once it is 
    needed.
   */
  virtual void ClearCache() override { if (m_image) m_image->ClearCache(); }
  //! Sets the bitmap that is shown
  void SetImage(const QImage &imagep);
  //! Copies the cell to the system's clipboard
  bool CopyToClipboard() override;
  // These methods should only be used for saving wxmx files
  // and are shared with SlideShowCell.
  static void WXMXResetCounter() { s_counter = 0; }
  static QString WXMXGetNewFileName();
  static int WXMXImageCount() { return s_counter; }
  void DrawRectangle(bool draw) { m_drawRectangle = draw; }
  //! Returns the file name extension that matches the image type
  QString GetExtension() const { return m_image ? m_image->GetExtension() : QString(); }
  //! Returns the original compressed version of the image
  QByteArray GetCompressedImage() const {return m_image->m_compressedImage;}
protected:
  std::unique_ptr<Image> m_image;
  void RecalculateHeight(int fontsize) override;
  void RecalculateWidths(int fontsize) override;
  void Draw(wxPoint point, int fontsize) override;
  wxString ToString() override;
  wxString ToRTF() override;
  wxString ToTeX() override;
  wxString ToXML() override;
	static int s_counter;
  bool m_drawRectangle = true;
  void DrawBoundingBox(wxDC& dc, bool all = false) override
    {
      m_drawBoundingBox = true;
    }
private:
  bool m_drawBoundingBox = false;
};

#endif // IMGCELL_H
