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
 * @brief : Imp of blockchain
 * @author: mingzhenliu
 * @date: 2018-09-21
 */
#pragma once

#include "BlockChainInterface.h"
#include <libethcore/Block.h>
#include <libethcore/Common.h>
#include <libethcore/Transaction.h>
#include <libethcore/TransactionReceipt.h>
#include <libstorage/Common.h>
#include <libstorage/Storage.h>
namespace dev
{
namespace blockverifier
{
class ExecutiveContext;
}
namespace storage
{
class MemoryTableFactory;
}

namespace blockchain
{
class BlockChainImp : public BlockChainInterface
{
public:
    BlockChainImp(){};
    virtual ~BlockChainImp(){};
    int64_t number() override;
    dev::h256 numberHash(int64_t _i) override;
    dev::eth::Transaction getTxByHash(dev::h256 const& _txHash) override;
    dev::eth::LocalisedTransaction getLocalisedTxByHash(dev::h256 const& _txHash) override;
    dev::eth::TransactionReceipt getTransactionReceiptByHash(dev::h256 const& _txHash) override;
    std::shared_ptr<dev::eth::Block> getBlockByHash(dev::h256 const& _blockHash) override;
    std::shared_ptr<dev::eth::Block> getBlockByNumber(int64_t _i) override;
    CommitResult commitBlock(dev::eth::Block& block,
        std::shared_ptr<dev::blockverifier::ExecutiveContext> context) override;
    virtual void setStateStorage(dev::storage::Storage::Ptr stateStorage);
    virtual std::shared_ptr<dev::storage::MemoryTableFactory> getMemoryTableFactory();
    void setGroupMark(std::string const& groupMark) override {}

private:
    void writeNumber(const dev::eth::Block& block,
        std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    void writeTxToBlock(const dev::eth::Block& block,
        std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    void writeBlockInfo(
        dev::eth::Block& block, std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    void writeNumber2Hash(const dev::eth::Block& block,
        std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    void writeHash2Block(
        dev::eth::Block& block, std::shared_ptr<dev::blockverifier::ExecutiveContext> context);
    dev::storage::Storage::Ptr m_stateStorage;
    std::mutex commitMutex;
    const std::string c_genesisHash =
        "0xeb8b84af3f35165d52cb41abe1a9a3d684703aca4966ce720ecd940bd885517c";
};
}  // namespace blockchain
}  // namespace dev
