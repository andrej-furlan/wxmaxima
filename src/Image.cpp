// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2015-2016 Gunter Königsmann <wxMaxima@physikbuch.de>
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
  This file defines the class Image that stores compressed images and handles scaling and uncompressing them.
*/

#include "Image.h"
#include "Utilities.h"
#include <wx/mstream.h>
#include <wx/wfstream.h>
#include <QBuffer>
#include <QFileInfo>
#include <QMap>
#include <QPainter>
#include <QSaveFile>

#undef _
static auto _ = trFor("Image");

QByteArray Image::ReadCompressedImage(wxInputStream *data)
{
  QByteArray retval;
  char buf[8192];

  while(data->CanRead())
    {
      data->Read(buf, sizeof(buf));
      size_t siz;
      retval.append(buf, siz=data->LastRead());
    }
  return retval;
}

QImage Image::GetUnscaledImage() const
{
  return QImage::fromData(m_compressedImage);
}

Image::Image()
{}

Image::Image(const QByteArray &image, const QString &type)
{
  m_compressedImage = image;
  m_extension = type;
  m_originalWidth  = 640;
  m_originalHeight = 480;  

  if(!m_compressedImage.isEmpty())
    {
      auto image = QImage::fromData(m_compressedImage);
      m_originalWidth  = image.width();
      m_originalHeight = image.height();
    }
 }

Image::Image(const QImage &image)
{
  LoadImage(image);
}

// constructor which loads an image
Image::Image(const wxString &image, bool remove, wxFileSystem *filesystem)
{
  m_scaledImage = dummyImage();
  LoadImage(image,remove,filesystem);
}

QSize Image::ToImageFile(const QString &filename)
{
  QFileInfo fi(filename);
  auto ext = fi.suffix();

  if(filename.toLower().endsWith(GetExtension().toLower()))
  {
    QSaveFile file(filename);
    if (!file.open(QFile::WriteOnly))
      return QSize();
    file.write(m_compressedImage);
    if(file.commit())
      return {m_originalWidth, m_originalHeight};
    return {};
  }
  else
  {
    auto const image = GetUnscaledImage();
    static const QMap<QByteArray, const char *> formats = {
      {"jpeg", "JPG"}, {"jpg", "JPG"},
      {"bmp", "BMP"},
      {"png", "PNG"},
      {"xpm", "XPM"}
      // wx addtionally supported: pcx, tiff, ico, cur
    };
    auto format = std::find(formats.begin(), formats.end(), ext.toLower().toLatin1());
    if (format == formats.end())
      return {};
    if (!image.save(filename, *format))
      return {};
    return image.size();
  }
}

QImage Image::GetImage()
{
  Recalculate();

  // Let's see if we have cached the scaled bitmap with the right size
  if(m_scaledImage.width() == m_width)
    return m_scaledImage;

  Q_ASSERT(m_scaledImage.width() != m_width);

  QImage img;
  if(!m_compressedImage.isEmpty())
    img.loadFromData(m_compressedImage);

  if(!img.isNull())
    m_scaledImage = img;
  else
    {
      // Create a "image not loaded" bitmap.
      auto message = _("Error");
      auto error = Q$("%1\n%2").arg(message).arg(message);
      img = QImage(400, 250, QImage::Format_ARGB32_Premultiplied);
      img.fill(Qt::white);
      QPainter p(&img);
      p.drawRect(0, 0, 400, 250);
      p.drawLine(0, 0, 400, 250);
      p.drawLine(0, 250, 400, 0);
      p.drawText(img.rect(), error);
    }

  
  // Make sure we stay within sane defaults
  if(m_width<1)m_width = 1;
  if(m_height<1)m_height = 1;

  // Create a scaled bitmap and return it.
  m_scaledImage = img.scaled(m_width, m_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  return m_scaledImage;
}

void Image::LoadImage(const QImage &image)
{
  QBuffer buffer(&m_compressedImage);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");

  // Set the info about the image.
  m_extension = Q$("png");
  m_originalWidth  = image.width();
  m_originalHeight = image.height();
  m_scaledImage = dummyImage();
  m_width = 1;
  m_height = 1;
}

void Image::LoadImage(const wxString &image, bool remove, wxFileSystem *filesystem)
{
  m_compressedImage.clear();
  m_scaledImage = dummyImage();

  if (filesystem) {
    std::unique_ptr<wxFSFile> fsfile(filesystem->OpenFile(image));
    if (fsfile) { // open successful
      wxInputStream *istream = fsfile->GetStream();
      m_compressedImage = ReadCompressedImage(istream);
    }
    // Closing and deleting fsfile is important: if the file is left open,
    // opening .wxmx files containing hundreds of images might lead to a
    // "too many open files" error.
  }
  else {
    QFile file($$(image));
    if (file.open(QIODevice::ReadOnly)) {
      m_compressedImage = file.readAll();
      if (file.error() != QFile::NoError)
        m_compressedImage.clear();
      else if (remove)
        file.remove();
    }
  }

  QImage loaded;
  if(!m_compressedImage.isEmpty())
    loaded.loadFromData(m_compressedImage);
  
  m_extension = wxFileName(image).GetExt();

  if(!loaded.isNull())
    {
      m_originalWidth  = loaded.width();
      m_originalHeight = loaded.height();
    }
  else
    {
      // Leave space for an image showing an error message
      m_originalWidth  = 400;
      m_originalHeight = 250;
    }
  Recalculate();

}

void Image::Recalculate()
{
  int width  = m_originalWidth;
  int height = m_originalHeight;
  double scale;
  Configuration *configuration = Configuration::Get();

  // We want the image to get bigger if the user zooms in - and
  // if a high printing resolution requires us to scale everything up.
  scale = configuration->GetZoomFactor() * configuration->GetScale();

  // Ensure a minimum size for images.
  if(scale < 0.01) scale = 0.01;
  
  if((width < 1) || (height < 1))
    {
      m_width = 400;
      m_height = 250;
      return;
    }

  int viewPortHeight = configuration->GetClientHeight();
  int viewPortWidth = configuration->GetClientWidth();

  if(viewPortHeight < 10)
    viewPortHeight = 10;
  if(viewPortWidth < 10)
    viewPortWidth = 10;  
  
  // Shrink to .9* the canvas size, if needed
  if(scale * width > .9 * viewPortWidth)
  {
    scale = .9 * viewPortWidth / width;
  }
  if(scale * height > .9 * viewPortHeight)
  {
    if(scale > .9 * viewPortHeight / height)
      scale = .9 * viewPortHeight / height;
  }

  // Set the width of the scaled image
  m_height = (int) (scale * height);
  m_width  = (int) (scale * width);

  // Clear this cell's image cache if it doesn't contain an image of the size
  // we need right now.
  if(m_scaledImage.width() != m_width)
    ClearCache();
}

QImage Image::dummyImage()
{
  static QImage dummy;
  if (dummy.isNull())
  {
    dummy = QImage(1, 1, QImage::Format_ARGB32_Premultiplied);
    dummy.fill(Qt::white);
  }
  return dummy;
}
