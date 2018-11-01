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
 * @brief : implementation of Ledger manager
 * @file: LedgerManager.h
 * @author: yujiechen
 * @date: 2018-10-23
 */
#pragma once
#include "Ledger.h"
#include "LedgerInterface.h"
#include <libethcore/Protocol.h>
#include <map>
#define LedgerManager_LOG(LEVEL) LOG(LEVEL) << "[" << utcTime() << "] [LEDGERMANAGER] "
namespace dev
{
namespace ledger
{
class LedgerManager
{
public:
    /**
     * @brief: constructor of LedgerManager
     * @param _service: p2p service handler used to send/receive messages
     * @param _keyPair: the keyPair used to init consensus module
     * @param _preCompile: map that stores the PrecompiledContract (required by blockverifier)
     */
    LedgerManager(std::shared_ptr<dev::p2p::P2PInterface> _service, dev::KeyPair keyPair)
      : m_service(_service), m_keyPair(keyPair)
    {
        assert(m_service);
    }

    /**
     * @brief : init a single ledger with the given params
     *
     * @param _groupId : the groupId of the ledger need to be inited
     * @param _baseDir: baseDir used to place the data of the group
     * @param configFileName: the configuration file path of the group to be inited
     * @return true: init single ledger succeed
     * @return false: init single ledger failed
     */
    template <class T>
    inline bool initSingleLedger(dev::GROUP_ID const& _groupId,
        std::string const& _baseDir = "data", std::string const& configFileName = "")
    {
        if (m_ledgerMap.count(_groupId) == 0)
        {
            std::shared_ptr<LedgerInterface> ledger =
                std::make_shared<T>(m_service, _groupId, m_keyPair, _baseDir, configFileName);
            LedgerManager_LOG(INFO)
                << "[initSingleLedger] [GroupId]:  " << std::to_string(_groupId) << std::endl;
            ledger->initLedger();
            m_ledgerMap.insert(std::make_pair(_groupId, ledger));
            return true;
        }
        else
        {
            LedgerManager_LOG(WARNING) << "[initSingleLedger] Group already inited [GroupId]:  "
                                       << std::to_string(_groupId) << std::endl;
            return false;
        }
    }

    /**
     * @brief : start a single ledger by groupId
     * @param groupId : the ledger need to be started
     * @return true : start success
     * @return false : start failed (maybe the ledger doesn't exist)
     */
    virtual inline bool startByGroupID(dev::GROUP_ID const& groupId)
    {
        if (!m_ledgerMap.count(groupId))
            return false;
        m_ledgerMap[groupId]->startAll();
        return true;
    }

    /**
     * @brief: stop the ledger by group id
     * @param groupId: the groupId of the ledger need to be stopped
     * @return true: stop the ledger succeed
     * @return false: stop the ledger failed
     */
    virtual inline bool stopByGroupID(dev::GROUP_ID const& groupId)
    {
        if (!m_ledgerMap.count(groupId))
            return false;
        m_ledgerMap[groupId]->stopAll();
        return true;
    }

    /// start all the ledgers that have been created
    virtual inline void startAll()
    {
        for (auto item : m_ledgerMap)
            item.second->startAll();
    }
    /// stop all the ledgers that have been started
    virtual inline void stopAll()
    {
        for (auto item : m_ledgerMap)
            item.second->stopAll();
    }
    /// get pointer of txPool by group id
    inline std::shared_ptr<dev::txpool::TxPoolInterface> txPool(dev::GROUP_ID const& groupId)
    {
        if (!m_ledgerMap.count(groupId))
            return nullptr;
        return m_ledgerMap[groupId]->txPool();
    }

    /// get pointer of blockverifier by group id
    inline std::shared_ptr<dev::blockverifier::BlockVerifierInterface> blockVerifier(
        dev::GROUP_ID const& groupId)
    {
        if (!m_ledgerMap.count(groupId))
            return nullptr;
        return m_ledgerMap[groupId]->blockVerifier();
    }
    /// get pointer of blockchain by group id
    inline std::shared_ptr<dev::blockchain::BlockChainInterface> blockChain(
        dev::GROUP_ID const& groupId)
    {
        if (!m_ledgerMap.count(groupId))
            return nullptr;
        return m_ledgerMap[groupId]->blockChain();
    }
    /// get pointer of consensus by group id
    inline std::shared_ptr<dev::consensus::ConsensusInterface> consensus(
        dev::GROUP_ID const& groupId)
    {
        if (!m_ledgerMap.count(groupId))
            return nullptr;
        return m_ledgerMap[groupId]->consensus();
    }
    /// get pointer of blocksync by group id
    inline std::shared_ptr<dev::sync::SyncInterface> sync(dev::GROUP_ID const& groupId)
    {
        if (!m_ledgerMap.count(groupId))
            return nullptr;
        return m_ledgerMap[groupId]->sync();
    }
    /// get ledger params by group id
    inline std::shared_ptr<LedgerParamInterface> getParamByGroupId(dev::GROUP_ID const& groupId)
    {
        if (!m_ledgerMap.count(groupId))
            return nullptr;
        return m_ledgerMap[groupId]->getParam();
    }

private:
    /// map used to store the mappings between groupId and created ledger objects
    std::map<dev::GROUP_ID, std::shared_ptr<LedgerInterface>> m_ledgerMap;
    /// p2p service shared by all the ledgers
    std::shared_ptr<dev::p2p::P2PInterface> m_service;
    /// keyPair shared by all the ledgers
    dev::KeyPair m_keyPair;
};
}  // namespace ledger
}  // namespace dev
