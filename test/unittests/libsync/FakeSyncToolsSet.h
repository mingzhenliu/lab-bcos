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
 * @brief : fake sync tools set
 * @author: catli
 * @date: 2018-10-25
 */

#pragma once
#include <test/unittests/libethcore/FakeBlock.h>
#include <test/unittests/libp2p/FakeHost.h>
#include <test/unittests/libtxpool/FakeBlockChain.h>
#include <memory>

namespace dev
{
namespace test
{
class FakeSyncToolsSet
{
    using TxPoolPtr = std::shared_ptr<dev::txpool::TxPoolInterface>;
    using ServicePtr = std::shared_ptr<dev::p2p::Service>;
    using BlockChainPtr = std::shared_ptr<dev::blockchain::BlockChainInterface>;
    using SessionPtr = std::shared_ptr<dev::p2p::SessionFace>;
    using TransactionPtr = std::shared_ptr<dev::eth::Transaction>;

public:
    FakeSyncToolsSet(uint64_t _blockNum = 5, size_t const& _transSize = 5)
      : m_txPoolFixture(_blockNum, _transSize), m_fakeBlock()
    {
        //m_host = createFakeHost(m_clientVersion, m_listenIp, m_listenPort);
    }

    TxPoolPtr getTxPoolPtr() const { return m_txPoolFixture.m_txPool; }

    ServicePtr getServicePtr() const { return m_txPoolFixture.m_topicService; }

    BlockChainPtr getBlockChainPtr() const { return m_txPoolFixture.m_blockChain; }

    Block& getBlock() { return m_fakeBlock.getBlock(); }

    P2PSession::Ptr createSession(std::string _ip = "127.0.0.1")
    {
        unsigned const protocolVersion = 0;
        NodeIPEndpoint peer_endpoint(bi::address::from_string(_ip), m_listenPort, m_listenPort);
        KeyPair key_pair = KeyPair::create();
#if 0
        std::shared_ptr<Peer> peer = std::make_shared<Peer>(key_pair.pub(), peer_endpoint);
        PeerSessionInfo peer_info({key_pair.pub(), peer_endpoint.address.to_string(),
            chrono::steady_clock::duration(), 0});

        std::shared_ptr<SessionFace> session =
            std::make_shared<FakeSessionForHost>(m_host, peer, peer_info);
#endif
        std::shared_ptr<P2PSession> session = std::make_shared<P2PSession>();
        session->start();
        return session;
    }

    P2PSession::Ptr createSessionWithID(NodeID _peerID, std::string _ip = "127.0.0.1")
    {
        unsigned const protocolVersion = 0;
        NodeIPEndpoint peer_endpoint(bi::address::from_string(_ip), m_listenPort, m_listenPort);
        PeerSessionInfo peer_info(
            {_peerID, peer_endpoint.address.to_string(), std::chrono::steady_clock::duration(), 0});

#if 0
        std::shared_ptr<SessionFace> session =
            std::make_shared<FakeSessionForHost>(m_host, peer, peer_info);
#endif
        std::shared_ptr<P2PSession> session = std::make_shared<P2PSession>();
        session->start();
        return session;
    }

    TransactionPtr createTransaction(int64_t _currentBlockNumber)
    {
        const bytes c_txBytes = fromHex(
            "f8ef9f65f0d06e39dc3c08e32ac10a5070858962bc6c0f5760baca823f2d5582d03f85174876e7ff"
            "8609184e729fff82020394d6f1a71052366dbae2f7ab2d5d5845e77965cf0d80b86448f85bce000000"
            "000000000000000000000000000000000000000000000000000000001bf5bd8a9e7ba8b936ea704292"
            "ff4aaa5797bf671fdc8526dcd159f23c1f5a05f44e9fa862834dc7cb4541558f2b4961dc39eaaf0af7"
            "f7395028658d0e01b86a371ca00b2b3fabd8598fefdda4efdb54f626367fc68e1735a8047f0f1c4f84"
            "0255ca1ea0512500bc29f4cfe18ee1c88683006d73e56c934100b8abf4d2334560e1d2f75e");
        const u256 c_maxBlockLimit = u256(1000);
        Secret sec = KeyPair::create().secret();
        TransactionPtr txPtr =
            std::make_shared<Transaction>(ref(c_txBytes), CheckTransaction::Everything);
        txPtr->setNonce(txPtr->nonce() + u256(rand()));
        txPtr->setBlockLimit(u256(_currentBlockNumber) + c_maxBlockLimit);
        dev::Signature sig = sign(sec, txPtr->sha3(WithoutSignature));
        txPtr->updateSignature(SignatureStruct(sig));
        return txPtr;
    }

private:
    std::string m_clientVersion = "2.0";
    std::string m_listenIp = "127.0.0.1";
    uint16_t m_listenPort = 30304;

    TxPoolFixture m_txPoolFixture;
    FakeBlock m_fakeBlock;
};
}  // namespace test
}  // namespace dev
