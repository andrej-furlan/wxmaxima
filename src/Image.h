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

/*! \file
  This file declares the class Image

  Image stores compressed images and handles scaling and uncompressing them.
*/

#ifndef IMAGE_H
#define IMAGE_H

#include "MathCell.h"

#include <wx/filesys.h>
#include <QByteArray>
#include <QImage>

/*! Manages an auto-scaling image

  This class keeps two versions of an image:
    - The image in its original compressed format. This way the image can losslessly
      be exported lateron
    - A bitmap version of the image that is scaled down to a size that makes sense 
      with the current viewport.

  Storing images this way has many advantages:
    - It allows us to restrict scaling operations to only once on actually drawing 
      the image after a resize of the wxMaxima window.
    - We can keep (and therefore export and save) jpeg photos in their original 
      format instead of using the .png compression that is superior for line art - 
      but not for depicting real-live images.
    - We save time on saving the .wxmx file since image compression, if done good, 
      might need a good deal of time and this class never needs to compress images
      itself.
    - It allows images to keep their metadata, if needed
    - and if we have big images (big plots or for example photographs) we don't need
      to store them in their uncompressed form.
    - One could even delete the cached scaled images for all cells that currently 
      are off-screen in order to save memory.
 */
class Image
{
public:
  //! A constructor that generates an empty image. See LoadImage()
  Image();
  //! A constructor that loads the compressed file.
  Image(const QByteArray &image, const QString &type);
  //! A constructor that loads an image and compresses it for internal storage.
  Image(const QImage &image);
  /*! A constructor that loads a compressed file.

    \param image The name of the file
    \param filesystem The filesystem to load it from
    \param remove true = Delete the file after loading it
   */
  Image(const wxString &image, bool remove = true, wxFileSystem *filesystem = nullptr);
  /*! Temporarily forget the scaled image in order to save memory.

    Will recreate the scaled image as soon as needed.
   */
  void ClearCache() { m_scaledImage = dummyImage(); }
  //! Reads the compressed image into a memory buffer
  QByteArray ReadCompressedImage(wxInputStream *data);
  //! Returns the file name extension of the current image
  QString GetExtension() const {return m_extension;}
  //! Loads an image from a file
  void LoadImage(const wxString &image, bool remove = true, wxFileSystem *filesystem = nullptr);
  //! "Loads" an image from an image
  void LoadImage(const QImage &image);
  //! Saves the image in its original form, or as .png if it originates in a bitmap
  QSize ToImageFile(const QString &filename);
  //! Returns the image being displayed
  QImage GetImage();
  //! Returns the image in its unscaled form
  QImage GetUnscaledImage() const;
  //! Returns a 1x1 filler image
  static QImage dummyImage();
  //! Needs to be called on changing the viewport size 
  void Recalculate();
  //! The width of the scaled image
  int m_width = 1;
  //! The height of the scaled image
  int m_height = 1;
  //! Returns the original image in its compressed form
  QByteArray GetCompressedImage() const {return m_compressedImage;}
  //! Returns the original width
  size_t GetOriginalWidth() const {return m_originalWidth;}
  //! Returns the original height
  size_t GetOriginalHeight() const {return m_originalHeight;}
  //! The image in its original compressed form
  QByteArray m_compressedImage;

protected:
  //! The width of the unscaled image
  int m_originalWidth = 1;
  //! The height of the unscaled image
  int m_originalHeight = 1;
  //! The bitmap, scaled down to the screen size
  QImage m_scaledImage = dummyImage();
  //! The file extension for the current image type
  QString m_extension;
};

#endif // IMAGE_H
