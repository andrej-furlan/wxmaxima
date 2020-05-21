// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2014-2018 Gunter Königsmann <wxMaxima@physikbuch.de>
//            (C) 2020 Kuba Ober <kuba@mareimbrium.org>
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
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
//  SPDX-License-Identifier: GPL-2.0+

/*!
 * \file This file implements the text style system.
 */

#define wxNO_UNSAFE_WXSTRING_CONV
#include "TextStyle.h"
#include "FontCache.h"
#include <wx/colour.h>
#include <wx/hashmap.h>
#include <wx/thread.h>
#include <wx/translation.h>
#include <array>
#include <list>
#include <vector>

/*! \brief Mixes two hashes together.
 *
 * Used to obtain hashes of composite data types, such as structures.
*/
template <typename T>
static size_t MixHash(size_t seed, const T &value)
{
  std::hash<T> const hasher;
  seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  return seed;
}

//! Comparator for wxString*
static bool operator<(const wxString *l, const wxString &r) { return *l < r; }

/*! \brief An implementation of string interning arena.
 */
template <typename T>
class Interner
{
  struct Slice {
    size_t nextIndex = 0;
    std::array<T, 128> data;
    T* GetNext() {
      if (nextIndex >= data.size()) return nullptr;
      return &(data[nextIndex++]);
    }
  };

  std::list<Slice> m_storage;
  std::vector<const T*> m_index;
public:
  bool IsInterned(const T *value) const
  {
    for (auto const &slice : m_storage)
    {
      auto const &data = slice.data;
      auto const nextIndex = slice.nextIndex;
      if (nextIndex)
      {
        auto *const first = &(data[0]);
        auto *const last = &(data[nextIndex-1]);
        if (value >= first && value <= last)
        {
          wxLogDebug("IsInterned %p", value);
          return true;
        }
      }
    }
    return false;
  }
  const T* Intern(const T *value) {
    if (!value) return nullptr;
    return &Intern(*value);
  }
  const T* Intern(const T &value)
  {
    // pointer equality: is the value within one of the slices?
    if (IsInterned(&value))
      return &value;
    // value equality: is the value equal to one already interned?
    auto indexPos = std::lower_bound(m_index.begin(), m_index.end(), value);
    if (indexPos != m_index.end() && **indexPos == value)
      return *indexPos;
    // none of the above: we must intern
    if (m_storage.empty()) m_storage.emplace_back();
    T *loc = m_storage.back().GetNext();
    if (!loc)
    {
      m_storage.emplace_back();
      loc = m_storage.back().GetNext();
    }
    *loc = value;
    indexPos = m_index.insert(indexPos, loc);
    wxASSERT(*indexPos && *indexPos == loc);
    return *indexPos;
  }
};

/*! \brief Syncrhonizes access to the font face string interner.
 *
 * This is only active on Windows. The class is empty on non-windows systems, where
 * access to the font objects is only allowed from the main thread.
 */
struct InternerLock
{
#ifdef __WINDOWS__
  // Windows allows font access from multiple threads, as long as each font
  // is built separately. We must thus synchronize the access to the interner.
  static wxMutex mutex;
  InternerLock() { mutex.Lock(); }
  ~InternerLock() { mutex.Unlock(); }
  InternerLock(const InternerLock &) = delete;
  void operator=(const InternerLock &) = delete;
#endif
};

#ifdef __WINDOWS__
//! The mutex is only for Windows, where fonts can be accessed from multiple threads.
//! On other systems, this is not allowed.
wxMutex InternerLock::mutex;
#endif

static Interner<wxString>& GetFontNameInterner()
{
  static Interner<wxString> interner;
  return interner;
}

const wxString *AFontName::Intern(const wxString &str)
{
  // cppcheck-suppress unusedVariable
  InternerLock lock;
  return GetFontNameInterner().Intern(str);
}

const wxString *AFontName::GetInternedEmpty()
{
  static const wxString *internedEmpty = AFontName::Intern(wxEmptyString);
  return internedEmpty;
}

/*
 * Style
 */

Style::Style(const Style &o) : m(o.m) {}

Style &Style::operator=(const Style &o)
{
  if (&o != this)
    m = o.m;

  return *this;
}


wxFontFamily Style::GetFamily() const
{ return m.family; }

wxFontEncoding Style::GetEncoding() const
{ return m.encoding; }

wxFontWeight Style::GetWeight() const
{ return wxFontWeight(m.weight); }

wxFontStyle Style::GetFontStyle() const
{ return m.fontStyle; }

bool Style::IsUnderlined() const
{ return m.underlined; }

bool Style::IsStrikethrough() const
{ return m.strikethrough; }

AFontName Style::GetFontName() const
{ return m.fontName; }

double Style::GetFontSize() const
{
auto fontSize = m.fontSize;
  wxASSERT(fontSize > 0);
  return fontSize;
}

const wxColor &Style::GetColor() const
{ return m.color; }

using did_change = Style::did_change;

did_change Style::SetFamily(wxFontFamily family)
{
  if (m.family == family) return false;
  m.family = family;
  m.attributeHash = 0;
  return true;
}

did_change Style::SetEncoding(wxFontEncoding encoding)
{
  if (m.encoding == encoding) return false;
  m.encoding = encoding;
  m.attributeHash = 0;
  return true;
}

did_change Style::SetFontStyle(wxFontStyle fontStyle)
{
  if (m.fontStyle == fontStyle) return false;
  m.fontStyle = fontStyle;
  m.attributeHash = 0;
  return true;
}

did_change Style::SetWeight(int weight)
{
  if (m.weight == weight) return false;
  m.weight = weight;
  m.attributeHash = 0;
  return true;
}

did_change Style::SetBold(bool bold)
{
  return SetWeight(bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
}

did_change Style::SetLight(bool light)
{
  return SetWeight(light ? wxFONTWEIGHT_LIGHT : wxFONTWEIGHT_NORMAL);
}

did_change Style::SetItalic(bool italic)
{
  return SetFontStyle(italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL);
}

did_change Style::SetSlant(bool slant)
{
  return SetFontStyle(slant ? wxFONTSTYLE_SLANT : wxFONTSTYLE_NORMAL);
}

did_change Style::SetUnderlined(bool underlined)
{
  if (m.underlined == underlined) return false;
  m.underlined = underlined;
  m.attributeHash = 0;
  return true;
}

did_change Style::SetStrikethrough(bool strikethrough)
{
  if (m.strikethrough == strikethrough) return false;
  m.strikethrough = strikethrough;
  m.attributeHash = 0;
  return true;
}

did_change Style::SetFontName(AFontName faceName)
{
  if (m.fontName == faceName) return false;
  m.fontName = faceName;
  m.attributeHash = 0;
  return true;
}

// cppcheck-suppress unusedFunction
did_change Style::SetFontNameFromFont()
{
  return SetFontName(AFontName(GetFont().GetFaceName()));
}

did_change Style::SetFontSize(double size)
{
  if (m.fontSize == size) return false;
  m.fontSize = size;
  m.attributeHash = m.fontHash = 0;
  return true;
}

did_change Style::SetColor(const wxColor &color)
{
  if (color == m.color) return false;
  m.color = color;
  return true;
}

did_change Style::SetColor(wxSystemColour sysColour)
{
  return SetColor(wxSystemSettings::GetColour(sysColour));
}

size_t Style::GetAttributeHash() const
{
  size_t hash_ = m.attributeHash;
  if (!hash_)
  {
    hash_ = MixHash(hash_, m.family);
    hash_ = MixHash(hash_, m.encoding);
    hash_ = MixHash(hash_, m.weight);
    hash_ = MixHash(hash_, m.fontStyle);
    hash_ = MixHash(hash_, m.underlined << 1 | m.strikethrough << 3 | m.isNotOK << 5);
    hash_ = MixHash(hash_, m.fontName);
    if (!hash_) hash_++;
    m.attributeHash = hash_;
  }
  return hash_;
}

size_t Style::GetSizeHash() const
{
  size_t hash_ = 0;
  hash_ = MixHash(hash_, m.fontSize);
  if (!hash_) hash_++;
  return hash_;
}

size_t Style::GetFontHash() const
{
  size_t hash_ = m.fontHash;
  if (!hash_)
  {
    hash_ = MixHash(GetAttributeHash(), GetSizeHash());
    if (!hash_) hash_++;
    m.fontHash = hash_;
  }
  return hash_;
}

bool Style::IsFontEqualTo(const Style &o_) const
{
  const Data &o = o_.m;
  if (m.font && m.font == o.font) return true;
  return
    (!m.fontHash || !o.fontHash || m.fontHash == o.fontHash) &&
    (!m.attributeHash || !o.attributeHash || m.attributeHash == o.attributeHash) &&
    m.fontSize == o.fontSize &&
    m.family == o.family &&
    m.encoding == o.encoding &&
    m.weight == o.weight &&
    m.fontStyle == o.fontStyle &&
    m.fontName == o.fontName &&
    m.underlined == o.underlined &&
    m.strikethrough == o.strikethrough &&
    m.isNotOK == o.isNotOK;
}

bool Style::IsStyleEqualTo(const Style &o) const
{
  return
    this->IsFontEqualTo(o) &&
    m.color == o.m.color;
}

const wxFont& Style::LookupFont() const
{
  GetFontHash();
  m.font = &FontCache::GetAFont(*this);
  wxASSERT(m.font);
  wxASSERT(!GetFontName().empty());
  return *m.font;
}

bool Style::IsFontOk() const
{
  return m.isNotOK ? false : GetFont().IsOk();
}

const wxFont& Style::GetFontAt(double fontSize) const
{
  if (fontSize == GetFontSize()) return GetFont();
  auto prevFontHash = m.fontHash;
  double prevFontSize = m.fontSize;
  m.fontHash = 0;
  m.fontSize = fontSize;
  auto &font = LookupFont();
  m.fontHash = prevFontHash;
  m.fontSize = prevFontSize;
  return font;
}

void Style::SetFromFont(const wxFont &font)
{
  wxASSERT_MSG(
    &font != wxITALIC_FONT
      && &font != wxNORMAL_FONT
      && &font != wxSMALL_FONT
      && &font != wxSWISS_FONT, "Use Style::FromStockFont to get stock fonts!");

  *this = FontCache::AddAFont(font);
}

Style Style::FromFontNoCache(const wxFont &font)
{
  Style style;
  style.SetFromFontNoCache(font);
  return style;
}

void Style::SetFromFontNoCache(const wxFont &font)
{
  if (font.IsOk())
  {
    m.color = GetColor();
    m.attributeHash = m.fontHash = 0;
    m.encoding = font.GetEncoding();
    m.family = font.GetFamily();
    m.fontStyle = font.GetStyle();
    m.underlined = font.GetUnderlined();
    m.strikethrough = font.GetStrikethrough();
#if wxCHECK_VERSION(3,1,2)
    m.weight = font.GetNumericWeight();
#else
    m.weight = font.GetWeight();
#endif
    m.fontName = AFontName(font.GetFaceName());
    m.fontSize = GetFontSize(font);
  }
  else
    m = Data::NotOK;
}

bool Style::IsFractionalFontSizeSupported()
{
  return wxCHECK_VERSION(3,1,2);
}

double Style::GetFontSize(const wxFont &font)
{
#if wxCHECK_VERSION(3,1,2)
  return font.GetFractionalPointSize();
#endif
  return font.GetPointSize();
}

void Style::SetFontSize(wxFont &font, double fontSize)
{
#if wxCHECK_VERSION(3,1,2)
  return font.SetFractionalPointSize(fontSize);
#endif
  return font.SetPointSize(fontSize);
}

wxFontInfo Style::GetAsFontInfo() const
{
  wxFontInfo result(GetFontSize());

  result
    .Family(GetFamily())
    .FaceName(GetFontName())
    .Underlined(IsUnderlined())
    .Strikethrough(IsStrikethrough())
    .Encoding(GetEncoding())
    ;

  // This pattern is used to ensure that the legacy variant
  // still compiles (doesn't bitrot).
#if wxCHECK_VERSION(3,1,2)
  return result
    .Style(GetFontStyle())
    .Weight(GetWeight())
    ;
#endif
  return result
    .Slant(IsSlant())
    .Italic(IsItalic())
    .Bold(IsBold())
    .Light(IsLight())
    ;
}

AFontName Style::Default_FontName()
{
#if defined(__WXOSX_MAC__)
  static auto fontName = AFontName(wxT("Monaco"));
#elif defined(__WINDOWS__)
  static auto fontName = AFontName(wxT("Linux Libertine O"));
#else
  static auto fontName = AFontName(wxT("Arial"));
#endif
  return fontName;
}

static const wxString k_color = wxT("%s/color");
static const wxString k_bold = wxT("%s/bold");
static const wxString k_light = wxT("%s/light");
static const wxString k_italic = wxT("%s/italic");
static const wxString k_slant = wxT("%s/slant");
static const wxString k_underlined = wxT("%s/underlined");
static const wxString k_strikethrough = wxT("%s/strikethrough");
static const wxString k_fontsize = wxT("%s/Style/Text/fontsize");
static const wxString k_fontname = wxT("%s/Style/Text/fontname");

void Style::Read(wxConfigBase *config, const wxString &where)
{
  wxString tmpStr;
  bool tmpBool;
  long tmpLong;

  // Unset all fields
  m = {};

  if (config->Read(wxString::Format(k_color, where), &tmpStr))
  {
    wxColor color = wxColor(tmpStr);
    if (color.IsOk()) SetColor(color);
  }
  if (config->Read(wxString::Format(k_bold, where), &tmpBool) && tmpBool) SetBold(true);
  else if (config->Read(wxString::Format(k_light, where), &tmpBool) && tmpBool) SetLight(true);
  if (config->Read(wxString::Format(k_italic, where), &tmpBool) && tmpBool) SetItalic(true);
  else if (config->Read(wxString::Format(k_slant, where), &tmpBool) && tmpBool) SetSlant(true);
  if (config->Read(wxString::Format(k_underlined, where), &tmpBool)) SetUnderlined(tmpBool);
  if (config->Read(wxString::Format(k_strikethrough, where), &tmpBool)) SetStrikethrough(tmpBool);
  if (config->Read(wxString::Format(k_fontsize, where), &tmpLong))
    SetFontSize(tmpLong);
  if (config->Read(wxString::Format(k_fontname, where), &tmpStr))
    SetFontName(AFontName(tmpStr));

  // Validation is deferred to the point of first use, etc.
}

void Style::Write(wxConfigBase *config, const wxString &where) const
{
  config->Write(wxString::Format(k_color, where), GetColor().GetAsString());
  config->Write(wxString::Format(k_bold, where), IsBold());
  config->Write(wxString::Format(k_italic, where), IsItalic());
  config->Write(wxString::Format(k_underlined, where), IsUnderlined());
  config->Write(wxString::Format(k_fontsize, where), long(GetFontSize()));
  config->Write(wxString::Format(k_fontname, where), GetFontName().GetAsString());

  // We don't write the slant, light nor strikethrough attributes so as not to grow the
  // configuration compared to the previous releases. The slant and strikethrough are only
  // emitted when set or when previously set.
  auto const optWrite = [](wxConfigBase *config, const wxString &keyFormat,
                           const wxString &where, auto condition)
  {
    decltype(condition) tempVal;
    auto path = wxString::Format(keyFormat, where);
    if (condition != decltype(condition){} || config->Read(path, &tempVal))
      config->Write(path, condition);
  };

  optWrite(config, k_slant, where, IsSlant());
  optWrite(config, k_light, where, IsLight());
  optWrite(config, k_strikethrough, where, IsStrikethrough());
}

const Style &Style::FromStockFont(wxStockGDI::Item font)
{
  static auto getStyleFor = [](const wxFont *font){
    Style style = Style::FromFontNoCache(*font);
    style.GetFontHash();
    style.m.font = font; // Pre-cache the stock font in the style itself
    return style;
  };

  switch (font)
  {
  case wxStockGDI::FONT_ITALIC:
  {
    static Style italic = getStyleFor(wxITALIC_FONT);
    return italic;
  }
  case wxStockGDI::FONT_NORMAL:
  {
    static Style normal = getStyleFor(wxNORMAL_FONT);
    return normal;
  }
  case wxStockGDI::FONT_SMALL:
  {
    static Style small = getStyleFor(wxSMALL_FONT);
    return small;
  }
  case wxStockGDI::FONT_SWISS:
  {
    static Style swiss = getStyleFor(wxSWISS_FONT);
    return swiss;
  }
  default:
  {
    static Style defaultStyle;
    return defaultStyle;
  }
  }
}
