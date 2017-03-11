// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2014-2016 Gunter Königsmann <wxMaxima@physikbuch.de>
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

/*! \file
  This file defines the class ImgCell

  ImgCell is the MathCell type that represents still images in maxima's output
  or in user-provided images.
 */

#include "ImgCell.h"
#include "Utilities.h"

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/clipbrd.h>
#include <wx/mstream.h>
#include <QBuffer>
#include <QClipboard>
#include <QGuiApplication>
#include <QPainter>

#undef _
static auto _ = trFor("ImgCell");

ImgCell::ImgCell() : MathCell()
{
  m_type = MC_TYPE_IMAGE;
  m_imageBorderWidth = 1;
}

ImgCell::ImgCell(const wxMemoryBuffer &image, wxString type) : ImgCell()
{
  m_image.reset(new Image(QByteArray::fromRawData((const char *)image.GetData(), image.GetDataLen()), $$(type)));
}

ImgCell::ImgCell(const QImage &image) : ImgCell()
{
  m_image.reset(new Image(image));
}

int ImgCell::s_counter = 0;

ImgCell::ImgCell(wxString image, bool remove, wxFileSystem *filesystem) : MathCell()
{
  if(image != wxEmptyString)
    m_image.reset(new Image(image,remove,filesystem));
  else
    m_image.reset(new Image);
}

void ImgCell::SetImage(const QImage &image)
{
  m_width = m_height = -1;
  m_image.reset(new Image(image));
}

MathCell* ImgCell::Copy()
{
  ImgCell* tmp = new ImgCell;
  CopyData(this, tmp);
  tmp->m_drawRectangle = m_drawRectangle;

  Image *img = new Image();
  *img = *m_image;
  tmp->m_image.reset(img);
  
  return tmp;
}

void ImgCell::RecalculateWidths(int fontsize)
{
  // Here we recalculate the height, as well:
  //  - This doesn't cost much time and
  //  - as image cell's sizes might change when the resolution does
  //    we might have intermittent calculation issues otherwise
  m_image-> Recalculate();
  
  m_width  = m_image->m_width  + 2 * m_imageBorderWidth;
  m_height = m_image->m_height + 2 * m_imageBorderWidth;
  m_center = m_height / 2;
}

void ImgCell::RecalculateHeight(int fontsize)
{
  // Here we recalculate the width, as well:
  //  - This doesn't cost much time and
  //  - as image cell's sizes might change when the resolution does
  //    we might have intermittent calculation issues otherwise
  RecalculateWidths(fontsize);
}

static bool drawQt = true;

void ImgCell::Draw(wxPoint point, int fontsize)
{
  MathCell::Draw(point, fontsize);
  if (drawQt) return;
  m_image->Recalculate();

  // TODO: Enable this when unselecting text updates the right region.
  //if (!InUpdateRegion()) return;

  Configuration *configuration = Configuration::Get();
  wxDC& dc = configuration->GetDC();
  if (DrawThisCell(point) && (m_image != NULL))
  {
    wxMemoryDC bitmapDC;

    if(m_drawBoundingBox)
      dc.SetBrush( *(wxTheBrushList->FindOrCreateBrush(configuration->GetColor(TS_SELECTION))));
    else
      SetPen();
    
    if (m_drawRectangle || m_drawBoundingBox)
      dc.DrawRectangle(wxRect(point.x, point.y - m_center, m_width, m_height));
    
    auto bitmap = $bitmap(m_image->GetImage());
    bitmapDC.SelectObject(bitmap);

    if ((m_drawBoundingBox == false) || (m_imageBorderWidth > 0))
      dc.Blit(point.x + m_imageBorderWidth, point.y - m_center + m_imageBorderWidth, m_width - 2 * m_imageBorderWidth, m_height - 2 * m_imageBorderWidth, &bitmapDC, 0, 0);
    else
      dc.StretchBlit(point.x + 5, point.y - m_center + 5, m_width - 2 * 5, m_height - 2 * 5, &bitmapDC, 0, 0, m_width, m_height);
  }
  else
    // The cell isn't drawn => No need to keep it's image cache for now.
    ClearCache();
  
  // The next time we need to draw a bounding box we will be informed again.
  m_drawBoundingBox = false;
}

QImage ImgCell::DrawQt(const QPoint &point, int)
{
  if (!drawQt) return {};
  QImage drawn;
  m_image->Recalculate();

  // TODO: Enable this when unselecting text updates the right region.
  //if (!InUpdateRegion()) return;

  Configuration *configuration = Configuration::Get();
  if (DrawThisCell($point(point)) && m_image)
  {
    drawn = {m_width, m_height, QImage::Format_ARGB32_Premultiplied};
    QPainter dc(&drawn);

    if (m_drawBoundingBox)
      dc.setBrush(configuration->GetQColor(TS_SELECTION));
    else
    {
      auto setPen = GetSetPen();
      dc.setPen(QPen($color(setPen.color), setPen.lineWidth));
    }

    if (m_drawRectangle || m_drawBoundingBox)
      dc.drawRect(0, 0, m_width, m_height);

    if ((m_drawBoundingBox == false) || (m_imageBorderWidth > 0))\
    {
      QRect rect(m_imageBorderWidth, m_imageBorderWidth,
                 m_width - 2 * m_imageBorderWidth, m_height - 2 * m_imageBorderWidth);
      dc.drawImage(rect, m_image->GetImage(), rect);
    }
    else
      dc.drawImage(QRect{5, 5, m_width - 2 * 5, m_height - 2 * 5}, m_image->GetImage(), {0, 0, m_width, m_height});
  }
  else
    // The cell isn't drawn => No need to keep it's image cache for now.
    ClearCache();
  
  // The next time we need to draw a bounding box we will be informed again.
  m_drawBoundingBox = false;
  return drawn;
}

wxString ImgCell::ToString()
{
  return $$(_(" (Graphics) "));
}

wxString ImgCell::ToTeX()
{
  return $$(_(" (Graphics) "));
}

QSize ImgCell::ToImageFile(const QString &file)
{
  return m_image->ToImageFile(file);
}

wxString ImgCell::ToRTF()
{
  // Lines that are common to all types of images
  auto header=Q$("{\\pict");
  auto footer=Q$("}\n");
  
  // Extract the description of the image data
  QString image;
  QByteArray imgdata;
  if(m_image->GetExtension().toLower() == Q$("png"))
  {
    imgdata = GetCompressedImage();
    image = Q$("\\pngblip");
  } else if(
    (m_image->GetExtension().toLower() == Q$("jpg"))||
    (m_image->GetExtension().toLower() == Q$("jpeg"))
    )
  {
    imgdata = GetCompressedImage();
    image = Q$("\\jpegblip");
  }
    else
    {
      // Convert any non-rtf-enabled format to .png before adding it to the .rtf file.
      image = Q$("\\pngblip");
      auto imagedata = m_image->GetUnscaledImage();
      QBuffer buf(&imgdata);
      buf.open(QBuffer::WriteOnly | QBuffer::Append);
      imagedata.save(&buf, "png");
    }

  image += Q$("\\picw%1\\pich%2 ")
      .arg(m_image->GetOriginalWidth())
      .arg(m_image->GetOriginalHeight());
  
  // Convert the data into a hexadecimal string
  appendHex(image, imgdata);

  return $$(header+image+footer);
}

wxString ImgCell::ToXML()
{
  auto basename = ImgCell::WXMXGetNewFileName();

  // add the file to memory
  if(m_image && !m_image->GetCompressedImage().isEmpty())
      wxMemoryFSHandler::AddFile($$(basename + m_image->GetExtension()),
                                 m_image->GetCompressedImage().data(),
                                 m_image->GetCompressedImage().length()
        );
  return $$((m_drawRectangle ? Q$("<img>") : Q$("<img rect=\"false\">")) +
    basename + m_image->GetExtension() + Q$("</img>"));
}

QString ImgCell::WXMXGetNewFileName()
{
  return Q$("image%1.").arg(++s_counter);
}

bool ImgCell::CopyToClipboard()
{
  auto clipboard = QGuiApplication::clipboard();
  if (clipboard) {
    clipboard->setImage(m_image->GetUnscaledImage());
    return true;
  }
  return false;
}
