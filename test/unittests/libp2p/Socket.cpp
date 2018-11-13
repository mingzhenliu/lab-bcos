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
 * @brief: unit test for Socket
 *
 * @file Socket.cpp
 * @author: yujiechen
 * @date 2018-09-19
 */

#include <libdevcore/FileSystem.h>
#include <libinitializer/SecureInitializer.h>
#include <libp2p/Socket.h>
#include <openssl/ssl.h>
#include <test/tools/libutils/Common.h>
#include <test/tools/libutils/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>

using namespace dev;
using namespace dev::p2p;
namespace dev
{
namespace test
{
BOOST_FIXTURE_TEST_SUITE(SocketTest, TestOutputHelperFixture)
BOOST_AUTO_TEST_CASE(testSocket)
{
    ba::io_service m_io_service;
    std::string address = "127.0.0.1";
    int port = 30303;
    NodeIPEndpoint m_endpoint(address, port, port);
    setDataDir(getTestPath().string() + "/fisco-bcos-data");

    boost::property_tree::ptree pt;
    pt.put("secure.data_path", getTestPath().string() + "/fisco-bcos-data/");
    auto secureInitiailizer = std::make_shared<dev::initializer::SecureInitializer>();
    /// secureInitiailizer->setDataPath(getTestPath().string() + "/fisco-bcos-data/");
    secureInitiailizer->initConfig(pt);
    auto sslContext = secureInitiailizer->SSLContext();

    Socket m_socket(m_io_service, *sslContext, m_endpoint);
    BOOST_CHECK(m_socket.isConnected() == false);
    BOOST_CHECK(
        SSL_get_verify_mode(m_socket.sslref().native_handle()) == boost::asio::ssl::verify_peer);
    /// close socket
    m_socket.close();
    BOOST_CHECK(m_socket.isConnected() == false);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace test
}  // namespace dev
