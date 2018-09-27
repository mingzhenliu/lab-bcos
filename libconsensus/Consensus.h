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
 * @brief : implementation of consensus
 * @file: Consensus.h
 * @author: yujiechen
 * @date: 2018-09-27
 */
#pragma once
#include "ConsensusInterface.h"
#include <libblockmanager/BlockManagerInterface.h>
#include <libblocksync/SyncInterface.h>
#include <libdevcore/Worker.h>
#include <libethcore/Block.h>
#include <libethcore/Protocol.h>
#include <libp2p/Service.h>
#include <libtxpool/TxPool.h>

namespace dev
{
class ConsensusStatus;
namespace consensus
{
class Consensus : public Worker, virtual ConsensusInterface
{
public:
    Consensus() = default;
    Consensus(std::shared_ptr<dev::p2p::Service>& _service,
        std::shared_ptr<dev::txpool::TxPool>& _txPool,
        std::shared_ptr<dev::sync::SyncInterface> _blockSync,
        std::shared_ptr<dev::blockmanager::BlockManagerInterface> _blockManager,
        int16_t const& _protocolId, h512s const& _minerList = h512s())
      : Worker("consensus", 0),
        m_service(_service),
        m_txPool(_txPool),
        m_blockSync(_blockSync),
        m_blockManager(_blockManager),
        m_protocolId(_protocolId),
        m_minerList(_minerList)
    {
        assert(m_service && m_txPool && m_blockSync);
        if (m_protocolId == 0)
            BOOST_THROW_EXCEPTION(
                InvalidProtocolID() << errinfo_comment("Protocol id must be larger than 0"));
    }

    virtual ~Consensus() { stop(); }
    /// start the consensus module
    void start() override;
    /// stop the consensus module
    void stop() override;

    /// get miner list
    h512s minerList() const override { return m_minerList; }
    /// set the miner list
    void setMinerList(h512s const& _minerList) override { m_minerList = _minerList; }
    /// append miner
    void appendMiner(h512 const& _miner) override { m_minerList.push_back(_miner); }

    /// get status of consensus
    ConsensusStatus consensusStatus() const override { return ConsensusStatus(); }

    /// protocol id used when register handler to p2p module
    int16_t const& getProtocolId() const { return m_protocolId; }
    void setMaxBlockTransactions(uint64_t const& _maxBlockTransactions)
    {
        m_maxBlockTransactions = _maxBlockTransactions;
    }

    uint64_t getMaxBlockTransactions() const { return m_maxBlockTransactions; }

    /// get account type
    NodeAccountType getNodeAccountType() override { return m_accountType; }
    /// set the node account type
    void setNodeAccountType(dev::consensus::NodeAccountType const& _accountType) override
    {
        m_accountType = _accountType;
    }
    u256 nodeIdx() const override { return m_idx; }
    virtual void setNodeIdx(u256 const& _idx) override { m_idx = _idx; }
    void setExtraData(std::vector<bytes> const& _extra) { m_extraData = _extra; }
    std::vector<bytes> const& extraData() const { return m_extraData; }

protected:
    /// sealing block
    virtual void rejigSealing(){};
    virtual bool shouldSeal() { return false; }
    /// for pbft
    virtual void updatePackedTxNum(uint64_t& tx_num) {}
    /// for pbft
    virtual bool shouldWaitForNextInterval() { return false; }
    /// fetch transactions from transaction pool
    virtual bool generateSeal(bytes& _block_data);
    virtual void doWork(bool wait);

    /// sync transactions from txPool
    void syncTxPool(uint64_t const& maxBlockTransactions);
    void doWork() override { doWork(true); }
    bool isBlockSyncing();

private:
    /// reset timestamp of block header
    void inline resetCurrentTime()
    {
        uint64_t parent_time = m_blockManager->getLatestBlockHeader().timestamp();
        m_sealingHeader.setTimestamp(max(parent_time + 1, utcTime()));
    }
    void inline submitToEncode();

private:
    std::shared_ptr<dev::p2p::Service> m_service;
    std::shared_ptr<dev::txpool::TxPool> m_txPool;
    std::shared_ptr<dev::sync::SyncInterface> m_blockSync;
    std::shared_ptr<dev::blockmanager::BlockManagerInterface> m_blockManager;
    int16_t m_protocolId;
    h512s m_minerList;
    uint64_t m_maxBlockTransactions = 1000;

    /// current sealing block
    Block m_sealing;
    BlockHeader m_sealingHeader;
    /// lock on m_sealing
    mutable SharedMutex x_sealing;
    /// atomic value represents that whether is calling syncTransactionQueue now
    std::atomic<bool> m_syncTxPool = {false};
    /// signal to notify all thread to work
    std::condition_variable m_signalled;
    /// mutex to access m_signalled
    Mutex x_signalled;
    ///< Has the remote worker recently been reset?
    bool m_remoteWorking = false;
    /// True if we /should/ be sealing.
    bool m_startSeal = false;

    std::vector<bytes> m_extraData;

    NodeAccountType m_accountType;
    u256 m_idx = 0;
};
}  // namespace consensus
}  // namespace dev
