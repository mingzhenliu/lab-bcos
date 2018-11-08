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
 * @brief : external interface for the param of ledger
 * @file : LedgerParamInterface.h
 * @author: yujiechen
 * @date: 2018-10-23
 */
#pragma once
#include <libdevcrypto/Common.h>
#include <libethcore/Common.h>
#include <memory>
#include <vector>
namespace dev
{
namespace ledger
{
/// forward class declaration
struct TxPoolParam;
struct ConsensusParam;
struct BlockChainParam;
struct SyncParam;
struct P2pParam;
/// struct GenesisParam;
struct GenesisParam;
struct AMDBParam;
class LedgerParamInterface
{
public:
    LedgerParamInterface() = default;
    virtual ~LedgerParamInterface() {}
    virtual TxPoolParam& mutableTxPoolParam() = 0;
    virtual ConsensusParam& mutableConsensusParam() = 0;
    virtual SyncParam& mutableSyncParam() = 0;
    virtual GenesisParam& mutableGenesisParam() = 0;
    virtual AMDBParam& mutableAMDBParam() = 0;
    virtual std::string const& dbType() const = 0;
    virtual bool enableMpt() const = 0;
    virtual void setMptState(bool mptState) = 0;
    virtual void setDBType(std::string const& dbType) = 0;
    virtual std::string const& baseDir() const = 0;
    virtual void setBaseDir(std::string const& baseDir) = 0;

protected:
    virtual void initLedgerParams() = 0;
};
}  // namespace ledger
}  // namespace dev
