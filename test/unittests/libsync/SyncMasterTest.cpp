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
 * @brief : sync master test
 * @author: jimmyshi
 * @date: 2018-10-15
 */

#include <libethcore/Transaction.h>
#include <libsync/Common.h>
#include <libsync/SyncMaster.h>
#include <libsync/SyncMsgEngine.h>
#include <libsync/SyncStatus.h>
#include <test/tools/libutils/TestOutputHelper.h>
#include <test/unittests/libblockverifier/FakeBlockVerifier.h>
#include <test/unittests/libtxpool/FakeBlockChain.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::sync;
using namespace dev::blockverifier;

namespace dev
{
namespace test
{
class SyncFixture : public TestOutputHelperFixture
{
public:
    const int16_t c_protocolId = 66;
    const bytes c_txBytes = fromHex(
        "f8aa8401be1a7d80830f4240941dc8def0867ea7e3626e03acee3eb40ee17251c880b84494e78a100000000000"
        "000000000000003ca576d469d7aa0244071d27eb33c5629753593e000000000000000000000000000000000000"
        "00000000000000000000000013881ba0f44a5ce4a1d1d6c2e4385a7985cdf804cb10a7fb892e9c08ff6d62657c"
        "4da01ea01d4c2af5ce505f574a320563ea9ea55003903ca5d22140155b3c2c968df0509464");
    const u256 c_maxBlockLimit = u256(1000);

public:
    SyncFixture()
    {
        m_sec = KeyPair::create().secret();
        m_genesisHash = h256(0);
    }

    struct FakeSyncToolsSet
    {
        std::shared_ptr<SyncMaster> sync;
        std::shared_ptr<FakeService> service;
        std::shared_ptr<TxPoolInterface> txPool;
        std::shared_ptr<BlockChainInterface> blockChain;
        std::shared_ptr<BlockVerifierInterface> verifier;
    };

    FakeSyncToolsSet fakeSyncToolsSet(
        uint64_t _blockNum, size_t const& _transSize, NodeID const& _nodeId)
    {
        TxPoolFixture txpool_creator(_blockNum, _transSize);
        m_genesisHash = txpool_creator.m_blockChain->getBlockByNumber(0)->headerHash();

        std::shared_ptr<BlockVerifierInterface> blockVerifier =
            std::make_shared<FakeBlockverifier>();
        std::shared_ptr<SyncMaster> fakeSyncMaster =
            std::make_shared<SyncMaster>(txpool_creator.m_topicService, txpool_creator.m_txPool,
                txpool_creator.m_blockChain, blockVerifier, c_protocolId, _nodeId, m_genesisHash);
        return FakeSyncToolsSet{fakeSyncMaster, txpool_creator.m_topicService,
            txpool_creator.m_txPool, txpool_creator.m_blockChain, blockVerifier};
    }

    shared_ptr<Transactions> fakeTransactions(size_t _num, int64_t _currentBlockNumber)
    {
        shared_ptr<Transactions> txs = make_shared<Transactions>();
        for (size_t i = 0; i < _num; ++i)
        {
            Transaction tx(ref(c_txBytes), CheckTransaction::Everything);
            tx.setNonce(tx.nonce() + u256(rand()));
            tx.setBlockLimit(u256(_currentBlockNumber) + c_maxBlockLimit);
            dev::Signature sig = sign(m_sec, tx.sha3(WithoutSignature));
            tx.updateSignature(SignatureStruct(sig));
            // std::pair<h256, Address> ret = txPool->submit(tx);
            txs->emplace_back(tx);
        }
        return txs;
    }

public:
    Secret m_sec;
    h256 m_genesisHash;
};

BOOST_FIXTURE_TEST_SUITE(SyncMasterTest, SyncFixture)

BOOST_AUTO_TEST_CASE(ConstructorTest)
{
    std::shared_ptr<SyncMaster> sync = fakeSyncToolsSet(5, 5, NodeID(100)).sync;
    BOOST_CHECK_EQUAL(sync->protocolId(), c_protocolId);
    BOOST_CHECK_EQUAL(sync->nodeId(), NodeID(100));
    BOOST_CHECK_EQUAL(sync->genesisHash(), m_genesisHash);
}

BOOST_AUTO_TEST_CASE(MaintainTransactionsTest)
{
    int64_t currentBlockNumber = 4;
    FakeSyncToolsSet syncTools = fakeSyncToolsSet(currentBlockNumber + 1, 5, NodeID(100));
    std::shared_ptr<SyncMaster> sync = syncTools.sync;
    std::shared_ptr<FakeService> service = syncTools.service;
    std::shared_ptr<TxPoolInterface> txPool = syncTools.txPool;

    sync->syncStatus()->newSyncPeerStatus(
        SyncPeerInfo{NodeID(101), 0, m_genesisHash, m_genesisHash});
    sync->syncStatus()->newSyncPeerStatus(
        SyncPeerInfo{NodeID(102), 0, m_genesisHash, m_genesisHash});

    shared_ptr<Transactions> txs = fakeTransactions(2, currentBlockNumber);
    for (auto& tx : *txs)
        txPool->submit(tx);

    sync->maintainTransactions();
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(101)) << endl;
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(102)) << endl;

    BOOST_CHECK_EQUAL(service->getAsyncSendSizeByNodeID(NodeID(101)), 1);
    BOOST_CHECK_EQUAL(service->getAsyncSendSizeByNodeID(NodeID(102)), 1);

    // Message::Ptr msg = service->getAsyncSendMessageByNodeID(NodeID(101));
    // TODO

    // test transaction has send logic
    txs = fakeTransactions(2, currentBlockNumber);
    for (auto& tx : *txs)
        txPool->submit(tx);
    sync->maintainTransactions();
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(101)) << endl;
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(102)) << endl;

    BOOST_CHECK_EQUAL(service->getAsyncSendSizeByNodeID(NodeID(101)), 2);
    BOOST_CHECK_EQUAL(service->getAsyncSendSizeByNodeID(NodeID(102)), 2);

    // test transaction known by peer logic
    txs = fakeTransactions(1, currentBlockNumber);
    for (auto& tx : *txs)
    {
        txPool->submit(tx);
        txPool->transactionIsKonwnBy(tx.sha3(), NodeID(101));
    }
    sync->maintainTransactions();
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(101)) << endl;
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(102)) << endl;

    BOOST_CHECK_EQUAL(service->getAsyncSendSizeByNodeID(NodeID(101)), 2);
    BOOST_CHECK_EQUAL(service->getAsyncSendSizeByNodeID(NodeID(102)), 3);

    // test transaction already sent
    sync->maintainTransactions();
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(101)) << endl;
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(102)) << endl;

    BOOST_CHECK_EQUAL(service->getAsyncSendSizeByNodeID(NodeID(101)), 2);
    BOOST_CHECK_EQUAL(service->getAsyncSendSizeByNodeID(NodeID(102)), 3);
}

BOOST_AUTO_TEST_CASE(MaintainBlocksTest)
{
    int64_t currentBlockNumber = 4;
    FakeSyncToolsSet syncTools = fakeSyncToolsSet(currentBlockNumber + 1, 5, NodeID(100));
    std::shared_ptr<SyncMaster> sync = syncTools.sync;
    std::shared_ptr<FakeService> service = syncTools.service;

    sync->syncStatus()->newSyncPeerStatus(
        SyncPeerInfo{NodeID(101), 0, m_genesisHash, m_genesisHash});
    sync->syncStatus()->newSyncPeerStatus(
        SyncPeerInfo{NodeID(102), 0, m_genesisHash, m_genesisHash});

    sync->maintainBlocks();
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(101)) << endl;

    BOOST_CHECK_EQUAL(service->getAsyncSendSizeByNodeID(NodeID(101)), 1);
}

BOOST_AUTO_TEST_CASE(MaintainPeersStatusTest)
{
    int64_t currentBlockNumber = 4;
    FakeSyncToolsSet syncTools = fakeSyncToolsSet(currentBlockNumber + 1, 5, NodeID(100));
    std::shared_ptr<SyncMaster> sync = syncTools.sync;
    std::shared_ptr<FakeService> service = syncTools.service;

    // Can recieve 5 req
    sync->syncStatus()->newSyncPeerStatus(SyncPeerInfo{
        NodeID(101), c_maxRequestBlocks * 5 + currentBlockNumber, m_genesisHash, m_genesisHash});
    // Can recieve 1 req
    sync->syncStatus()->newSyncPeerStatus(SyncPeerInfo{
        NodeID(102), c_maxRequestBlocks + currentBlockNumber, m_genesisHash, m_genesisHash});
    // Can recieve 10(at least 5)req
    sync->syncStatus()->newSyncPeerStatus(SyncPeerInfo{
        NodeID(103), c_maxRequestBlocks * 10 + currentBlockNumber, m_genesisHash, m_genesisHash});

    sync->maintainPeersStatus();
    cout << "Msg number: " << service->getAsyncSendSizeByNodeID(NodeID(103)) << endl;

    BOOST_CHECK(service->getAsyncSendSizeByNodeID(NodeID(103)) >= 5);

    size_t reqPacketSum = service->getAsyncSendSizeByNodeID(NodeID(101)) +
                          service->getAsyncSendSizeByNodeID(NodeID(102)) +
                          service->getAsyncSendSizeByNodeID(NodeID(103));
    BOOST_CHECK_EQUAL(reqPacketSum, 10);
}

BOOST_AUTO_TEST_CASE(MaintainDownloadingQueueTest)
{
    int64_t currentBlockNumber = 0;
    int64_t latestNumber = 6;
    FakeSyncToolsSet syncTools = fakeSyncToolsSet(currentBlockNumber + 1, 5, NodeID(100));
    std::shared_ptr<SyncMaster> sync = syncTools.sync;
    std::shared_ptr<SyncMasterStatus> status = sync->syncStatus();
    std::shared_ptr<BlockChainInterface> blockChain = syncTools.blockChain;

    FakeBlockChain latestBlockChain(latestNumber + 1, 5);
    status->knownHighestNumber = latestNumber;
    status->knownLatestHash = latestBlockChain.getBlockByNumber(latestNumber)->headerHash();

    int64_t currentNumber = 0;

    // latestNumber: 6  curr:[0] -> downloadqueue:[1] -> curr:[0, 1]  not finish
    cout << "Number: " << blockChain->number() << endl;
    shared_ptr<Block> b1 = latestBlockChain.getBlockByNumber(1);
    status->bq().push(b1);
    BOOST_CHECK_EQUAL(blockChain->number(), 0);
    BOOST_CHECK_EQUAL(sync->maintainDownloadingQueue(), false);  // Not finish
    BOOST_CHECK_EQUAL(blockChain->number(), 1);

    // latestNumber: 6  curr:[0, 1] -> downloadqueue:[3, 4] -> curr:[0, 1]  not finish
    cout << "Number: " << blockChain->number() << endl;
    shared_ptr<Block> b3 = latestBlockChain.getBlockByNumber(3);
    shared_ptr<Block> b4 = latestBlockChain.getBlockByNumber(4);
    status->bq().push(b3);
    status->bq().push(b4);
    BOOST_CHECK_EQUAL(blockChain->number(), 1);
    BOOST_CHECK_EQUAL(sync->maintainDownloadingQueue(), false);  // Not finish
    BOOST_CHECK_EQUAL(blockChain->number(), 1);

    // latestNumber: 6  curr:[0, 1] -> downloadqueue:[2, 3, 4] -> curr:[0, 4]  not finish
    cout << "Number: " << blockChain->number() << endl;
    shared_ptr<Block> b2 = latestBlockChain.getBlockByNumber(2);
    status->bq().push(b2);
    BOOST_CHECK_EQUAL(blockChain->number(), 1);
    BOOST_CHECK_EQUAL(sync->maintainDownloadingQueue(), false);  // Not finish
    BOOST_CHECK_EQUAL(blockChain->number(), 4);

    // latestNumber: 6  curr:[0, 4] -> downloadqueue:[5, 6] -> curr:[0, 6]  finish
    cout << "Number: " << blockChain->number() << endl;
    shared_ptr<Block> b5 = latestBlockChain.getBlockByNumber(5);
    shared_ptr<Block> b6 = latestBlockChain.getBlockByNumber(6);
    status->bq().push(b5);
    status->bq().push(b6);
    BOOST_CHECK_EQUAL(blockChain->number(), 4);
    BOOST_CHECK_EQUAL(sync->maintainDownloadingQueue(), true);  // finish
    BOOST_CHECK_EQUAL(blockChain->number(), 6);
    cout << "Number: " << blockChain->number() << endl;

    // Check download hash and expected hash are the same
    for (int64_t i = 0; i <= latestNumber; ++i)
    {
        h256 const& downloadHash = blockChain->getBlockByNumber(i)->headerHash();
        h256 const& expectedHash = latestBlockChain.getBlockByNumber(i)->headerHash();

        cout << "Block: " << i << endl;
        cout << "Download hash: " << downloadHash << endl;
        cout << "Expected hash: " << expectedHash << endl;
        BOOST_CHECK_EQUAL(downloadHash, expectedHash);
    }
}


BOOST_AUTO_TEST_SUITE_END()
}  // namespace test
}  // namespace dev