// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2020 Kuba Ober <kuba@bertec.com>
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
 * Implementation of a Cell pointer without the overhead of atomic reference counting.
 * Useful since cells aren't really thread-safe.
 */

#ifndef CELLPTR_H
#define CELLPTR_H

#include <wx/debug.h>
#include <memory>

//! Objects deriving from this class can be observed by the CellPtr.
//! This class is not directly copyable.
class Observed
{
  struct ControlBlock
  {
    //! Pointer to the object
    Observed *m_object = {};
    //! Number of observers for this object - the object is an observer for itself
    unsigned int m_refCount = 1;

    ControlBlock *Ref() { ++m_refCount; return this; }
    ControlBlock *Deref() {
      if (m_refCount)
        --m_refCount;
      if (m_refCount)
        return this;
      wxASSERT(!m_object);
      delete this;
      return nullptr;
    }
    explicit ControlBlock(Observed *object) : m_object(object) {}
    ~ControlBlock() { m_object = {}; }
    ControlBlock(const ControlBlock &) = delete;
    void operator=(const ControlBlock &) = delete;
  };

  ControlBlock *m_cb = new ControlBlock(this);
  Observed(const Observed &) = delete;
  void operator=(const Observed &) = delete;

protected:
  Observed() = default;
  virtual ~Observed()
  {
    m_cb->m_object = {};
    m_cb->Deref();
  }
  friend class CellPtr;
};

class Cell;

//! A weak non-owning pointer that becomes null whenever the observed
//! object is destroyed.
class CellPtr
{
  using ControlBlock = Observed::ControlBlock;
  static ControlBlock emptyCB;
  ControlBlock *m_cb = Ref(nullptr);
  ControlBlock *Ref(Observed *obj) { return obj ? obj->m_cb->Ref() : emptyCB.Ref(); }
public:
  CellPtr() = default;
  CellPtr(decltype(nullptr)) {}
  CellPtr(Observed *obj) : m_cb(Ref(obj)) {}
  CellPtr(const std::unique_ptr<Cell> &ptr);
  CellPtr(const CellPtr &o) : m_cb(o.m_cb->Ref()) {}
  ~CellPtr() { m_cb->Deref(); }
  explicit operator bool() const { return m_cb->m_object; }
  CellPtr &operator=(const CellPtr &o)
  {
    m_cb->Deref();
    m_cb = o.m_cb->Ref();
    return *this;
  }
  CellPtr &operator=(Observed *obj)
  {
    reset(obj);
    return *this;
  }
  void reset() { reset(nullptr); }
  void reset(Observed *obj)
  {
    m_cb->Deref();
    m_cb = Ref(obj);
  }
  //bool operator==(decltype(nullptr)) const { return m_cb->m_object; }
  bool operator==(const CellPtr &ptr) const { return m_cb == ptr.m_cb; }
  bool operator==(Observed *o) const { return m_cb->m_object == o; }
  bool operator!=(const CellPtr &ptr) const { return m_cb != ptr.m_cb; }
  bool operator!=(Observed *o) const { return m_cb->m_object != o; }
  bool operator<(const CellPtr &ptr) const { return m_cb->m_object < ptr.m_cb->m_object; }
  bool operator<(Observed *o) const { return m_cb->m_object < o; }
  inline Cell *get() const;
  inline Cell &operator*() const;
  inline Cell *operator->() const;
  inline operator Cell*() const;

  template <typename PtrT>
  PtrT CastAs() const { return dynamic_cast<PtrT>(m_cb->m_object); }
};

//! A pointer that owns the cell it points to
template <typename T>
using OwningPtr = std::unique_ptr<T>;

template <typename T,
         typename std::enable_if<std::is_base_of<Cell, T>::value, bool>::type = true,
         typename... Args>
[[nodiscard]] OwningPtr<T>
MakeOwned(Args&&... args)
{
  return OwningPtr<T>(new T(std::forward<Args>(args)...));
}

#endif // CELLPTR_H
