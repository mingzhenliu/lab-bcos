/*
    This file is part of FISCO-BCOS.

    FISCO-BCOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FISCO-BCOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file RPCInitiailizer.h
 *  @author chaychen
 *  @modify first draft
 *  @date 20181022
 */

#include "RPCInitiailizer.h"

using namespace dev;
using namespace dev::initializer;

void RPCInitiailizer::initConfig(boost::property_tree::ptree const& _pt)
{
    std::string listenIP = _pt.get<std::string>("rpc.listen_ip", "0.0.0.0");
    int listenPort = _pt.get<int>("rpc.listen_port", 30301);
    int httpListenPort = _pt.get<int>("rpc.http_listen_port", 0);
    if (!isValidPort(listenPort) || !isValidPort(httpListenPort))
    {
        LOG(ERROR) << "[#RPCInitiailizer] initConfig for RPCInitiailizer failed";
        BOOST_THROW_EXCEPTION(InvalidListenPort() << errinfo_comment(
                                  "[#RPCInitiailizer] initConfig for RPCInitiailizer "
                                  "failed! Invalid ListenPort for RPC, must between [0,65536]"));
    }
    /// init channelServer
    ChannelRPCServer::Ptr m_channelRPCServer;
    ///< TODO: Double free or no free?
    ///< Donot to set destructions, the ModularServer will destruct.
    m_channelRPCServer.reset(new ChannelRPCServer(), [](ChannelRPCServer* p) { (void)p; });
    m_channelRPCServer->setListenAddr(listenIP);
    m_channelRPCServer->setListenPort(listenPort);
    m_channelRPCServer->setSSLContext(m_sslContext);
    m_channelRPCServer->setService(m_p2pService);

    auto ioService = std::make_shared<boost::asio::io_service>();

    auto server = std::make_shared<dev::channel::ChannelServer>();
    server->setIOService(ioService);
    server->setSSLContext(m_sslContext);
    server->setEnableSSL(true);
    server->setBind(listenIP, listenPort);
    server->setMessageFactory(std::make_shared<dev::channel::ChannelMessageFactory>());

    m_channelRPCServer->setChannelServer(server);

    auto rpcEntity = new rpc::Rpc(m_ledgerManager, m_p2pService);
    m_channelRPCHttpServer = new ModularServer<rpc::Rpc>(rpcEntity);
    m_channelRPCHttpServer->addConnector(m_channelRPCServer.get());
    m_channelRPCHttpServer->StartListening();
    INITIALIZER_LOG(INFO) << "ChannelRPCHttpServer started.";

    /// init httpListenPort
    ///< Donot to set destructions, the ModularServer will destruct.
    m_safeHttpServer.reset(
        new SafeHttpServer(listenIP, httpListenPort), [](SafeHttpServer* p) { (void)p; });
    m_jsonrpcHttpServer->addConnector(m_safeHttpServer.get());
    m_jsonrpcHttpServer->StartListening();
    INITIALIZER_LOG(INFO) << "JsonrpcHttpServer started.";
}
