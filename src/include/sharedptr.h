/*  C++ shared_ptr
 *
 *  (c) Copyright 2014 Yann Diorcet <diorcet.yann@gmail.com>
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __sharedptr_h__
#define __sharedptr_h__

#include <memory>

template <class T>
using SharedPtr = std::shared_ptr<T>;
template <class T>
using WeakPtr = std::weak_ptr<T>;
template <class T>
using ConstSharedPtr = std::shared_ptr<const T>;
template <class T>
using ConstWeakPtr = std::weak_ptr<const T>;
template <class T>
using EnableSharedFromThis = std::enable_shared_from_this<T>;

#define MakeShared std::make_shared
#define SharedFromThis shared_from_this
#define DynamicPointerCast std::dynamic_pointer_cast
#define StaticPointerCast std::static_pointer_cast

#define FREEJ_FORWARD_PTR(x) class x; \
    typedef SharedPtr<x> x ## Ptr; \
    typedef WeakPtr<x> x ## WeakPtr; \
    typedef ConstSharedPtr<x> x ## ConstPtr; \
    typedef ConstWeakPtr<x> x ## ConstWeakPtr;

#endif //__shared_ptr_h__
