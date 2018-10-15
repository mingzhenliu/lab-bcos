/*
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 */

/**
 * @brief : common functions and types of Blocksync modules
 * @author: yujiechen
 * @date: 2018-09-21
 */

/**
 * @brief : common functions and types of Blocksync modules
 * @author: jimmyshi
 * @date: 2018-10-15
 */
#pragma once
#include <libdevcore/Exceptions.h>
#include <libp2p/Common.h>
#include <set>
namespace dev
{
namespace sync
{
using NodeList = std::set<dev::p2p::NodeID>;

enum class SyncState
{
    NotSynced,  ///< Initial chain sync has not started yet
    Idle,       ///< Initial chain sync complete. Waiting for new packets
    Waiting,    ///< Block downloading paused. Waiting for block queue to process blocks and free
                ///< space
    Blocks,     ///< Downloading blocks
    State,      ///< Downloading state
    NewBlocks,  ///< Downloading blocks learned from NewHashes packet
    Size        /// Must be kept last
};
struct SyncStatus
{
    SyncState state = SyncState::Idle;
    int16_t protocolId;
    unsigned startBlockNumber;
    unsigned currentBlockNumber;
    unsigned highestBlockNumber;
    bool majorSyncing = false;
};

}  // namespace sync
}  // namespace dev
