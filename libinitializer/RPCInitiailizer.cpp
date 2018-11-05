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

    if (listenPort > 0)
    {
        m_channelRPCServer.reset(new ChannelRPCServer(), [](ChannelRPCServer* p) { (void)p; });
        m_channelRPCServer->setListenAddr(listenIP);
        m_channelRPCServer->setListenPort(listenPort);
        m_channelRPCServer->setSSLContext(m_sslContext);

        auto ioService = std::make_shared<boost::asio::io_service>();

        auto server = std::make_shared<dev::channel::ChannelServer>();
        server->setIOService(ioService);
        server->setSSLContext(m_sslContext);
        ///< TODO: The p2pService is passed to the ChannelServer module.
        // server->setService(m_p2pService);
        server->setEnableSSL(true);
        server->setBind(listenIP, listenPort);

        server->setMessageFactory(std::make_shared<dev::channel::ChannelMessageFactory>());

        m_channelRPCServer->setChannelServer(server);
    }

    ///< TODO: Wait for SafeHttpServer to commit and open this.
    /*if (httpListenPort > 0)
    {
        m_safeHttpServer.reset(
            new SafeHttpServer(listenIP, httpListenPort), [](SafeHttpServer* p) { (void)p; });
        auto rpcEntity = new rpc::Rpc(m_ledgerManager, m_p2pService);
        ModularServer<>* jsonrpcHttpServer = new ModularServer<rpc::Rpc>(rpcEntity);
        jsonrpcHttpServer->addConnector(m_safeHttpServer.get());
        jsonrpcHttpServer->StartListening();
        cout << "JsonrpcHttpServer started." << std::endl;
    }*/
}