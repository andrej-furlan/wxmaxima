// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2018 Gunter Königsmann <wxMaxima@physikbuch.de>
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

#include "VariablesPane.h"
#include "memory"

Variablespane::Variablespane(wxWindow *parent, wxWindowID id) : wxGrid(parent, id)
{
  SetMinSize(wxSize(wxSystemSettings::GetMetric ( wxSYS_SCREEN_X )/10,
                    wxSystemSettings::GetMetric ( wxSYS_SCREEN_Y )/10));
  CreateGrid(1,2);
  SetUseNativeColLabels();
  wxGridCellAttr *attr0, *attr1;
  attr0 = new wxGridCellAttr;
  SetColAttr(0,attr0);
  SetColLabelValue(0,_("Variable"));
  attr1 = new wxGridCellAttr;
//  attr1->SetReadOnly();
  SetColAttr(1,attr1);
  SetColLabelValue(1,_("Contents"));
  m_rightClickRow = -1;
  Connect(wxEVT_GRID_CELL_CHANGED,
          wxGridEventHandler(Variablespane::OnTextChange),
          NULL, this);
  Connect(wxEVT_GRID_CELL_CHANGING,
          wxGridEventHandler(Variablespane::OnTextChanging),
          NULL, this);
  Connect(wxEVT_GRID_CELL_RIGHT_CLICK,
          wxGridEventHandler(Variablespane::OnRightClick),
          NULL, this);
  Connect(wxEVT_MENU,
          wxCommandEventHandler(Variablespane::InsertMenu),
          NULL, this);
  Connect(wxEVT_KEY_DOWN,
          wxKeyEventHandler(Variablespane::OnKey),
          NULL, this);
//  Connect(wxEVT_CHAR,
//          wxKeyEventHandler(Variablespane::OnChar),
//          NULL, this);
  HideRowLabels();
  EnableDragCell();
}

void Variablespane::OnChar(wxKeyEvent &event)
{
  wxChar txt(event.GetUnicodeKey());
  if((wxIsprint(txt))&&(GetGridCursorRow()>=0))
  {
    SetCellValue(GetGridCursorRow(),0,wxString(txt));
    GoToCell(GetGridCursorRow(),0);
    ShowCellEditControl();
    EnableCellEditControl();    
  }
}

void Variablespane::OnKey(wxKeyEvent &event)
{
  switch (event.GetKeyCode())
  {

  case WXK_DELETE:
  case WXK_NUMPAD_DELETE:
  {
    if(GetNumberRows() > 1)
    {
      BeginBatch();
      wxArrayInt selectedRows = GetSelectedRows();
      selectedRows.Sort(CompareInt);
      
      if(!selectedRows.empty())
      {
        int offset = 0;
        for (wxArrayInt::const_iterator it = selectedRows.begin(); it != selectedRows.end(); ++it)
        {
          DeleteRows(*it-offset);
          offset++;
        }
      }
      else
      {
        DeleteRows(GetGridCursorRow());
      }
      wxGridEvent evt(wxID_ANY,wxEVT_GRID_CELL_CHANGED,this,GetNumberRows()-1,0);
      OnTextChange(evt);
      EndBatch();
    }
    break;
  }
  default:
    event.Skip();
  }
}

void Variablespane::InsertMenu(wxCommandEvent &event)
{
  wxString varname;
  switch(event.GetId())
  {
  case varID_values:varname="values";break;
  case varID_functions:varname="functions";break;
  case varID_arrays:varname="arrays";break;
  case varID_myoptions:varname="myoptions";break;
  case varID_rules:varname="rules";break;
  case varID_aliases:varname="aliases";break;
  case varID_structs:varname="structures";break;
  case varID_dependencies:varname="dependencies";break;
  case varID_gradefs:varname="gradefs";break;
  case varID_prop:varname="props";break;
  case varID_let_rule_packages:varname="let_rule_packages";break;
  case varID_clear:Clear();break;
  case varID_add_all:
  {
    wxMenuEvent *VarAddEvent = new wxMenuEvent(wxEVT_MENU, varID_add_all);
    GetParent()->GetParent()->GetEventHandler()->QueueEvent(VarAddEvent);
    break;
  }
  case varID_delete_row:
    if((m_rightClickRow>=0)&&(m_rightClickRow<GetNumberRows()))
    {
      DeleteRows(m_rightClickRow);
      wxGridEvent evt(wxID_ANY,wxEVT_GRID_CELL_CHANGED,this,m_rightClickRow,0);
      OnTextChange(evt);
    }
    break;
  }
  SetCellValue(GetNumberRows()-1,0,varname);
  wxGridEvent evt(wxID_ANY,wxEVT_GRID_CELL_CHANGED,this,GetNumberRows()-1,0);
  OnTextChange(evt);
}

void Variablespane::OnRightClick(wxGridEvent &event)
{
  m_rightClickRow = event.GetRow();
  m_vars.clear();
  for(int i = 0; i < GetNumberRows(); i++)
    m_vars[GetCellValue(i,0)] = 1;
  
  std::unique_ptr<wxMenu> popupMenu(new wxMenu);
  if(m_vars["values"] != 1)
    popupMenu->Append(varID_values,
                      _("List of user variables"));
  if(m_vars["functions"] != 1)
    popupMenu->Append(varID_functions,
                      _("List of user functions"));
  if(m_vars["arrays"] != 1)
    popupMenu->Append(varID_arrays,
                      _("List of arrays"));
  if(m_vars["myoptions"] != 1)
    popupMenu->Append(varID_myoptions,
                      _("List of changed options"));
  if(m_vars["rules"] != 1)
    popupMenu->Append(varID_rules,
                      _("List of user rules"));
  if(m_vars["aliases"] != 1)
    popupMenu->Append(varID_aliases,
                      _("List of user aliases"));
  if(m_vars["structures"] != 1)
    popupMenu->Append(varID_structs,
                      _("List of structs"));
  if(m_vars["gradefs"] != 1)
    popupMenu->Append(varID_gradefs,
                      _("List of user-defined derivatives"));
  if(m_vars["props"] != 1)
    popupMenu->Append(varID_prop,
                      _("List of user-defined properties"));
  if(m_vars["gradefs"] != 1)
    popupMenu->Append(varID_gradefs,
                      _("List of user-defined let rule packages"));
  popupMenu->AppendSeparator();    
  if(GetGridCursorRow()>=0)
  {
    popupMenu->Append(varID_delete_row,
                      _("Remove"));
  }
  
  if(GetNumberRows()>2)
  {
    popupMenu->Append(varID_clear,
                      _("Remove all"));
  }
  popupMenu->Append(varID_add_all,
                    _("Add all"));

  if (popupMenu->GetMenuItemCount() > 0)
    PopupMenu(popupMenu.get());  
}

void Variablespane::OnTextChanging(wxGridEvent &event)
{
  // Setting the 2nd column to "Read-only" prevents copy-and-paste.
  // Preventing edits is therefore perhaps better for our purposes.
  if(event.GetCol()==1)
  {
    event.Veto();
    return;
  }
}

void Variablespane::OnTextChange(wxGridEvent &event)
{
  if((event.GetRow()>GetNumberRows()) || (event.GetRow()<0))
    return;
  BeginBatch();
  if(IsValidVariable(GetCellValue(event.GetRow(),0)))
  {
    SetCellValue(event.GetRow(),1,wxT(""));
    SetCellTextColour(event.GetRow(),0,*wxBLACK);
  }
  else
  {
    if(!GetCellValue(event.GetRow(),0).empty())
    {
      SetCellTextColour(event.GetRow(),0,*wxRED);
      SetCellTextColour(event.GetRow(),1,*wxLIGHT_GREY);
      SetCellValue(event.GetRow(),1,_("(Not a valid variable name)"));
      RefreshAttr(event.GetRow(), 1);
    }
  }
  RefreshAttr(event.GetRow(), 0);

  if((GetNumberRows() == 0) || (!GetCellValue(GetNumberRows()-1,0).empty()))
    AppendRows();
  else
    for(int i = 0; i < GetNumberRows() - 1; i++)
      if(GetCellValue(i,0).empty())
        DeleteRows(i);
  wxMenuEvent *VarReadEvent = new wxMenuEvent(wxEVT_MENU, varID_newVar);
  GetParent()->GetParent()->GetEventHandler()->QueueEvent(VarReadEvent);

  // Avoid introducing a cell with the same name twice.
  m_vars.clear();
  for(int i = 0; i < GetNumberRows(); i++)
  {
    if(i!=event.GetRow())
      m_vars[GetCellValue(i,0)] = i+1;      
  }
  int identicalVar = m_vars[GetCellValue(event.GetRow(),0)];
  if(identicalVar > 0)
  {
    wxEventBlocker blocker(this);
    DeleteRows(identicalVar-1);
  }
  EndBatch();
}

void Variablespane::VariableValue(const wxString &var, const wxString &val)
{
  for(int i = 0; i < GetNumberRows(); i++)
    if(GetCellValue(i,0) == UnescapeVarname(var))
    {
      SetCellTextColour(i,1,*wxBLACK);
      SetCellValue(i,1,val);
      RefreshAttr(i, 1);
    }
}

void Variablespane::VariableUndefined(const wxString &var)
{
  for(int i = 0; i < GetNumberRows(); i++)
    if(GetCellValue(i,0) == UnescapeVarname(var))
    {
      SetCellTextColour(i,1,*wxLIGHT_GREY);
      SetCellValue(i,1,_("Undefined"));
      RefreshAttr(i, 1);
    }
}

wxArrayString Variablespane::GetEscapedVarnames()
{
  wxArrayString retVal;
  for(int i = 0; i < GetNumberRows(); i++)
  {
    wxString var = GetCellValue(i,0);
    if(IsValidVariable(var))
      retVal.Add(InvertCase(EscapeVarname(var)));
  }
  return retVal;
}

wxArrayString Variablespane::GetVarnames()
{
  wxArrayString retVal;
  for(int i = 0; i < GetNumberRows(); i++)
  {
    wxString var = GetCellValue(i,0);
    retVal.Add(var);
  }
  return retVal;
}

wxString Variablespane::InvertCase(const wxString &var)
{
  wxString retval;
  for (auto ch : var)
  {
    if (wxIsupper(ch))
      retval += wxTolower(ch);
    else if (wxIslower(ch))
      retval += wxToupper(ch);
    else
      retval += ch;
  }
  return retval;
}

void Variablespane::AddWatchCode(const wxString &code)
{
  bool lastEscape = false;
  wxString unescapedCode;
  for (auto ch : code)
  {
    if (ch == '\\' && !lastEscape)
      lastEscape = true;
    else
    {
      unescapedCode += ch;
      lastEscape = false;
    }
  }
  AddWatch(unescapedCode);
}

void Variablespane::AddWatch(const wxString &watch)
{
  BeginBatch();
  SetCellValue(GetNumberRows()-1,0,watch);
  wxGridEvent evt(wxID_ANY,wxEVT_GRID_CELL_CHANGED,this,GetNumberRows()-1,0);
  OnTextChange(evt);
  EndBatch();
}

wxString Variablespane::UnescapeVarname(const wxString &var)
{
  return var.StartsWith(wxT("$")) ? var.Right(var.Length()-1) : var;
}

wxString Variablespane::EscapeVarname(const wxString &v)
{
  wxString var = v;
  var.Replace("\\","\\\\");
  var.Replace("+","\\+");
  var.Replace("#","\\#");
  var.Replace("'","\\'");
  var.Replace("\"","\\\"");
  var.Replace("!","\\!");
  var.Replace("-","\\-");
  var.Replace("*","\\*");
  var.Replace("/","\\/");
  var.Replace("^","\\^");
  var.Replace("$","\\$");
  var.Replace(";","\\;");
  var.Replace(",","\\,");
  var.Replace("<","\\<");
  var.Replace(">","\\>");
  var.Replace("@","\\@");
  var.Replace("!","\\!");
  var.Replace("~","\\~");
  var.Replace("`","\\`");
  var.Replace("?","\\?");
  var.Replace("(","\\(");
  var.Replace(")","\\)");
  var.Replace("{","\\{");
  var.Replace("}","\\}");
  var.Replace("[","\\[");
  var.Replace("]","\\]");
  var.Replace(" ","\\ ");
  if(var.StartsWith("\\?"))
    var.Remove(0, 1);
  if(!var.StartsWith(wxT("?")))
    var.Prepend("$");
  return var;
}

bool Variablespane::IsValidVariable(const wxString &var)
{
  if (!std::all_of(var.begin(), var.end(), wxIsprint))
    return false;
  if (var.empty())
    return false;
  if ((var[0] >= '0') && (var[0] <= '9'))
    return false;
  if (std::find(var.begin(), var.end(), ':') != var.end())
    return false;
  return true;
}

void Variablespane::ResetValues()
{
  for(int i = 0; i < GetNumberRows(); i++)
  {
    if(!GetCellValue(i,0).empty())
    {
      SetCellTextColour(i,1,*wxLIGHT_GREY);
      SetCellValue(i,1,_("Undefined"));
      RefreshAttr(i,1);
    }
    else
      SetCellValue(i,1,wxT(""));
  }
}

void Variablespane::Clear()
{
  while(GetNumberRows() > 1)
    DeleteRows(0);    
}

Variablespane::~Variablespane()
{
}
