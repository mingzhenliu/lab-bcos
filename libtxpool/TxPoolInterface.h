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
 * @brief : interface of transaction pool
 * @file : TxPoolInterface.h
 * @author: yujiechen
 * @date: 2018-09-21
 */
#pragma once
#include <libethcore/Transaction.h>
namespace dev
{
namespace txpool
{
class TxPoolStatus;
class TxPoolInterface
{
public:
    TxPoolInterface() = default;
    virtual ~TxPoolInterface() = 0;

    /**
     * @brief Remove transaction from the queue
     * @param _txHash: transaction hash
     */
    virtual void drop(h256 const& _txHash) = 0;
    virtual void dropGood(Transaction const& _t) = 0;
    /**
     * @brief Get top transactions from the queue
     *
     * @param _limit : _limit Max number of transactions to return.
     * @param _avoid : Transactions to avoid returning.
     * @return Transactions : up to _limit transactions
     */
    virtual Transactions topTransactions(
        unsigned _limit, h256Hash const& _avoid = h256Hash()) const = 0;

    /// get all current transactions(maybe blocksync module need this interface)
    virtual Transactions pendingList() const = 0;
    /// get current transaction num
    virtual size_t pendingSize() = 0;

    /**
     * @brief submit a transaction through RPC
     * @param _t : transaction
     * @return std::pair<h256, Address>: maps from transaction hash to contract address
     */
    virtual std::pair<h256, Address> submit(Transaction const& _tx) = 0;

    /**
     * @brief : submit a transaction through p2p, Verify and add transaction to the queue
     * synchronously.
     * @param _tx : Trasaction data.
     * @param _ik : Set to Retry to force re-addinga transaction that was previously dropped.
     * @return ImportResult : Import result code.
     */
    virtual ImportResult import(Transaction const& _tx, IfDropped _ik = IfDropped::Ignore);

    /// @returns the status of the transaction queue.
    virtual TxPoolStatus status() const = 0;

    /// protocol id used when register handler to p2p module
    virtual const int32_t getProtocolId() const = 0;
    virtual void setProtocolId(uint32_t const _protocolId) = 0;
};
}  // namespace txpool
}  // namespace dev
