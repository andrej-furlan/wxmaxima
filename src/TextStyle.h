// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//  Copyright (C) 2020 Kuba Ober <kuba@mareimbrium.org>
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

#ifndef TEXTSTYLE_H
#define TEXTSTYLE_H

/*! \file
 * This file declares everything needed for the text style system used
 * to style all the elements on the work sheet.
 */

#include <wx/colour.h>
#include <wx/config.h>
#include <wx/font.h>
#include <wx/settings.h>
#include <cstdint>
#include <functional>

/*! An interned font face name, very quick to compare and hash.
 *
 * Any given face name is only stored in memory once, and further comparisons and
 * hashing are based on the pointer to the underlying unique string.
 * There is no reference counting: the interned strings persist until the application
 * exits. This is acceptable, since there is a limited number of fonts available in
 * any system, and the size of the interned face name table will not grow without bounds.
 * The interning mechanism, however, does not have a fixed upper limit to the number of
 * interned strings.
 */
class AFontName final
{
  using AFN = AFontName;
public:
  AFontName() = default;
  /*! Constructs a font name by interning the name string
   * The constructor is explicit because this requires a string lookup in the intern
   * table. The font names should be constructed once for any given name, and then used
   * instead of the string.
   */
  explicit AFontName(const wxString &fontName) : m_fontName(Intern(fontName)) {}
  AFontName(const AFontName &o) : m_fontName(o.m_fontName) {}
  operator const wxString &() const { return GetAsString(); }
  bool operator==(const AFontName &o) const { return m_fontName == o.m_fontName; }
  bool operator<(const AFontName &o) const { return m_fontName < o.m_fontName; }
  bool empty() const { return !m_fontName || m_fontName->empty(); }
  AFontName &operator=(const AFontName &o)
  {
    m_fontName = o.m_fontName;
    return *this;
  }
  AFontName &Set(const wxString &str)
  {
    m_fontName = Intern(str);
    return *this;
  }
  const wxString& GetAsString() const { return m_fontName ? *m_fontName : *GetInternedEmpty(); }

  // All constant font names should be collected here, to avoid needless duplication and
  // lookups.
  static AFontName LinuxLibertineO() { static auto n = AFN(wxT("Linux Libertine O")); return n; }
  static AFontName LinuxLibertine()  { static auto n = AFN(wxT("Linux Libertine")); return n; }
  static AFontName Monaco()          { static auto n = AFN(wxT("Monaco")); return n; }
  static AFontName Arial()           { static auto n = AFN(wxT("Arial")); return n; }
  static AFontName CMEX10()          { static auto n = AFN(wxT("jsMath-cmex10")); return n; }
  static AFontName CMSY10()          { static auto n = AFN(wxT("jsMath-cmsy10")); return n; }
  static AFontName CMR10()           { static auto n = AFN(wxT("jsMath-cmr10")); return n; }
  static AFontName CMMI10()          { static auto n = AFN(wxT("jsMath-cmmi10")); return n; }
  static AFontName CMTI10()          { static auto n = AFN(wxT("jsMath-cmti10")); return n; }

private:
  friend class FontCache;
  friend struct std::hash<AFontName>;
  const wxString *m_fontName = {};

  static const wxString *Intern(const wxString &str);
  static const wxString *GetInternedEmpty();
};

template <> struct std::hash<AFontName> final
{
  size_t operator()(AFontName name) const
  {
    return std::hash<const void*>()(name.m_fontName);
  }
};

/*! A class that carries text styling information.
 *
 * It covers the characteristics of the font as well as other aspects of the style,
 * e.g. its descriptive name and color of the text.
 *
 * The text styles are also used as keys into the FontCache. They are designed to be quick
 * to compare for equality and order (less-than).
 */
class Style final
{
public:
  Style() = default;
  Style(const Style &);
  explicit Style(double fontSize) { m.fontSize = fontSize; }
//  explicit Style(const wxString &fontName) { m.fontName = AFontName(fontName); }

  Style &operator=(const Style &);
  bool operator==(const Style &o) const = delete;

  /*! Read this style from a config source.
   * Only touches the attributes that were successfully read. Remaining attributes
   * are unchanged.
   */
  Style &Read(wxConfigBase *config, const wxString &where);
  //! Write this style to a config source
  void Write(wxConfigBase *config, const wxString &where) const;

  //! Gets a style that represents a given font. The font gets cached.
  static const Style &FromFont(const wxFont &font);
  //! Gets a style that represents a stock font. The font is pre-cached.
  static const Style &FromStockFont(wxStockGDI::Item font);
  //! Gets a "factory" default text style for TS_DEFAULT
  static Style FactoryDefault();
  //! Gets a "factory" default math style (for Configuration::GetMathFont and GetMathFontSize)
  static Style FactoryMath();

  constexpr static wxFontFamily Default_Family = wxFONTFAMILY_DEFAULT;
  constexpr static wxFontEncoding Default_Encoding = wxFONTENCODING_DEFAULT;
  constexpr static wxFontWeight Default_Weight = wxFONTWEIGHT_NORMAL;
  constexpr static wxFontStyle Default_FontStyle = wxFONTSTYLE_NORMAL;
  constexpr static bool Default_Underlined = false;
  constexpr static bool Default_Strikethrough = false;
  static AFontName Default_FontName();
  constexpr static double Default_FontSize = 10.0;
  static inline const wxColor &Default_Color() { return *wxBLACK; }

  wxFontFamily GetFamily() const;
  wxFontEncoding GetEncoding() const;
  wxFontWeight GetWeight() const;
  bool IsBold() const { return GetWeight() == wxFONTWEIGHT_BOLD; }
  bool IsLight() const { return GetWeight() == wxFONTWEIGHT_LIGHT; }
  wxFontStyle GetFontStyle() const;
  bool IsItalic() const { return GetFontStyle() == wxFONTSTYLE_ITALIC; }
  bool IsSlant() const { return GetFontStyle() == wxFONTSTYLE_SLANT; }
  bool IsUnderlined() const;
  bool IsStrikethrough() const;
  AFontName GetFontName() const;
  const wxString &GetNameStr() const;
  double GetFontSize() const;
  uint32_t GetRGBColor() const;
  wxColor GetColor() const { return wxColor(GetRGBColor()); }

  using did_change = bool;
  did_change SetFamily(wxFontFamily family);
  did_change SetEncoding(wxFontEncoding encoding);
  did_change SetWeight(int weight);
  did_change SetBold(bool bold = true);
  did_change SetLight(bool light = true);
  did_change SetFontStyle(wxFontStyle style);
  did_change SetItalic(bool italic = true);
  did_change SetSlant(bool slant = true);
  did_change SetUnderlined(bool underlined = true);
  did_change SetStrikethrough(bool strikethrough = true);
  did_change SetFontName(AFontName fontName);
  did_change SetFontSize(double size);
  did_change SetRGBColor(uint32_t rgb);
  did_change SetColor(const wxColor &color);
  did_change SetColor(wxSystemColour sysColour);

  //! Resolves the style to the parameters of the font it represents
  did_change ResolveToFont();

  Style& Family(wxFontFamily family) { return SetFamily(family), *this; }
  Style& Encoding(wxFontEncoding encoding) { return SetEncoding(encoding), *this; }
  Style& Weight(int weight) { return SetWeight(weight), *this; }
  Style& FontStyle(wxFontStyle style) { return SetFontStyle(style), *this; }
  Style& Bold(bool bold = true) { return SetBold(bold), *this; }
  Style& Light(bool light = true) { return SetLight(light), *this; }
  Style& Italic(bool italic = true) { return SetItalic(italic), *this; }
  Style& Slant(bool slant = true) { return SetSlant(slant), *this; }
  Style& Underlined(bool underlined = true) { return SetUnderlined(underlined), *this; }
  Style& Strikethrough(bool strikethrough = true) { return SetStrikethrough(strikethrough), *this; }
  Style& FontName(class AFontName fontName) { return SetFontName(fontName), *this; }
  Style& FontSize(double size) { return SetFontSize(size), *this; }
  Style& RGBColor(uint32_t rgb) { return SetRGBColor(rgb), *this; }
  Style& Color(const wxColor &color) { return SetColor(color), *this; }
  Style& Color(uint8_t r, uint8_t g, uint8_t b) { return SetColor({r, g, b}), *this; }
  Style& Color(wxSystemColour sysColour) { return SetColor(sysColour), *this; }
  Style& ChangeLightness(int alpha) { return SetColor(GetColor().ChangeLightness(alpha)), *this; }

  wxFontInfo GetAsFontInfo() const;

  bool IsFontLessThan(const Style &o) const { return GetFontHash() < o.GetFontHash(); }
  bool IsFontEqualTo(const Style &) const;
  bool IsStyleEqualTo(const Style &o) const;

  bool IsFontOk() const;
  bool HasFontCached() const { return m.fontHash && m.font; }
  const wxFont& GetFont() const { return HasFontCached() ? *m.font : LookupFont(); }
  const wxFont& GetFontAt(double fontSize) const;

  //! Sets all font-related properties based on another font
  did_change SetFromFont(const wxFont&);
  //! Sets all font-related properties based on another style, including size, font style and weight
  did_change SetFontFrom(const Style&);
  //! Sets font-face-only properties based on another style
  did_change SetFontFaceFrom(const Style&);
  //! Sets font-face and size only properties based on another style (not attributes like bold, etc.)
  did_change SetFontFaceAndSizeFrom(const Style&);

  static bool IsFractionalFontSizeSupported();
  static double GetFontSize(const wxFont &);
  static void SetFontSize(wxFont &, double fontSize);

  wxString GetDump() const;

private:
  friend struct StyleFontHasher;
  friend class FontCache;
  Style &FromFontNoCache(const wxFont &);
  void SetFromFontNoCache(const wxFont &);

  struct Data // 48 bytes on 64-bit platforms
  {
    // 8-byte members
    double fontSize = Default_FontSize;
    // 8/4-byte members
    mutable const wxFont *font = nullptr;
    AFontName fontName = Default_FontName();
    mutable size_t fontHash = 0;
    // 4-byte members
    uint32_t rgbColor = Default_Color().GetRGB();
    // 2-byte members
    int16_t family = Default_Family;
    int16_t encoding = Default_Encoding;
    int16_t weight = Default_Weight;
    int16_t fontStyle = Default_FontStyle;
    // 1-byte members
    bool underlined : 1;
    bool strikethrough : 1;
    bool isNotOK : 1;

    Data() : underlined(false), strikethrough(false), isNotOK(false) {}
    static constexpr enum class NotOK_t {} NotOK = {};
    Data(NotOK_t) : underlined(false), strikethrough(false), isNotOK(true) {}
  } m;

  size_t GetFontHash() const;

  const wxFont& LookupFont() const;
  Style(Data::NotOK_t) : m(Data::NotOK) {}
};

//! Hash functor of the font size and attributes of the style
struct StyleFontHasher final
{
  size_t operator()(const Style &style) const { return style.GetFontHash(); }
};

//! Less-than-comparator of the font size and attributes of the style
struct StyleFontLess final
{
  bool operator()(const Style &l, const Style &r) const { return l.IsFontLessThan(r); }
};

//! Equals-comparator of the font size and attributes of the style
struct StyleFontEquals final
{
  bool operator()(const Style &l, const Style &r) const { return l.IsFontEqualTo(r); }
};

/*! All text styles known to wxMaxima
 *
 * \attention If this list is changed, the config dialogue
 * sometimes needs additional tweaking after that.
 */
enum TextStyle
{
  TS_DEFAULT             = 0,
  TS_VARIABLE            = 1,
  TS_NUMBER              = 2,
  TS_FUNCTION            = 3,
  TS_SPECIAL_CONSTANT    = 4,
  TS_GREEK_CONSTANT      = 5,
  TS_STRING              = 6,
  TS_INPUT               = 7,
  TS_MAIN_PROMPT         = 8,
  TS_OTHER_PROMPT        = 9,
  TS_LABEL               = 10,
  TS_USERLABEL           = 11,
  TS_HIGHLIGHT           = 12,
  TS_WARNING             = 13,
  TS_ERROR               = 14,
  TS_TEXT                = 15,
  TS_HEADING6            = 16,
  TS_HEADING5            = 17,
  TS_SUBSUBSECTION       = 18,
  TS_SUBSECTION          = 19,
  TS_SECTION             = 20,
  TS_TITLE               = 21,
  TS_TEXT_BACKGROUND     = 22,
  TS_DOCUMENT_BACKGROUND = 23,
  TS_CELL_BRACKET        = 24,
  TS_ACTIVE_CELL_BRACKET = 25,
  TS_CURSOR              = 26,
  TS_SELECTION           = 27,
  TS_EQUALSSELECTION     = 28,
  TS_OUTDATED            = 29,
  TS_CODE_VARIABLE       = 30,
  TS_CODE_FUNCTION       = 31,
  TS_CODE_COMMENT        = 32,
  TS_CODE_NUMBER         = 33,
  TS_CODE_STRING         = 34,
  TS_CODE_OPERATOR       = 35,
  TS_CODE_LISP           = 36,
  TS_CODE_ENDOFLINE      = 37,
  TS_MATH                = 38,
  NUMBEROFSTYLES //!< This is not a style, but its value tells us how many styles are defined
};

#endif // TEXTSTYLE_H
