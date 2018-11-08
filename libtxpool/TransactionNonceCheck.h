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
/**
 * @file: TransactionNonceCheck.h
 * @author: toxotguo
 *
 * @date: 2017
 */

#pragma once
#include "CommonTransactionNonceCheck.h"
#include <libblockchain/BlockChainInterface.h>
#include <boost/timer.hpp>
#include <thread>

using namespace std;
using namespace dev::eth;
using namespace dev::blockchain;

#define NONCECHECKER_LOG(LEVEL) \
    LOG(LEVEL) << "[#TransactionNonceCheckER] [PROTOCOL: " << m_protocolId << "] "

namespace dev
{
namespace txpool
{
class TransactionNonceCheck : public CommonTransactionNonceCheck
{
public:
    TransactionNonceCheck(std::shared_ptr<dev::blockchain::BlockChainInterface> const& _blockChain,
        dev::PROTOCOL_ID const& protocolId)
      : CommonTransactionNonceCheck(protocolId), m_blockChain(_blockChain)
    {
        init();
    }
    ~TransactionNonceCheck() {}
    void init();
    bool ok(dev::eth::Transaction const& _transaction, bool _needinsert = false);
    void updateCache(bool _rebuild = false);
    unsigned const& maxBlockLimit() const { return m_maxBlockLimit; }
    void setBlockLimit(unsigned const& limit) { m_maxBlockLimit = limit; }

private:
    bool isBlockLimitOk(dev::eth::Transaction const& _trans);

private:
    std::shared_ptr<dev::blockchain::BlockChainInterface> m_blockChain;
    unsigned m_startblk;
    unsigned m_endblk;
    unsigned m_maxBlockLimit = 1000;
};
}  // namespace txpool
}  // namespace dev
