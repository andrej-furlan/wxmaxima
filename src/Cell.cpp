// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2014-2018 Gunter Königsmann <wxMaxima@physikbuch.de>
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

/*! \file
  This file defines the class Cell

  Cell is the base class for all cell- or list-type elements.
*/

#include "Cell.h"
#include <wx/regex.h>
#include <wx/sstream.h>

CellPtr::ControlBlock CellPtr::emptyCB{nullptr};

const wxString &Cell::GetToolTip(const wxPoint &point)
{
  static wxString empty;
  if (!ContainsPoint(point))
    return empty;

  for (auto &inner : OnInnerCells())
    for (auto &cell : inner.OnCellList())
    {
      const wxString &toolTip = cell.GetToolTip(point);
      if (!toolTip.empty())
        return toolTip;
    }

  return m_toolTip;
}

Cell::Cell(Cell *group, Configuration **config) :
    m_group(group),
    m_parent(group),
    m_configuration(config),
    m_toolTip((*m_configuration)->GetDefaultCellToolTip()),
    m_fontSize((*m_configuration)->GetMathFontSize())
{}

Cell::~Cell()
{
  // Delete this list of cells without using a recursive function call that can
  // run us out of stack space

  OwningCellPtr next = std::move(this->m_next);
  while (next)
  {
    OwningCellPtr cell = std::move(next);
    next = std::move(cell->m_next);
  }
}

void Cell::SetType(CellType type)
{
  m_type = type;

  switch (m_type)
  {
    case MC_TYPE_MAIN_PROMPT:
      m_textStyle = TS_MAIN_PROMPT;
      break;
    case MC_TYPE_PROMPT:
      m_textStyle = TS_OTHER_PROMPT;
      break;
    case MC_TYPE_LABEL:
      m_textStyle = TS_LABEL;
      HardLineBreak();
      break;
    case MC_TYPE_INPUT:
      m_textStyle = TS_INPUT;
      break;
    case MC_TYPE_ERROR:
      m_textStyle = TS_ERROR;
      break;
    case MC_TYPE_WARNING:
      m_textStyle = TS_WARNING;
      break;
    case MC_TYPE_TEXT:
      m_textStyle = TS_TEXT;
      break;
    case MC_TYPE_SUBSUBSECTION:
      m_textStyle = TS_SUBSUBSECTION;
      break;
    case MC_TYPE_HEADING5:
      m_textStyle = TS_HEADING5;
      break;
    case MC_TYPE_HEADING6:
      m_textStyle = TS_HEADING6;
      break;
    case MC_TYPE_SUBSECTION:
      m_textStyle = TS_SUBSECTION;
      break;
    case MC_TYPE_SECTION:
      m_textStyle = TS_SECTION;
      break;
    case MC_TYPE_TITLE:
      m_textStyle = TS_TITLE;
      break;
    default:
      m_textStyle = TS_DEFAULT;
      break;
  }
  ResetSize();
  if (m_group)
    GetGroup()->ResetSize();
}

void Cell::CopyCommonData(const Cell & cell)
{
  m_altCopyText = cell.m_altCopyText;
  m_toolTip = cell.m_toolTip;
  m_forceBreakLine = cell.m_forceBreakLine;
  m_type = cell.m_type;
  m_textStyle = cell.m_textStyle;
  m_isHidden = cell.m_isHidden;
  m_isHidableMultSign = cell.m_isHidableMultSign;
}

std::unique_ptr<Cell> Cell::CopyList()
{
  auto ret = Copy();
  Cell *dest = ret.get();
  Cell *src = m_next.get();

  while (src)
  {
    dest->AppendCell(src->Copy());
    src = src->m_next.get();
    dest = dest->m_next.get();
  }
  return ret;
}

void Cell::ClearCacheList()
{
  for (auto &cell : OnCellList())
    cell.ClearCache();
}

void Cell::SetGroupList(Cell *group)
{
  for (auto &cell : OnCellList())
  {
    cell.SetGroup(group);
    cell.SetParent(this);
  }
}

int Cell::CellsInListRecursive() const
{
  //! The number of cells the current group contains (-1, if no GroupCell)
  int cells = 0;
  for (auto &cell : OnCellList())
  {
    ++ cells;
    for (auto &inner : cell.OnInnerCells())
    {
      // I believe with the if(cell) we cannot use std::accumulate here.
      // cppcheck-suppress useStlAlgorithm
      cells += inner.CellsInListRecursive();
    }
  }
  return cells;
}

void Cell::SetGroup(Cell *group)
{
  m_group = group;
  if (group)
    wxASSERT (group->GetType() == MC_TYPE_GROUP);
  
  for (auto &inner : OnInnerCells())
    inner.SetGroupList(group);
}

void Cell::FontsChangedList()
{
  for (auto &cell : OnCellList())
  {
    cell.FontsChanged();
    for (auto &inner : OnInnerCells())
      inner.FontsChangedList();
  }
}


/***
 * Append new cell to the end of this list.
 */
void Cell::AppendCell(OwningCellPtr p_next)
{
  if (!p_next)
    return;
  m_maxDrop = -1;
  m_maxCenter = -1;

  // Search the last cell in the list
  auto lastInList = ListBegin();
  while (lastInList->m_next)
    ++ lastInList;

  // Append this p_next to the list
  lastInList->m_next = std::move(p_next);
  lastInList->m_next->m_previous = lastInList;

  wxASSERT(lastInList);
  if (!lastInList)
    return;
  
  // Search the last cell in the list that is sorted by the drawing order
  Cell *LastToDraw = lastInList;
  while (LastToDraw->GetNextToDraw())
    LastToDraw = LastToDraw->GetNextToDraw();

  // Append p_next to this list.
  LastToDraw->SetNextToDraw(p_next.get());
}

Cell *Cell::GetGroup()
{
  wxASSERT_MSG(m_group, _("Bug: Math Cell that claims to have no group Cell it belongs to"));
  return m_group;
}

/***
 * Get the maximum drop of the center.
 */
int Cell::GetCenterList()
{
  if (m_maxCenter < 0 || (*m_configuration)->RecalculationForce())
  {
    m_maxCenter  = 0;
    for (auto &cell : OnDrawList())
    {
      if (&cell != this && cell.m_breakLine)
        break;
      if (!cell.m_isBrokenIntoLines)
        m_maxCenter = wxMax(m_maxCenter, cell.m_center);
    }
  }
  return m_maxCenter;
}

bool Cell::NeedsRecalculation(int fontSize)
{
  bool result = (m_width < 0) || (m_height < 0) || (m_center < 0) ||
    (fontSize != m_fontsize_old) ||
    (m_isBrokenIntoLines != m_isBrokenIntoLines_old) ||
    (m_currentPoint.x < 0) || (m_currentPoint.y < 0) ||
    (m_clientWidth_old != (*m_configuration)->GetClientWidth()) ||
    (m_lastZoomFactor != (*m_configuration)->GetZoomFactor()) ||
    ((*m_configuration)->RecalculationForce()) ||
    (*m_configuration)->FontChanged();
  return result;
}

/***
 * Get the maximum drop of cell.
 */
int Cell::GetMaxDrop()
{
//  if (m_maxDrop < 0 || (*m_configuration)->RecalculationForce())
  {
    m_maxDrop = 0;
    for (auto &cell : OnDrawList())
    {
      if (&cell != this && cell.m_breakLine)
        break;
      if (!cell.m_isBrokenIntoLines)
        m_maxDrop = wxMax(m_maxDrop, cell.m_height - cell.m_center);
    }
  }
  return m_maxDrop;
}

//!  Get the maximum hight of cells in line.
int Cell::GetHeightList()
{
  return GetCenterList() + GetMaxDrop();
}

/*! Get full width of this group.
 */
int Cell::GetFullWidth()
{
  // Recalculate the with of this list of cells only if this has been marked as necessary.
  if (m_fullWidth < 0 || (*m_configuration)->RecalculationForce())
  {
    // We begin this calculation with a negative offset since the full width of only a single
    // cell doesn't contain the space that separates two cells - that is automatically added
    // to every cell in the next step.
    m_fullWidth = 0;
    for (auto &cell : OnCellList())
      m_fullWidth += cell.m_width;
  }
  return m_fullWidth;
}

/*! Get the width of this line.
 */
int Cell::GetLineWidth()
{
  if (m_lineWidth < 0)
  {
    int width = m_width; // TODO FIXME This double-counts the width of the current line!!
    bool first = true;

    for (auto &cell: OnDrawList())
    {
      if (!first)
        if (cell.m_isBrokenIntoLines || cell.m_breakLine || cell.m_type == MC_TYPE_MAIN_PROMPT)
          break;
      first = false;

      width += cell.m_width;
    }
    m_lineWidth = width;
  }
  return m_lineWidth;
}

/*! Draw this cell to dc

 To make this work each derived class must draw the content of the cell
 and then call MathCall::Draw(...).
 */
void Cell::Draw(wxPoint point)
{
  Configuration *configuration = *m_configuration;
  if((m_height > 0) && (point.y > 0))
    SetCurrentPoint(point);

  // Mark all cells that contain tooltips
  if (!m_toolTip.empty() && GetStyle() != TS_LABEL && GetStyle() != TS_USERLABEL &&
      configuration->ClipToDrawRegion() && !configuration->GetPrinting())
  {
    wxRect rect = Cell::CropToUpdateRegion(GetRect());
    if (Cell::InUpdateRegion(rect))
    {
      if((rect.GetWidth() > 0) && rect.GetHeight() > 0)
      {
        wxDC *dc = configuration->GetDC();
        dc->SetPen(*wxTRANSPARENT_PEN);
        dc->SetBrush((*m_configuration)->GetTooltipBrush());
        dc->DrawRectangle(rect);
      }
    }
  }
  
  // Tell the screen reader that this cell's contents might have changed.
#if wxUSE_ACCESSIBILITY
  if (GetWorksheet())
    NotifyEvent(0, configuration->GetWorkSheet(), wxOBJID_CLIENT, wxOBJID_CLIENT);
#endif
}

void Cell::AddToolTip(const wxString &tip)
{
  if (!m_toolTip.empty() && !m_toolTip.EndsWith("\n"))
    m_toolTip += "\n";
  m_toolTip += tip;
}
void Cell::DrawList(wxPoint point)
{
  for (auto &cell : OnDrawList())
  {
    cell.Draw(point);
    point.x += cell.m_width;
    wxASSERT(&cell != cell.GetNextToDraw());
  }
}

void Cell::RecalculateList(int fontsize)
{
  for (auto &cell : OnDrawList())
  {
    cell.RecalculateWidths(fontsize);
    cell.RecalculateHeight(fontsize);
  }
}

void Cell::ResetSizeList()
{
  for (auto &cell : OnCellList())
    cell.ResetSize();
}


void Cell::RecalculateHeightList(int fontsize)
{
  for (auto &cell : OnCellList())
    cell.RecalculateHeight(fontsize);
}

/*! Recalculate widths of cells.

  (Used for changing font size since in this case all size information has to be
  recalculated).

  Should set: set m_width.
*/
void Cell::RecalculateWidthsList(const int &fontsize)
{
  for (auto &cell : OnCellList())
    cell.RecalculateWidths(fontsize);
}

void Cell::RecalculateWidths(int WXUNUSED(fontsize))
{}

void Cell::RecalculateHeight(int fontsize)
{
  if(NeedsRecalculation(fontsize))
    ResetData();
  m_fontSize = fontsize;
  m_fontsize_old = fontsize;
  m_isBrokenIntoLines_old = m_isBrokenIntoLines;
  m_clientWidth_old = (*m_configuration)->GetClientWidth();
  m_lastZoomFactor = (*m_configuration)->GetZoomFactor();
}

/*! Is this cell currently visible in the window?.
 */
bool Cell::DrawThisCell(wxPoint point)
{
  // If the cell isn't on the worksheet we don't draw it.
  if((point.x < 0) || (point.y < 0))
    return false;

  SetCurrentPoint(point);

  // If a cell is broken into lines the cells it contains are displayed but
  // not the cell itself (example: Denominator and Numerator are displayed
  // but not the horizontal line with denominator above and numerator below.
  if(m_isBrokenIntoLines)
    return false;
  
  if(!(*m_configuration)->ClipToDrawRegion())
    return true;
  
  return(InUpdateRegion());
}

wxRect Cell::GetRect(bool wholeList)
{
  if (wholeList)
    return wxRect(m_currentPoint.x, m_currentPoint.y - GetCenterList(),
                  GetLineWidth(), GetHeightList());
  else
    return wxRect(m_currentPoint.x, m_currentPoint.y - m_center,
                  m_width, m_height);
}

bool Cell::InUpdateRegion(const wxRect &rect)
{
  if (!(*m_configuration)->ClipToDrawRegion())
    return true;

  if((m_currentPoint.x < 0) || (m_currentPoint.y < 0))
    return false;
  
  wxRect updateRegion = (*m_configuration)->GetUpdateRegion();

  // If we have deferred the recalculation of the cell height but now
  // got a draw request due to moving the mouse wheel we need to guess
  // the cell size 
  if(m_height < 0)
  {
    int height = 0;
    if(m_next)
      height = m_next->m_currentPoint.y - m_currentPoint.y;

    if ((updateRegion.GetBottom() >= m_currentPoint.y) &&
        (updateRegion.GetTop() <= m_currentPoint.y+height))
    return true;
  }

  if(updateRegion.Contains(m_currentPoint))
    return true;

  return updateRegion.Intersects(rect) ||
    updateRegion.Contains(rect) ||
    (updateRegion == rect) || rect.Contains(updateRegion);
}

void Cell::DrawBoundingBox(wxDC &dc, bool all)
{
  wxRect rect = GetRect(all);
  if (InUpdateRegion())
  {
    dc.DrawRectangle(CropToUpdateRegion(rect));
  }
}

/***
 * Do we have an operator in this line - draw () in frac...
 */
bool Cell::IsCompound()
{
  for (auto &cell : OnCellList())
    if (cell.IsOperator())
      return true;
  return false;
}

/***
 * Is operator - draw () in frac...
 */
bool Cell::IsOperator() const
{
  return false;
}

/***
 * Return the string representation of cell.
 */
wxString Cell::ToString()
{
  return {};
}

wxString Cell::VariablesAndFunctionsList()
{
  wxString retval;
  for (auto &cell : OnDrawList())
  {
    auto style = cell.GetStyle();
    if(
      (style == TS_LABEL) ||
      (style == TS_USERLABEL) ||
      (style == TS_MAIN_PROMPT) ||
      (style == TS_VARIABLE) ||
      (style == TS_FUNCTION))
      retval += cell.ToString() + wxT(' ');
  }
  return retval;
}

wxString Cell::ListToString()
{
  wxString retval;
  bool firstline = true;

  for (auto &cell : OnDrawList())
  {
    if (!firstline && cell.m_forceBreakLine)
    {
      if (!retval.EndsWith(wxT('\n')))
        retval += wxT("\n");
      // if(
      //    (tmp->GetStyle() != TS_LABEL) &&
      //    (tmp->GetStyle() != TS_USERLABEL) &&
      //    (tmp->GetStyle() != TS_MAIN_PROMPT) &&
      //    (tmp->GetStyle() != TS_OTHER_PROMPT))
      //   retval += wxT("\t");
    }
    // if(firstline)
    // {
    //   if((tmp->GetStyle() != TS_LABEL) &&
    //      (tmp->GetStyle() != TS_USERLABEL) &&
    //      (tmp->GetStyle() != TS_MAIN_PROMPT) &&
    //      (tmp->GetStyle() != TS_OTHER_PROMPT))
    //     retval += wxT("\t");
    // }
    retval += cell.ToString();

    firstline = false;
  }
  return retval;
}

wxString Cell::ToMatlab()
{
  return wxEmptyString;
}

wxString Cell::ListToMatlab()
{
  bool firstline = true;
  wxString retval;

  for (auto &cell : OnDrawList())
  {
    // auto style = cell.GetStyle();
    if (!firstline && cell.m_forceBreakLine)
    {
      if (!retval.EndsWith(wxT('\n')))
        retval += wxT("\n");
      // if(
      //    (style != TS_LABEL) &&
      //    (style != TS_USERLABEL) &&
      //    (style != TS_MAIN_PROMPT) &&
      //    (style != TS_OTHER_PROMPT))
      //   retval += wxT("\t");
    }
    // if(firstline)
    // {
    //   if((style != TS_LABEL) &&
    //      (style != TS_USERLABEL) &&
    //      (style != TS_MAIN_PROMPT) &&
    //      (style != TS_OTHER_PROMPT))
    //     retval += wxT("\t");
    // }
    retval += cell.ToMatlab();

    firstline = false;
  }

  return retval;
}

wxString Cell::ToTeX()
{
  return {};
}

wxString Cell::ListToTeX()
{
  wxString retval;

  for (auto &cell : OnCellList())
  {
    if ((cell.m_textStyle == TS_LABEL && !retval.empty()) ||
        (cell.m_breakLine && !retval.empty()))
      retval += wxT("\\]\\[");
    retval += cell.ToTeX();
  }
  return retval;
}

wxString Cell::ToXML()
{ return {}; }

wxString Cell::ToMathML()
{ return {}; }

wxString Cell::ListToMathML(bool startofline)
{
  bool highlight = false;
  wxString retval;

  // If the region to export contains linebreaks or labels we put it into a table.
  bool needsTable = false;

  for (auto &cell : OnCellList())
  {
    if (cell.HardLineBreak())
      needsTable = true;
    else if (cell.GetType() == MC_TYPE_LABEL)
      needsTable = true;
    else
      continue;
    break;
  }

  // If the list contains multiple cells we wrap them in a <mrow> in order to
  // group them into a single object.
  bool multiCell = bool(m_next);

  // Export all cells
  for (auto &cell : OnCellList())
  {
    // Do we need to end a highlighting region?
    if (!cell.m_highlight && highlight)
      retval += wxT("</mrow>");

    // Handle linebreaks
    if (&cell != this && cell.HardLineBreak())
      retval += wxT("</mtd></mlabeledtr>\n<mlabeledtr columnalign=\"left\"><mtd>");

    // If a linebreak isn't followed by a label we need to introduce an empty one.
    if ((((cell.HardLineBreak()) || (startofline && (this == &cell))) &&
         ((cell.GetStyle() != TS_LABEL) && (cell.GetStyle() != TS_USERLABEL))) && (needsTable))
      retval += wxT("<mtext></mtext></mtd><mtd>");

    // Do we need to start a highlighting region?
    if (cell.m_highlight && !highlight)
      retval += wxT("<mrow mathcolor=\"red\">");
    highlight = cell.m_highlight;

    retval += cell.ToMathML();
  }

  // If the region we converted to MathML ended within a highlighted region
  // we need to close this region now.
  if (highlight)
    retval += wxT("</mrow>");

  // If we grouped multiple cells as a single object we need to cose this group now
  if (multiCell && !needsTable)
    retval = wxT("<mrow>") + retval + wxT("</mrow>\n");

  // If we put the region we exported into a table we need to end this table now
  if (needsTable)
    retval = wxT("<mtable>\n<mlabeledtr columnalign=\"left\"><mtd>") + retval + wxT("</mtd></mlabeledtr>\n</mtable>");
  return retval;
}

wxString Cell::OMML2RTF(wxXmlNode *node)
{
  wxString result;

  while (node != NULL)
  {
    if (node->GetType() == wxXML_ELEMENT_NODE)
    {
      wxString ommlname = node->GetName();
      result += wxT("{\\m") + ommlname.Right(ommlname.Length() - 2);

      // Convert the attributes
      wxXmlAttribute *attributes = node->GetAttributes();
      while (attributes != NULL)
      {
        wxString ommlatt = attributes->GetName();
        result += wxT("{\\m") + ommlatt.Right(ommlatt.Length() - 2) +
                  wxT(" ") + attributes->GetValue() + wxT("}");
        attributes = attributes->GetNext();
      }

      // Convert all child nodes
      if (node->GetChildren() != NULL)
      {
        result += OMML2RTF(node->GetChildren());
      }
      result += wxT("}");
    }
    else
      result += wxT(" ") + RTFescape(node->GetContent());

    node = node->GetNext();
  }
  return result;
}

wxString Cell::OMML2RTF(const wxString &ommltext)
{
  if (ommltext.empty())
    return {};

  wxString result;
  wxXmlDocument ommldoc;
  wxStringInputStream ommlStream(wxT("<m:r>") + ommltext + wxT("</m:r>"));

  ommldoc.Load(ommlStream, wxT("UTF-8"));

  wxXmlNode *node = ommldoc.GetRoot();
  result += OMML2RTF(node);

  if (!result.empty() && result != wxT("\\mr"))
  {
    result = wxT("{\\mmath {\\*\\moMath") + result + wxT("}}");
  }
  return result;
}

wxString Cell::XMLescape(wxString input)
{
  input.Replace(wxT("&"), wxT("&amp;"));
  input.Replace(wxT("<"), wxT("&lt;"));
  input.Replace(wxT(">"), wxT("&gt;"));
  input.Replace(wxT("'"), wxT("&apos;"));
  input.Replace(wxT("\""), wxT("&quot;"));
  return input;
}

wxString Cell::RTFescape(wxString input, bool MarkDown)
{
  // Characters with a special meaning in RTF
  input.Replace("\\", "\\\\");
  input.Replace("{", "\\{");
  input.Replace("}", "\\}");
  input.Replace(wxT("\r"), "\n");

  // The Character we will use as a soft line break
  input.Replace("\r", wxEmptyString);

  // Encode unicode characters in a rather mind-boggling way
  wxString output;
  for (size_t i = 0; i < input.Length(); i++)
  {
    wxChar ch = input[i];
    if (ch == wxT('\n'))
    {
      if (((i > 0) && (input[i - 1] == wxT('\n'))) || !MarkDown)
        output += wxT("\\par}\n{\\pard ");
      else
        output += wxT("\n");
    }
    else
    {
      if ((ch < 128) && (ch > 0))
      {
        output += ch;
      }
      else
      {
        if (ch < 32768)
        {
          output += wxString::Format("\\u%i?", int(ch));
        }
        else
        {
          output += wxString::Format("\\u%i?", int(ch) - 65536);
        }
      }
    }
  }
  return (output);
}

wxString Cell::ListToOMML(bool WXUNUSED(startofline))
{
  bool multiCell = bool(m_next);

  wxString retval;

  // If the region to export contains linebreaks or labels we put it into a table.
  // Export all cells

  for (auto &cell : OnCellList())
  {
    wxString token = cell.ToOMML();

    // End exporting the equation if we reached the end of the equation.
    if (token.empty())
      break;

    retval += token;

    // Hard linebreaks aren't supported by OMML and therefore need a new equation object
    if (cell.HardLineBreak())
      break;
  }

  if (multiCell && !retval.empty())
    return wxT("<m:r>") + retval + wxT("</m:r>");
  else
    return retval;
}

wxString Cell::ListToRTF(bool startofline)
{
  wxString retval;

  for (auto cell = ListBegin(); cell != ListEnd(); ++ cell)
  {
    wxString rtf = cell->ToRTF();
    if (!rtf.empty())
    {
      if (GetStyle() == TS_LABEL || GetStyle() == TS_USERLABEL)
      {
        retval += wxT("\\par}\n{\\pard\\s22\\li1105\\lin1105\\fi-1105\\f0\\fs24 ") + rtf + wxT("\\tab");
        startofline = false;
      }
      else
      {
        if (startofline)
          retval += wxT("\\par}\n{\\pard\\s21\\li1105\\lin1105\\f0\\fs24 ") + rtf + wxT("\\n");
        startofline = true;
      }
    }
    else
    {
      if (!cell->ListToOMML().empty())
      {
        // Math!

        // set the style for this line.
        if (startofline)
          retval += wxT("\\pard\\s21\\li1105\\lin1105\\f0\\fs24 ");

        retval += OMML2RTF(cell->ListToOMML());

        startofline = true;

        // Skip the rest of this equation
        for (; cell != ListEnd(); ++ cell)
        {
          // A non-equation item starts a new rtf item
          if (cell->ToOMML().empty())
            break;

          // A newline starts a new equation
          if (cell->HardLineBreak())
            break;
        }
      }
    }
  }
  return retval;
}

void Cell::SelectPointText(const wxPoint &WXUNUSED(point)){}

void Cell::SelectRectText(const wxPoint &WXUNUSED(one), const wxPoint &WXUNUSED(two)){}

void Cell::PasteFromClipboard(const bool &WXUNUSED(primary)){}

wxString Cell::ListToXML()
{
  bool highlight = false;
  wxString retval;

  for (auto &cell : OnCellList())
  {
    if (cell.GetHighlight() && !highlight)
    {
      retval += wxT("<hl>\n");
      highlight = true;
    }

    if (!cell.GetHighlight() && highlight)
    {
      retval += wxT("</hl>\n");
      highlight = false;
    }

    retval += cell.ToXML();
  }

  if (highlight)
    retval += wxT("</hl>\n");

  return retval;
}

/***
 * Get the part for diff tag support - only ExpTag overvrides this.
 */
wxString Cell::GetDiffPart()
{
  return {};
}

/***
 * Find the first and last cell in rectangle rect in this line.
 */
void Cell::SelectRect(const wxRect &rect, CellPtr &first, CellPtr &last)
{
  SelectFirst(rect, first);
  if (first)
  {
    last = first;
    first->SelectLast(rect, last);
    if (last == first)
      first->SelectInner(rect, first, last);
  }
  else
    last = nullptr;
}

/***
 * Find the first cell in rectangle rect in this line.
 */
void Cell::SelectFirst(const wxRect &rect, CellPtr &first)
{
  if (rect.Intersects(GetRect(false)))
    first = this;
  else if (GetNextToDraw())
    GetNextToDraw()->SelectFirst(rect, first);
  else
    first = nullptr;
}

/***
 * Find the last cell in rectangle rect in this line.
 */
void Cell::SelectLast(const wxRect &rect, CellPtr &last)
{
  if (rect.Intersects(GetRect(false)))
    last = this;
  if (GetNextToDraw())
    GetNextToDraw()->SelectLast(rect, last);
}

/***
 * Select rectangle in deeper cell
 */
void Cell::SelectInner(const wxRect &rect, CellPtr &first, CellPtr &last)
{
  first = nullptr;
  last = nullptr;

  for (auto &inner : OnInnerCells())
    for (auto &cell : inner.OnCellList())
      if (cell.ContainsRect(rect))
        cell.SelectRect(rect, first, last);

  if (!first || !last)
  {
    first = this;
    last = this;
  }
}

bool Cell::BreakLineHere() const
{
  return (((!m_isBrokenIntoLines) && m_breakLine) || m_forceBreakLine);
}

bool Cell::ContainsRect(const wxRect &sm, bool all)
{
  wxRect big = GetRect(all);
  if (big.x <= sm.x &&
      big.y <= sm.y &&
      big.x + big.width >= sm.x + sm.width &&
      big.y + big.height >= sm.y + sm.height)
    return true;
  return false;
}

/*!
 Resets remembered data.

 Resets cached data like width and the height of the current cell
 as well as the vertical position of the center. Temporarily unbreaks all
 lines until the widths are recalculated if there aren't any hard line
 breaks.
 */
void Cell::ResetData()
{
  m_fullWidth = -1;
  m_lineWidth = -1;
  m_maxCenter = -1;
  m_maxDrop   = -1;
  for (auto &inner : OnInnerCells())
    for (auto &cell : inner.OnCellList())
      cell.ResetData();
}

Cell *Cell::first()
{
  Cell *tmp = this;
  while (tmp->m_previous)
    tmp = tmp->m_previous;

  return tmp;
}

Cell *Cell::last()
{
  auto cell = ListBegin();
  while (cell != ListEnd())
    ++ cell;

  return &*cell;
}

void Cell::Unbreak()
{
  if (m_isBrokenIntoLines)
    ResetData();

  m_isBrokenIntoLines = false;
  SetNextToDraw(m_next.get());

  // Unbreak the inner cells, too
  for (auto &inner : OnInnerCells())
    inner.UnbreakList();
}

void Cell::UnbreakList()
{
  for (auto &cell : OnCellList())
    cell.Unbreak();
}

// cppcheck-suppress functionStatic
// cppcheck-suppress functionConst
// Set the pen in device context according to the style of the cell.
void Cell::SetPen(double lineWidth)
{
  Configuration *configuration = (*m_configuration);
  wxDC *dc = configuration->GetDC();

  wxPen pen;

  if (m_highlight)
    pen = *(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_HIGHLIGHT),
                                          lineWidth * configuration->GetDefaultLineWidth(), wxPENSTYLE_SOLID));
  else if (m_type == MC_TYPE_PROMPT)
    pen = *(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_OTHER_PROMPT),
                                              lineWidth * configuration->GetDefaultLineWidth(), wxPENSTYLE_SOLID));
  else if (m_type == MC_TYPE_INPUT)
    pen = *(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_INPUT),
                                          lineWidth * configuration->GetDefaultLineWidth(), wxPENSTYLE_SOLID));
  else
    pen = *(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_DEFAULT),
                                          lineWidth * configuration->GetDefaultLineWidth(), wxPENSTYLE_SOLID));

  dc->SetPen(pen);
  if(configuration->GetAntialiassingDC() != dc)
    configuration->GetAntialiassingDC()->SetPen(pen);
}

/***
 * Reset the pen in the device context.
 */
void Cell::UnsetPen()
{
  Configuration *configuration = (*m_configuration);
  wxDC *dc = configuration->GetDC();
  if (m_type == MC_TYPE_PROMPT || m_type == MC_TYPE_INPUT || m_highlight)
    dc->SetPen(*(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_DEFAULT),
                                              1, wxPENSTYLE_SOLID)));
}

void Cell::SetForeground()
{
  Configuration *configuration = (*m_configuration);
  wxColour color;
  wxDC *dc = configuration->GetDC();
  if (m_highlight)
  {
    color = configuration->GetColor(TS_HIGHLIGHT);
  }
  else
  {
    switch (m_type)
    {
      case MC_TYPE_PROMPT:
        color = configuration->GetColor(TS_OTHER_PROMPT);
        break;
      case MC_TYPE_MAIN_PROMPT:
        color = configuration->GetColor(TS_MAIN_PROMPT);
        break;
      case MC_TYPE_ERROR:
        color = wxColour(wxT("red"));
        break;
      case MC_TYPE_WARNING:
        color = configuration->GetColor(TS_WARNING);
        break;
      case MC_TYPE_LABEL:
        color = configuration->GetColor(TS_LABEL);
        break;
      default:
        color = configuration->GetColor(m_textStyle);
        break;
    }
  }

  dc->SetTextForeground(color);
}

bool Cell::IsMath() const
{
  return !(m_textStyle == TS_LABEL ||
           m_textStyle == TS_USERLABEL ||
           m_textStyle == TS_INPUT);
}

#if wxUSE_ACCESSIBILITY
wxAccStatus Cell::GetDescription(int childId, wxString *description)
{
  if (!description)
    return wxACC_FAIL;

  if (childId == 0)
    return (*description = _("Math output")), wxACC_OK;

  wxAccessible *cell = {};
  if (GetChild(childId, &cell) == wxACC_OK && cell)
    return cell->GetDescription(0, description);

  return (description->clear()), wxACC_FAIL;
}

wxAccStatus Cell::GetParent (wxAccessible **parent)
{
  if (!parent)
    return wxACC_FAIL;

  if (*parent != this)
    return (*parent = m_parent), wxACC_OK;

  if ((*m_configuration)->GetWorkSheet())
    *parent = (*m_configuration)->GetWorkSheet()->GetAccessible();

  return wxACC_OK;
}

wxAccStatus Cell::GetValue (int childId, wxString *strValue)
{
  if (!strValue)
    return wxACC_FAIL;

  wxAccessible *cell;
  if (GetChild(childId, &cell) == wxACC_OK)
    return (*strValue = static_cast<Cell*>(cell)->ToString()), wxACC_OK;

  return (strValue->clear()), wxACC_FAIL;
}

wxAccStatus Cell::GetChildCount(int *childCount)
{
  if (!childCount)
    return wxACC_FAIL;

  int count = 0;
  for (auto &inner : OnInnerCells())
    ++ count;

  return (*childCount = count), wxACC_OK;
}

wxAccStatus Cell::HitTest(const wxPoint &pt, int *childId, wxAccessible **child)
{
  wxRect rect;
  GetLocation(rect, 0);
  // If this cell doesn't contain the point none of the sub-cells does.
  if (!rect.Contains(pt))
    return (childId && (*childId = 0)), (child && (*child = nullptr)),
           wxACC_FAIL;

  int id = 0; // Child #0 is this very cell
  for (auto &inner : OnInnerCells())
  {
    ++ id; // The first valid inner cell will have id #1, and so on.

    inner.GetLocation(rect, 0);
    if (rect.Contains(pt))
      return (childId && (*childId = id)), (child && (*child = &inner)),
             wxACC_OK;
  }

  return (childId && (*childId = 0)), (child && (*child = this)),
         wxACC_OK;
}

wxAccStatus Cell::GetChild(int childId, wxAccessible **child)
{
  if (!child)
    return wxACC_FAIL;

  if (childId == 0)
    return (*child = this), wxACC_OK;

  if (childId > 0)
    for (auto &inner : OnInnerCells())
      if (--childId == 0)
        return (*child = &inner), wxACC_OK;

  return wxACC_FAIL;
}

wxAccStatus Cell::GetFocus(int *childId, wxAccessible **child)
{
  int id = 0;
  for (auto &inner : OnInnerCells())
  {
    ++ id;
    int dummy;
    if (inner.GetFocus(&dummy, child) == wxACC_OK)
      return (childId && (*childId = id)), (child && (*child = &inner)),
             wxACC_OK;
  }

  return (childId && (*childId = 0)), (child && (*child = nullptr)),
         wxACC_FAIL;
}

wxAccStatus Cell::GetLocation(wxRect &rect, int elementId)
{
  if (elementId == 0)
  {
    rect = wxRect(GetRect().GetTopLeft()     + (*m_configuration)->GetVisibleRegion().GetTopLeft(),
                  GetRect().GetBottomRight() + (*m_configuration)->GetVisibleRegion().GetTopLeft());
    if(rect.GetTop() < 0)
      rect.SetTop(0);
    if(rect.GetLeft() < 0)
      rect.SetLeft(0);
    if(rect.GetBottom() > (*m_configuration)->GetVisibleRegion().GetWidth())
      rect.SetBottom((*m_configuration)->GetVisibleRegion().GetWidth());
    if(rect.GetRight() > (*m_configuration)->GetVisibleRegion().GetHeight())
      rect.SetRight((*m_configuration)->GetVisibleRegion().GetHeight());
    rect = wxRect(rect.GetTopLeft()+(*m_configuration)->GetWorksheetPosition(),rect.GetBottomRight()+(*m_configuration)->GetWorksheetPosition());
    return wxACC_OK;
  }

  wxAccessible *cell = {};
  if (GetChild(elementId, &cell) == wxACC_OK)
    return cell->GetLocation(rect, 0);

  return wxACC_FAIL;
}

wxAccStatus Cell::GetRole(int WXUNUSED(childId), wxAccRole *role)
{
  if (!role)
    return wxACC_FAIL;

  return (*role = wxROLE_SYSTEM_STATICTEXT), wxACC_OK;
}

#endif

Cell::CellPointers::CellPointers(wxScrolledCanvas *mathCtrl) :
    m_mathCtrl(mathCtrl)
{}

wxString Cell::CellPointers::WXMXGetNewFileName()
{
  wxString file(wxT("image"));
  file << (++m_wxmxImgCounter) << wxT('.');
  return file;
}

Cell::InnerCellIterator Cell::InnerBegin() const { return {}; }
