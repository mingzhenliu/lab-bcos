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
 * @brief : The implementation of callback from p2p
 * @author: jimmyshi
 * @date: 2018-10-17
 */
#include "SyncMsgEngine.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::sync;
using namespace dev::p2p;
using namespace dev::blockchain;
using namespace dev::txpool;

void SyncMsgEngine::messageHandler(
    NetworkException _e, std::shared_ptr<dev::p2p::P2PSession> _session, P2PMessage::Ptr _msg)
{
    SYNCLOG(TRACE) << "[Rcv] [Packet] Receive packet from: " << _session->nodeID() << std::endl;
    if (!checkSession(_session) || !checkMessage(_msg))
    {
        SYNCLOG(WARNING) << "[Rcv] [Packet] Reject packet: [reason]: session or msg illegal"
                         << endl;
        _session->stop(LocalIdentity);
        return;
    }

    SyncMsgPacket packet;
    if (!packet.decode(_session, _msg))
    {
        SYNCLOG(WARNING)
            << "[Rcv] [Packet] Reject packet: [reason/nodeId/size/message]: decode failed/"
            << _session->nodeID() << "/" << _msg->buffer()->size() << "/" << toHex(*_msg->buffer())
            << endl;
        _session->stop(BadProtocol);
        return;
    }

    bool ok = interpret(packet);
    if (!ok)
        SYNCLOG(WARNING)
            << "[Rcv] [Packet] Reject packet: [reason/packetType]: illegal packet type/"
            << int(packet.packetType) << endl;
}

bool SyncMsgEngine::checkSession(std::shared_ptr<dev::p2p::P2PSession> _session)
{
    /// TODO: denine LocalIdentity after SyncPeer finished
    if (_session->nodeID() == m_nodeId)
        return false;
    return true;
}

bool SyncMsgEngine::checkMessage(P2PMessage::Ptr _msg)
{
    bytesConstRef msgBytes = ref(*_msg->buffer());
    if (msgBytes.size() < 2 || msgBytes[0] > 0x7f)
        return false;
    if (RLP(msgBytes.cropped(1)).actualSize() + 1 != msgBytes.size())
        return false;
    return true;
}

bool SyncMsgEngine::interpret(SyncMsgPacket const& _packet)
{
    SYNCLOG(TRACE) << "[Rcv] [Packet] interpret packet type: " << int(_packet.packetType) << endl;
    try
    {
        switch (_packet.packetType)
        {
        case StatusPacket:
            onPeerStatus(_packet);
            break;
        case TransactionsPacket:
            onPeerTransactions(_packet);
            break;
        case BlocksPacket:
            onPeerBlocks(_packet);
            break;
        case ReqBlocskPacket:
            onPeerRequestBlocks(_packet);
            break;
        default:
            return false;
        }
    }
    catch (std::exception& e)
    {
        SYNCLOG(WARNING) << "[Rcv] [Packet] Interpret error for " << e.what() << endl;
        return false;
    }
    return true;
}

void SyncMsgEngine::onPeerStatus(SyncMsgPacket const& _packet)
{
    shared_ptr<SyncPeerStatus> status = m_syncStatus->peerStatus(_packet.nodeId);

    RLP const& rlps = _packet.rlp();

    if (rlps.itemCount() != 3)
    {
        SYNCLOG(TRACE) << "[Rcv] [Status] Invalid status packet format. From" << _packet.nodeId
                       << endl;
        return;
    }

    SyncPeerInfo info{
        _packet.nodeId, rlps[0].toInt<int64_t>(), rlps[1].toHash<h256>(), rlps[2].toHash<h256>()};

    if (status == nullptr)
    {
        SYNCLOG(TRACE) << "[Rcv] [Status] Peer status new " << info.nodeId << endl;
        m_syncStatus->newSyncPeerStatus(info);
    }
    else
    {
        SYNCLOG(TRACE) << "[Rcv] [Status] Peer status update " << info.nodeId << endl;
        status->update(info);
    }
}

void SyncMsgEngine::onPeerTransactions(SyncMsgPacket const& _packet)
{
    if (m_syncStatus->state == SyncState::Downloading)
    {
        SYNCLOG(TRACE) << "[Rcv] [Tx] Transaction dropped when downloading blocks [fromNodeId]: "
                       << _packet.nodeId << endl;
        return;
    }

    RLP const& rlps = _packet.rlp();
    unsigned itemCount = rlps.itemCount();

    size_t successCnt = 0;

    for (unsigned i = 0; i < itemCount; ++i)
    {
        Transaction tx;
        tx.decode(rlps[i]);

        auto importResult = m_txPool->import(tx);
        if (ImportResult::Success == importResult)
            successCnt++;
        else
            SYNCLOG(TRACE) << "[Rcv] [Tx] Transaction import into txPool FAILED from peer "
                              "[reason/txHash/peer]: "
                           << int(importResult) << "/" << _packet.nodeId << "/" << move(tx.sha3())
                           << endl;


        m_txPool->transactionIsKnownBy(tx.sha3(), _packet.nodeId);
    }
    SYNCLOG(TRACE) << "[Rcv] [Tx] Peer transactions import [import/rcv/txPool]: " << successCnt
                   << "/" << itemCount << "/" << m_txPool->pendingSize() << " from "
                   << _packet.nodeId << endl;
}

void SyncMsgEngine::onPeerBlocks(SyncMsgPacket const& _packet)
{
    RLP const& rlps = _packet.rlp();

    SYNCLOG(TRACE) << "[Rcv] [Download] Peer block packet received [packetSize]: "
                   << rlps.data().size() << "B" << endl;

    m_syncStatus->bq().push(rlps);
}

void SyncMsgEngine::onPeerRequestBlocks(SyncMsgPacket const& _packet)
{
    RLP const& rlp = _packet.rlp();

    if (rlp.itemCount() != 2)
    {
        SYNCLOG(TRACE) << "[Rcv] [Send] [Download] Invalid request blocks packet format. From"
                       << _packet.nodeId << endl;
        return;
    }

    // request
    int64_t from = rlp[0].toInt<int64_t>();
    unsigned size = rlp[1].toInt<unsigned>();

    SYNCLOG(TRACE) << "[Rcv] [Send] [Download] Block request from " << _packet.nodeId << " req["
                   << from << ", " << from + size - 1 << "]" << endl;

    // fetch block into downloading blocks container
    DownloadBlocksContainer blockContainer(m_service, m_protocolId, from);
    for (int64_t number = from; number < from + size; ++number)
    {
        shared_ptr<Block> block = m_blockChain->getBlockByNumber(number);
        if (!block)
        {
            SYNCLOG(TRACE)
                << "[Rcv] [Send] [Download] Get block for node failed [reason/number/nodeId]: "
                << "block is null/" << number << "/" << _packet.nodeId << endl;
            break;
        }
        else if (block->header().number() != number)
        {
            SYNCLOG(TRACE)
                << "[Rcv] [Send] [Download] Get block for node failed [reason/number/nodeId]: "
                << number << "number incorrect /" << _packet.nodeId << endl;
            break;
        }

        blockContainer.push(block);
    }

    // send it
    blockContainer.send(_packet.nodeId);
}

void DownloadBlocksContainer::push(BlockPtr _block)
{
    bytes blockRLP = _block->rlp();
    if ((m_currentShardSize + blockRLP.size()) > c_maxPayload &&
        0 != m_blockRLPShards.back().size())
    {
        m_blockRLPShards.emplace_back(vector<bytes>());
        m_currentShardSize = 0;
    }
    m_blockRLPShards.back().emplace_back(blockRLP);
    m_currentShardSize += blockRLP.size();

    // Note that: if _block->rlp().size() > c_maxPayload
    // We also send it as a packet
}

void DownloadBlocksContainer::send(NodeID _nodeId)
{
    if (0 == m_blockRLPShards.size())
    {
        SYNCLOG(TRACE) << "[Rcv] [Send] [Download] Block back to " << _nodeId << " back[null]"
                       << endl;
        return;
    }

    int64_t numberOffset = 0;
    for (vector<bytes> const& shard : m_blockRLPShards)
    {
        if (0 == shard.size())
            continue;
        SyncBlocksPacket retPacket;
        retPacket.encode(shard);

        auto msg = retPacket.toMessage(m_protocolId);
        m_service->asyncSendMessageByNodeID(_nodeId, msg, CallbackFuncWithSession(), Options());
        SYNCLOG(TRACE) << "[Rcv] [Send] [Download] Block back to " << _nodeId << " back["
                       << m_startBlockNumber + numberOffset << ", "
                       << m_startBlockNumber + numberOffset + shard.size() - 1 << "/ "
                       << msg->buffer()->size() << "B]" << endl;

        numberOffset += shard.size();
    }
}
