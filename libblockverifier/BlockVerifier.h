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
/** @file BlockVerifier.h
 *  @author mingzhenliu
 *  @date 20180921
 */
#pragma once

#include "BlockVerifierInterface.h"
#include "ExecutiveContext.h"
#include "ExecutiveContextFactory.h"
#include "Precompiled.h"
#include <libdevcore/FixedHash.h>
#include <libdevcore/easylog.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Block.h>
#include <libethcore/Transaction.h>
#include <libethcore/TransactionReceipt.h>
#include <libevm/ExtVMFace.h>
#include <libexecutive/ExecutionResult.h>
#include <libmptstate/State.h>
#include <boost/function.hpp>
#include <memory>
namespace dev
{
namespace eth
{
class PrecompiledContract;
class TransactionReceipt;
class LastBlockHashesFace;

}  // namespace eth

namespace executive
{
class ExecutionResult;
}

namespace blockverifier
{
class BlockVerifier : public BlockVerifierInterface,
                      public std::enable_shared_from_this<BlockVerifier>
{
public:
    typedef std::shared_ptr<BlockVerifier> Ptr;
    typedef boost::function<dev::h256(int64_t x)> NumberHashCallBackFunction;
    BlockVerifier(){};

    virtual ~BlockVerifier(){};

    ExecutiveContext::Ptr executeBlock(dev::eth::Block& block, h256 const& parentStateRoot);

    std::pair<dev::executive::ExecutionResult, dev::eth::TransactionReceipt> executeTransaction(
        const dev::eth::BlockHeader& blockHeader, dev::eth::Transaction const& _t);

    std::pair<dev::executive::ExecutionResult, dev::eth::TransactionReceipt> execute(
        dev::eth::EnvInfo const& _envInfo, dev::eth::Transaction const& _t,
        dev::eth::OnOpFunc const& _onOp,
        dev::blockverifier::ExecutiveContext::Ptr executiveContext);


    void setExecutiveContextFactory(ExecutiveContextFactory::Ptr executiveContextFactory)
    {
        m_executiveContextFactory = executiveContextFactory;
    }
    ExecutiveContextFactory::Ptr getExecutiveContextFactory() { return m_executiveContextFactory; }
    void setNumberHash(const NumberHashCallBackFunction& _pNumberHash)
    {
        m_pNumberHash = _pNumberHash;
    }

private:
    ExecutiveContextFactory::Ptr m_executiveContextFactory;
    NumberHashCallBackFunction m_pNumberHash;
};

}  // namespace blockverifier

}  // namespace dev
