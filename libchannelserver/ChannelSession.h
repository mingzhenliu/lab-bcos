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
/**
 * @file: ChannelSession.h
 * @author: monan
 *
 * @date: 2017
 */

#pragma once

#include <arpa/inet.h>
#include <libdevcore/easylog.h>
#include <queue>
#include <string>
#include <thread>

#include <libdevcore/Common.h>
#include <libdevcore/FixedHash.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>

#include "ChannelException.h"
#include "Message.h"
#include "libdevcore/ThreadPool.h"

namespace dev
{
namespace channel
{
class ChannelSession : public std::enable_shared_from_this<ChannelSession>
{
public:
    ChannelSession();
    virtual ~ChannelSession() { LOG(TRACE) << "Session exit"; };

    typedef std::shared_ptr<ChannelSession> Ptr;
    typedef std::function<void(dev::channel::ChannelException, dev::channel::Message::Ptr)>
        CallbackType;

    const size_t bufferLength = 1024;

    virtual Message::Ptr sendMessage(Message::Ptr request, size_t timeout = 0);
    virtual void asyncSendMessage(Message::Ptr request,
        std::function<void(dev::channel::ChannelException, Message::Ptr)> callback,
        uint32_t timeout = 0);

    virtual void run();

    virtual bool actived() { return _actived; };

    virtual void setMessageHandler(
        std::function<void(ChannelSession::Ptr, dev::channel::ChannelException, Message::Ptr)>
            handler)
    {
        _messageHandler = handler;
    };

    virtual std::string host() { return _host; };
    virtual int port() { return _port; };

    virtual void setHost(std::string host) { _host = host; };
    virtual void setPort(int port) { _port = port; };

    virtual bool enableSSL() { return _enableSSL; }
    virtual void setEnableSSL(bool ssl) { _enableSSL = ssl; }

    virtual std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> > sslSocket()
    {
        return _sslSocket;
    };
    virtual void setSSLSocket(
        std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> > socket);

    virtual void setIOService(std::shared_ptr<boost::asio::io_service> IOService)
    {
        _ioService = IOService;
    };

    std::shared_ptr<std::set<std::string> > topics() { return _topics; };
    void setTopics(std::shared_ptr<std::set<std::string> > topics) { _topics = topics; };

    void setThreadPool(ThreadPool::Ptr threadPool) { _threadPool = threadPool; }

    MessageFactory::Ptr messageFactory() { return _messageFactory; }
    void setMessageFactory(MessageFactory::Ptr messageFactory) { _messageFactory = messageFactory; }

    void setIdleTime(size_t idleTime) { _idleTime = idleTime; }

    void disconnectByQuit() { disconnect(ChannelException(-1, "quit")); }

private:
    void startRead();
    void onRead(const boost::system::error_code& error, size_t bytesTransferred);

    void startWrite();
    void onWrite(const boost::system::error_code& error, std::shared_ptr<bytes> buffer,
        size_t bytesTransferred);
    void writeBuffer(std::shared_ptr<bytes> buffer);

    void onMessage(dev::channel::ChannelException e, Message::Ptr message);
    void onTimeout(const boost::system::error_code& error, std::string seq);

    void onIdle(const boost::system::error_code& error);

    void disconnect(dev::channel::ChannelException e);

    void updateIdleTimer();

    MessageFactory::Ptr _messageFactory;
    std::function<void(ChannelSession::Ptr, dev::channel::ChannelException, Message::Ptr)>
        _messageHandler;

    bool _actived = false;
    bool _enableSSL = true;

    std::string _host;
    int _port = 0;

    byte _recvBuffer[1024];
    bytes _recvProtocolBuffer;

    std::queue<std::shared_ptr<bytes> > _sendBufferList;
    bool _writing = false;

    std::shared_ptr<boost::asio::deadline_timer> _idleTimer;
    std::recursive_mutex _mutex;

    std::shared_ptr<boost::asio::io_service> _ioService;
    std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> > _sslSocket;

    struct ResponseCallback : public std::enable_shared_from_this<ResponseCallback>
    {
        typedef std::shared_ptr<ResponseCallback> Ptr;

        std::string seq = "";
        std::function<void(ChannelException, Message::Ptr)> callback;
        std::shared_ptr<boost::asio::deadline_timer> timeoutHandler;
    };

    std::map<std::string, ResponseCallback::Ptr> _responseCallbacks;

    std::shared_ptr<std::set<std::string> > _topics;
    ThreadPool::Ptr _threadPool;

    size_t _idleTime = 30000;
};

}  // namespace channel

}  // namespace dev
