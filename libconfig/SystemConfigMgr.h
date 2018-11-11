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
/** @file SystemConfigMgr.h
 *  @author yujiechen
 *  @date 2018-11-08
 */
#pragma once
#include <libdevcore/Common.h>
namespace dev
{
namespace config
{
class SystemConfigMgr
{
public:
    /// default block time
    static const unsigned c_intervalBlockTime = 1000;
    /// omit empty block or not
    static const bool c_omitEmptyBlock = true;
    /// default blockLimit
    static const unsigned c_blockLimit = 1000;

    static const int64_t maxTransactionGasLimit = 0x7fffffffffffffff;
    static const int64_t maxBlockGasLimit = 0x7fffffffffffffff;
};
}  // namespace config
}  // namespace dev