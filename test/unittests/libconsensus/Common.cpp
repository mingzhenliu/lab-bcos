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
 * @brief: unit test for Common.h of libconsensus module
 * @file: Common.cpp
 * @author: yujiechen
 * @date: 2018-10-09
 */
#include <libconsensus/Common.h>
#include <test/tools/libutils/TestOutputHelper.h>
#include <test/unittests/libethcore/FakeBlock.h>
#include <boost/test/unit_test.hpp>
using namespace dev::consensus;

namespace dev
{
namespace test
{
template <typename T>
void checkPBFTMsg(T const& msg, KeyPair const _keyPair = KeyPair::create(),
    int64_t const& _height = -1, u256 const& _view = Invalid256, u256 const& _idx = Invalid256,
    u256 const& _timestamp = Invalid256, h256 const& _blockHash = h256())
{
    BOOST_CHECK(msg.height == _height);
    BOOST_CHECK(msg.view == _view);
    BOOST_CHECK(msg.idx == _idx);
    BOOST_CHECK(msg.timestamp == _timestamp);
    BOOST_CHECK(msg.block_hash == _blockHash);
    if (msg.sig != Signature())
        BOOST_CHECK(msg.sig == msg.signHash(msg.block_hash, _keyPair));
    if (msg.sig2 != Signature())
        BOOST_CHECK(msg.sig2 == msg.signHash(msg.fieldsWithoutBlock(), _keyPair));
}

template <typename T>
void checkSignAndCommitReq()
{
    KeyPair key_pair = KeyPair::create();
    h256 block_hash = sha3("key_pair");
    PrepareReq prepare_req(key_pair, 1000, u256(1), u256(134), block_hash);
    KeyPair key_pair2 = KeyPair::create();
    T checked_req(prepare_req, key_pair2, prepare_req.idx);
    BOOST_CHECK(prepare_req.sig != checked_req.sig && prepare_req.sig2 != checked_req.sig2);
    /// test encode && decode
    bytes req_data;
    BOOST_REQUIRE_NO_THROW(checked_req.encode(req_data));
    T tmp_req;
    BOOST_REQUIRE_NO_THROW(tmp_req.decode(ref(req_data)));
    BOOST_CHECK(tmp_req == checked_req);
    /// test decode exception
    req_data[0] += 1;
    BOOST_CHECK_THROW(tmp_req.decode(ref(req_data)), std::exception);
}

BOOST_FIXTURE_TEST_SUITE(consensusCommonTest, TestOutputHelperFixture)
/// test PBFTMsg
BOOST_AUTO_TEST_CASE(testPBFTMsg)
{
    /// test default construct
    PBFTMsg pbft_msg;
    checkPBFTMsg(pbft_msg);
    /// test encode
    h256 block_hash = sha3("block_hash");
    KeyPair key_pair = KeyPair::create();
    PBFTMsg faked_pbft_msg(key_pair, 1000, u256(1), u256(134), block_hash);
    checkPBFTMsg(
        faked_pbft_msg, key_pair, 1000, u256(1), u256(134), faked_pbft_msg.timestamp, block_hash);
    /// test encode
    bytes faked_pbft_data;
    faked_pbft_msg.encode(faked_pbft_data);
    BOOST_CHECK(faked_pbft_data.size());
    /// test decode
    PBFTMsg decoded_pbft_msg;
    BOOST_REQUIRE_NO_THROW(decoded_pbft_msg.decode(ref(faked_pbft_data)));
    checkPBFTMsg(
        decoded_pbft_msg, key_pair, 1000, u256(1), u256(134), faked_pbft_msg.timestamp, block_hash);
    /// test exception case of decode
    std::string invalid_str = "faked_pbft_msg";
    bytes invalid_data(invalid_str.begin(), invalid_str.end());
    BOOST_CHECK_THROW(faked_pbft_msg.decode(ref(invalid_data)), std::exception);

    faked_pbft_data[0] -= 1;
    BOOST_CHECK_THROW(faked_pbft_msg.decode(ref(faked_pbft_data)), std::exception);
    faked_pbft_data[0] += 1;
    BOOST_REQUIRE_NO_THROW(faked_pbft_msg.decode(ref(faked_pbft_data)));
}
/// test PrepareReq
BOOST_AUTO_TEST_CASE(testPrepareReq)
{
    KeyPair key_pair = KeyPair::create();
    h256 block_hash = sha3("key_pair");
    PrepareReq prepare_req(key_pair, 1000, u256(1), u256(134), block_hash);
    checkPBFTMsg(
        prepare_req, key_pair, 1000, u256(1), u256(134), prepare_req.timestamp, block_hash);
    FakeBlock fake_block(5);
    prepare_req.block = fake_block.m_blockData;

    /// test encode && decode
    bytes prepare_req_data;
    BOOST_REQUIRE_NO_THROW(prepare_req.encode(prepare_req_data));
    BOOST_CHECK(prepare_req_data.size());
    /// test decode
    PrepareReq decode_prepare;
    decode_prepare.decode(ref(prepare_req_data));
    BOOST_REQUIRE_NO_THROW(decode_prepare.decode(ref(prepare_req_data)));
    checkPBFTMsg(
        decode_prepare, key_pair, 1000, u256(1), u256(134), prepare_req.timestamp, block_hash);
    BOOST_CHECK(decode_prepare.block == fake_block.m_blockData);
    /// test exception case
    prepare_req_data[0] += 1;
    BOOST_CHECK_THROW(decode_prepare.decode(ref(prepare_req_data)), std::exception);

    /// test construct from given prepare req
    KeyPair key_pair2 = KeyPair::create();
    PrepareReq constructed_prepare(decode_prepare, key_pair2, u256(2), u256(135));
    checkPBFTMsg(constructed_prepare, key_pair2, 1000, u256(2), u256(135),
        constructed_prepare.timestamp, block_hash);
    BOOST_CHECK(constructed_prepare.timestamp >= decode_prepare.timestamp);
    BOOST_CHECK(decode_prepare.block == constructed_prepare.block);

    /// test construct prepare from given block
    PrepareReq block_populated_prepare(fake_block.m_block, key_pair2, u256(2), u256(135));
    checkPBFTMsg(block_populated_prepare, key_pair2, fake_block.m_block.blockHeader().number(),
        u256(2), u256(135), block_populated_prepare.timestamp, fake_block.m_block.header().hash());
    BOOST_CHECK(block_populated_prepare.timestamp >= constructed_prepare.timestamp);
    /// test encode && decode
    block_populated_prepare.encode(prepare_req_data);
    PrepareReq tmp_req;
    BOOST_REQUIRE_NO_THROW(tmp_req.decode(ref(prepare_req_data)));
    checkPBFTMsg(tmp_req, key_pair2, fake_block.m_block.blockHeader().number(), u256(2), u256(135),
        block_populated_prepare.timestamp, fake_block.m_block.header().hash());
    Block tmp_block;
    BOOST_REQUIRE_NO_THROW(tmp_block.decode(ref(tmp_req.block)));
    BOOST_CHECK(tmp_block.equalAll(fake_block.m_block));

    /// test updatePrepareReq
    Sealing sealing;
    sealing.block = fake_block.m_block;
    block_populated_prepare.block = bytes();
    sealing.p_execContext = dev::blockverifier::ExecutiveContext::Ptr();
    block_populated_prepare.updatePrepareReq(sealing, key_pair2);
    checkPBFTMsg(block_populated_prepare, key_pair2, fake_block.m_block.blockHeader().number(),
        u256(2), u256(135), block_populated_prepare.timestamp, fake_block.m_block.header().hash());
    BOOST_CHECK(block_populated_prepare.timestamp >= tmp_req.timestamp);
    BOOST_CHECK(block_populated_prepare.block == fake_block.m_blockData);
}

/// test SignReq and CommitReq
BOOST_AUTO_TEST_CASE(testSignReqAndCommitReq)
{
    checkSignAndCommitReq<SignReq>();
    checkSignAndCommitReq<CommitReq>();
}

/// test PBFTMsgPacket
BOOST_AUTO_TEST_CASE(testPBFTMsgPacket) {}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace test
}  // namespace dev
