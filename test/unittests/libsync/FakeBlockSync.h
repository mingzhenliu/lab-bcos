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
 * @brief:
 * @file: FakeBlockSync.cpp
 * @author: yujiechen
 * @date: 2018-10-10
 */
#pragma once
#include <libethcore/Block.h>
#include <libsync/SyncInterface.h>
#include <libsync/SyncStatus.h>
#include <memory>
using namespace dev::sync;
using namespace dev::eth;

namespace dev
{
namespace test
{
/// simple fake of blocksync
class FakeBlockSync : public SyncInterface
{
public:
    void start() override {}
    void stop() override {}
    SyncStatus status() const override { return m_syncStatus; }
    bool isSyncing() const override { return m_isSyncing; }
    int16_t const& protocolId() const override { return m_protocolId; };
    void setProtocolId(int16_t const _protocolId) override { m_protocolId = _protocolId; };

private:
    SyncStatus m_syncStatus;
    bool m_isSyncing;
    bool m_forceSync;
    Block m_latestSentBlock;
    int16_t m_protocolId;
};
}  // namespace test
}  // namespace dev
