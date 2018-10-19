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
 * @brief : Sync packet decode and encode
 * @author: jimmyshi
 * @date: 2018-10-18
 */

#pragma once
#include "Common.h"
#include "SyncMsgPacket.h"
#include <libdevcore/RLP.h>
#include <libp2p/Common.h>
#include <libp2p/Session.h>

namespace dev
{
namespace sync
{
class SyncMsgPacket
{
public:
    SyncMsgPacket() {}

    SyncPacketType packetType;
    NodeID nodeId;

    /// Extract data by decoding the message
    bool decode(std::shared_ptr<dev::p2p::Session> _session, dev::p2p::Message::Ptr _msg);

    /// encode is implement in derived class
    /// basic encode function
    RLPStream& prep(RLPStream& _s, unsigned _id, unsigned _args);

    /// Generate p2p message after encode
    dev::p2p::Message::Ptr toMessage(uint16_t _protocolId);

    RLP const& rlp() const { return m_rlp; }

protected:
    RLP m_rlp;              /// The result of decode
    RLPStream m_rlpStream;  // The result of encode

private:
    bool checkPacket(bytesConstRef _msg);
};


class SyncStatusPacket : public SyncMsgPacket
{
public:
    SyncStatusPacket() { packetType = StatusPacket; }
    void encode(h256 const& _latestHash, h256 const& _genesisHash, int64_t _number);
};

class SyncTransactionsPacket : public SyncMsgPacket
{
public:
    SyncTransactionsPacket() { packetType = TransactionsPacket; }
    void encode(unsigned txNumber, bytes const& txRLPs);
};

}  // namespace sync
}  // namespace dev