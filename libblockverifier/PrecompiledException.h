/*
    This file is part of FISCO-BCOS.

    FISCO-BCOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FISCO-BCOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file PrecompiledException.h
 *  @author mingzhenliu
 *  @date 20180921
 */
#pragma once
#include <exception>

namespace dev
{
namespace blockverifier
{
class FatalException : public std::exception
{
public:
    FatalException() _GLIBCXX_USE_NOEXCEPT {}

    virtual ~FatalException() _GLIBCXX_USE_NOEXCEPT;

    virtual const char* what() const _GLIBCXX_USE_NOEXCEPT;
};

}  // namespace blockverifier
}  // namespace dev
