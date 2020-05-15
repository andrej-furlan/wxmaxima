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
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
//  SPDX-License-Identifier: GPL-2.0+

#ifndef MATRCELL_H
#define MATRCELL_H

#include "Cell.h"

#include <vector>

using namespace std;

class MatrCell final : public Cell
{
public:
  MatrCell(Cell *parent, Configuration **config);
  MatrCell(const MatrCell &cell);
  OwningCellPtr Copy() override {return OwningCellPtr(new MatrCell(*this));}
  ~MatrCell();

  InnerCellIterator InnerBegin() const override
  { return m_cells.empty() ? InnerCellIterator{} : InnerCellIterator{&m_cells.front(), &m_cells.back()+1}; }

  void RecalculateHeight(int fontsize) override;

  void RecalculateWidths(int fontsize) override;

  void Draw(wxPoint point) override;

  void AddNewCell(OwningCellPtr cell) { m_cells.emplace_back(std::move(cell)); }

  void NewRow() { m_matHeight++; }

  void NewColumn() { m_matWidth++; }

  void SetDimension();

  wxString ToString() override;

  wxString ToMatlab() override;

  wxString ToTeX() override;

  wxString ToMathML() override;

  wxString ToOMML() override;

  wxString ToXML() override;

  void SetSpecialFlag(bool special) {m_specialMatrix = special;}

  void SetInferenceFlag(bool inference) {m_inferenceMatrix = inference;}

  void RowNames(bool rn) {m_rowNames = rn;}

  void ColNames(bool cn) {m_colNames = cn;}

  void RoundedParens(bool rounded) {m_roundedParens = rounded;}

  void SetNextToDraw(Cell *next) override;

  Cell *GetNextToDraw() const override {return m_nextToDraw;}

private:
  CellPtr m_nextToDraw;

  unsigned int m_matWidth;
  bool m_roundedParens;
  unsigned int m_matHeight;
  bool m_specialMatrix, m_inferenceMatrix, m_rowNames, m_colNames;
  //! Collections of pointers to inner cells.
  vector<OwningCellPtr> m_cells;
  vector<int> m_widths;
  vector<int> m_drops;
  vector<int> m_centers;
};

#endif // MATRCELL_H
