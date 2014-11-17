/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 *
 * Based on the information found on:
 * http://stackoverflow.com/questions/10270328/the-simplest-and-neatest-c11-scopeguard
 * http://stackoverflow.com/questions/1597007/creating-c-macro-with-and-line-token-concatenation-with-positioning-macr
 */
#ifndef UBUNTUSCOPEFINALIZER_H
#define UBUNTUSCOPEFINALIZER_H

#include <functional>

/*!
  \class ScopeFinalizer
  Defines a way to clean up a scope nicely when its exited
 */

template <typename CleanupFunc>
struct ScopeFinalizer
{
    inline ScopeFinalizer(){}
    inline ScopeFinalizer& operator<<(CleanupFunc &&lambda){
        m_lambda = std::move(lambda);
        return *this;
    }
    inline ~ScopeFinalizer(){
        m_lambda();
    }

    CleanupFunc m_lambda;
};

#define ScopeFinalizer_TOKENPASTE(x, y) x ## y
#define ScopeFinalizer_TOKENPASTE2(x, y) ScopeFinalizer_TOKENPASTE(x, y)
#define OnScopeExit ScopeFinalizer< std::function<void()> > ScopeFinalizer_TOKENPASTE2(OnScopeExit_,__LINE__); ScopeFinalizer_TOKENPASTE2(OnScopeExit_,__LINE__) << [&]()

#endif // UBUNTUSCOPEFINALIZER_H
