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

/**:
 * @brief : sync msg engine test
 * @author: catli
 * @date: 2018-10-25
 */

#include <libsync/SyncMsgEngine.h>
#include <libsync/SyncMsgPacket.h>
#include <test/tools/libutils/TestOutputHelper.h>
#include <test/unittests/libsync/FakeSyncToolsSet.h>
#include <boost/test/unit_test.hpp>
#include <memory>

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::p2p;
using namespace dev::sync;

namespace dev
{
namespace test
{
class SyncMsgEngineFixture : TestOutputHelperFixture
{
public:
    SyncMsgEngineFixture()
      : fakeSyncToolsSet(),
        fakeStatusPtr(make_shared<SyncMasterStatus>(h256(0x1024))),
        fakeMsgEngine(fakeSyncToolsSet.getServicePtr(), fakeSyncToolsSet.getTxPoolPtr(),
            fakeSyncToolsSet.getBlockChainPtr(), fakeStatusPtr, 0, NodeID(0xabcd), h256(0xcdef)),
        fakeException()
    {}
    FakeSyncToolsSet fakeSyncToolsSet;
    shared_ptr<SyncMasterStatus> fakeStatusPtr;
    SyncMsgEngine fakeMsgEngine;
    NetworkException fakeException;
};

BOOST_FIXTURE_TEST_SUITE(SyncMsgEngineTest, SyncMsgEngineFixture)

BOOST_AUTO_TEST_CASE(InvalidInputTest)
{
    auto fakeMsgPtr = make_shared<P2PMessage>();
    // Invalid Session
    auto fakeSessionPtr = fakeSyncToolsSet.createSessionWithID(h512(0xabcd));
    BOOST_CHECK(fakeSessionPtr->actived() == true);
    fakeMsgEngine.messageHandler(fakeException, fakeSessionPtr, fakeMsgPtr);
    BOOST_CHECK(fakeSessionPtr->actived() == false);

    // Invalid Message
    fakeSessionPtr = fakeSyncToolsSet.createSession();
    BOOST_CHECK(fakeSessionPtr->actived() == true);
    fakeMsgEngine.messageHandler(fakeException, fakeSessionPtr, fakeMsgPtr);
    BOOST_CHECK(fakeSessionPtr->actived() == false);
}

BOOST_AUTO_TEST_CASE(SyncStatusPacketTest)
{
    auto statusPacket = SyncStatusPacket();
    statusPacket.encode(0x00, h256(0xab), h256(0xcd));
    auto msgPtr = statusPacket.toMessage(0x01);
    auto fakeSessionPtr = fakeSyncToolsSet.createSessionWithID(h512(0x1234));
    fakeMsgEngine.messageHandler(fakeException, fakeSessionPtr, msgPtr);

    BOOST_CHECK(fakeStatusPtr->hasPeer(h512(0x1234)));
    fakeMsgEngine.messageHandler(fakeException, fakeSessionPtr, msgPtr);
    BOOST_CHECK(fakeStatusPtr->hasPeer(h512(0x1234)));
}

BOOST_AUTO_TEST_CASE(SyncTransactionPacketTest)
{
    auto txPacket = SyncTransactionsPacket();
    auto txPtr = fakeSyncToolsSet.createTransaction(0);
    bytes txRLPs = txPtr->rlp();
    txPacket.encode(0x01, txRLPs);
    auto msgPtr = txPacket.toMessage(0x02);
    auto fakeSessionPtr = fakeSyncToolsSet.createSession();
    fakeMsgEngine.messageHandler(fakeException, fakeSessionPtr, msgPtr);

    auto txPoolPtr = fakeSyncToolsSet.getTxPoolPtr();
    auto topTxs = txPoolPtr->topTransactions(1);
    BOOST_CHECK(topTxs.size() == 1);
    BOOST_CHECK_EQUAL(topTxs[0].sha3(), txPtr->sha3());
}

BOOST_AUTO_TEST_CASE(SyncBlocksPacketTest)
{
    SyncBlocksPacket blocksPacket;
    vector<bytes> blockRLPs;
    auto& fakeBlock = fakeSyncToolsSet.getBlock();
    fakeBlock.header().setNumber(INT64_MAX - 1);
    blockRLPs.push_back(fakeBlock.rlp());
    blocksPacket.encode(blockRLPs);
    auto msgPtr = blocksPacket.toMessage(0x03);
    auto fakeSessionPtr = fakeSyncToolsSet.createSession();
    fakeMsgEngine.messageHandler(fakeException, fakeSessionPtr, msgPtr);

    BOOST_CHECK(fakeStatusPtr->bq().size() == 1);
    auto block = fakeStatusPtr->bq().top(true);
    BOOST_CHECK(block->equalAll(fakeBlock));
}

BOOST_AUTO_TEST_CASE(SyncReqBlockPacketTest)
{
    SyncReqBlockPacket reqBlockPacket;
    reqBlockPacket.encode(int64_t(0x01), 0x02);
    auto msgPtr = reqBlockPacket.toMessage(0x03);
    auto fakeSessionPtr = fakeSyncToolsSet.createSession();
    fakeMsgEngine.messageHandler(fakeException, fakeSessionPtr, msgPtr);

    auto servicePtr = static_pointer_cast<FakeService>(fakeSyncToolsSet.getServicePtr());
    auto sendCount = servicePtr->getAsyncSendSizeByNodeID(fakeSessionPtr->nodeID());
    BOOST_CHECK(sendCount == 1);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace test
}  // namespace dev
