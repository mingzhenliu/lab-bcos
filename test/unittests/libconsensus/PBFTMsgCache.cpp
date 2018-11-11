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
 * @brief: unit test for libconsensus/pbft/PBFTMsgCache.h
 * @file: PBFTMsgCache.cpp
 * @author: yujiechen
 * @date: 2018-10-11
 */
#include <libconsensus/pbft/PBFTMsgCache.h>
#include <test/tools/libutils/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>
using namespace dev::consensus;
namespace dev
{
namespace test
{
BOOST_FIXTURE_TEST_SUITE(consensusTest, TestOutputHelperFixture)

void checkKeyExist(PBFTBroadcastCache<PBFTCacheMsg>& cache, unsigned const& type,
    KeyPair const& keyPair, std::string const& str, bool const& insert = true,
    bool const& exist = true)
{
    PBFTCacheMsg key;
    key.blockHash = dev::sign(keyPair.secret(), sha3(str)).hex();
    key.height = 10;
    if (insert)
        cache.insertKey(keyPair.pub(), type, key);
    if (exist)
        BOOST_CHECK(cache.keyExists(keyPair.pub(), type, key));
    else
        BOOST_CHECK(!cache.keyExists(keyPair.pub(), type, key));
}

BOOST_AUTO_TEST_CASE(testInsertKey)
{
    PBFTBroadcastCache<PBFTCacheMsg> broadCast_cache;
    KeyPair key_pair = KeyPair::create();
    std::string key = dev::sign(key_pair.secret(), sha3("test")).hex();
    /// test insertKey && keyExist
    /// test PrepareReqPacket
    checkKeyExist(broadCast_cache, PrepareReqPacket, key_pair, "test1");
    checkKeyExist(broadCast_cache, PrepareReqPacket, key_pair, "test2");
    checkKeyExist(broadCast_cache, PrepareReqPacket, key_pair, "test3");
    /// test SignReqPacket
    checkKeyExist(broadCast_cache, SignReqPacket, key_pair, "test1");
    checkKeyExist(broadCast_cache, SignReqPacket, key_pair, "test2");
    checkKeyExist(broadCast_cache, SignReqPacket, key_pair, "test3");
    /// test CommitReqPacket
    checkKeyExist(broadCast_cache, CommitReqPacket, key_pair, "test1");
    checkKeyExist(broadCast_cache, CommitReqPacket, key_pair, "test2");
    checkKeyExist(broadCast_cache, CommitReqPacket, key_pair, "test3");
    /// test ViewChangeReqPacket
    checkKeyExist(broadCast_cache, ViewChangeReqPacket, key_pair, "test1");
    checkKeyExist(broadCast_cache, ViewChangeReqPacket, key_pair, "test2");
    checkKeyExist(broadCast_cache, ViewChangeReqPacket, key_pair, "test3");
    /// test other packet
    checkKeyExist(broadCast_cache, 100, key_pair, "test1", true, false);
    checkKeyExist(broadCast_cache, 100, key_pair, "test2", true, false);
    checkKeyExist(broadCast_cache, 1000, key_pair, "test3", true, false);
    /// clearAll
    broadCast_cache.clearAll();
    checkKeyExist(broadCast_cache, ViewChangeReqPacket, key_pair, "test1", false, false);
    checkKeyExist(broadCast_cache, ViewChangeReqPacket, key_pair, "test2", false, false);
    checkKeyExist(broadCast_cache, ViewChangeReqPacket, key_pair, "test3", false, false);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace test
}  // namespace dev
