/**
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
 *
 * @brief: empty framework for main of consensus
 *
 * @file: evm_main.cpp
 * @author: yujiechen
 * @date 2018-10-29
 */
#include "EvmParams.h"
#include <libdevcore/Common.h>
#include <libethcore/ABI.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/Transaction.h>
#include <libevm/ExtVMFace.h>
#include <libexecutive/Executive.h>
#include <libexecutive/StateFace.h>
#include <libinitializer/Fake.h>
#include <libmptstate/MPTState.h>
INITIALIZE_EASYLOGGINGPP
using namespace dev;
using namespace dev::eth;
using namespace dev::executive;
using namespace dev::mptstate;
using namespace dev::blockchain;

static void FakeBlockHeader(BlockHeader& header, EvmParams const& param)
{
    header.setGasLimit(param.gasLimit());
    header.setNumber(param.blockNumber());
    header.setTimestamp(utcTime());
}

static void ExecuteTransaction(
    ExecutionResult& res, MPTState& mptState, EnvInfo& info, Transaction const& tx)
{
    Executive executive(mptState, info, 0);
    executive.setResultRecipient(res);
    executive.initialize(tx);
    /// execute transaction
    if (!executive.execute())
    {
        /// Timer timer;
        executive.go();
        /// double execTime = timer.elapsed();
    }
    executive.finalize();
}

static void updateSender(MPTState& mptState, Transaction& tx, EvmParams const& param)
{
    KeyPair key_pair = KeyPair::create();
    Address sender = toAddress(key_pair.pub());
    tx.forceSender(sender);
    mptState.addBalance(sender, param.transValue());
}

/// deploy contract
static Address deployContract(
    MPTState& mptState, EnvInfo& info, bytes const& code, EvmParams const& param)
{
    /// LOG(DEBUG) << "[evm_main] codeData: " << toHex(code);
    Transaction tx = Transaction(param.transValue(), param.gasPrice(), param.gas(), code, u256(0));
    updateSender(mptState, tx, param);
    ExecutionResult res;
    ExecuteTransaction(res, mptState, info, tx);
    EVMC_LOG(INFO) << "[evm_main/newAddress]:" << toHex(res.newAddress) << std::endl;
    EVMC_LOG(INFO) << "[evm_main/depositSize]:" << res.depositSize << std::endl;
}

static void updateMptState(MPTState& mptState, Input& input)
{
    Account account(u256(0), u256(0));
    account.setCode(bytes{input.codeData});
    AccountMap map;
    map[input.addr] = account;
    mptState.getState().populateFrom(map);
}

/// call contract
static void callTransaction(MPTState& mptState, EnvInfo& info, Input& input, EvmParams const& param)
{
    if (input.codeData.size() > 0)
    {
        KeyPair key_pair = KeyPair::create();
        input.addr = toAddress(key_pair.pub());
        updateMptState(mptState, input);
    }
    ContractABI abi;
    bytes inputData = abi.abiIn(input.inputCall);
    EVMC_LOG(INFO) << "[evm_main/callTransaction/Parms]: " << toHex(inputData) << std::endl;
    Transaction tx = Transaction(
        param.transValue(), param.gasPrice(), param.gas(), input.addr, inputData, u256(0));
    updateSender(mptState, tx, param);
    ExecutionResult res;
    ExecuteTransaction(res, mptState, info, tx);
    bytes output = std::move(res.output);
    std::string result;
    abi.abiOut(ref(output), result);
    EVMC_LOG(INFO) << "[evm_main/callTransaction/output]: " << toHex(output) << std::endl;
    EVMC_LOG(INFO) << "[evm_main/callTransaction/result string]: " << result << std::endl;
}

int main(int argc, const char* argv[])
{
    /// init configuration
    ptree pt;
    read_ini("config.ini", pt);
    EvmParams param(pt);
    /// fake blockHeader
    BlockHeader header;
    FakeBlockHeader(header, param);
    /// Fake envInfo
    std::shared_ptr<FakeBlockChain> blockChain = std::make_shared<FakeBlockChain>();
    EnvInfo envInfo(header, boost::bind(&FakeBlockChain::numberHash, blockChain, _1), u256(0));
    /// init state
    MPTState mptState(u256(0), MPTState::openDB("./", sha3("0x1234")), BaseState::Empty);
    /// test deploy
    for (size_t i = 0; i < param.code().size(); i++)
    {
        EVMC_LOG(INFO) << "=======[evm_main/BEGIN deploy Contract/index]:" << i
                       << "=======" << std::endl;
        deployContract(mptState, envInfo, param.code()[i], param);
    }
    /// test callfunctions
    for (size_t i = 0; i < param.input().size(); i++)
    {
        EVMC_LOG(INFO) << "=======[evm_main/BEGIN call transaction/index]:" << i
                       << "=======" << std::endl;
        callTransaction(mptState, envInfo, param.input()[i], param);
    }
    return 0;
}
