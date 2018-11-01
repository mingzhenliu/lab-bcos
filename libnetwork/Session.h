/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Session.h
 * @author monan <651932351@qq.com>
 * @date 2018
 */

#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Guards.h>
#include <libdevcore/RLP.h>
#include <libdevcore/ThreadPool.h>
#include <boost/heap/priority_queue.hpp>
#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

#include "Common.h"
#include "SessionFace.h"
#include "SocketFace.h"

namespace dev
{
namespace p2p
{
class Peer;
class Host;

class Session : public SessionFace, public std::enable_shared_from_this<Session>
{
public:
    Session();
    virtual ~Session();

    typedef std::shared_ptr<Session> Ptr;
    static const size_t BUFFER_LENGTH = 1024;

    virtual void start() override;
    virtual void disconnect(DisconnectReason _reason) override;

    virtual bool isConnected() const override { return m_socket->isConnected(); }

    virtual void asyncSendMessage(Message::Ptr message, Options options, CallbackFunc callback) override;
    virtual Message::Ptr sendMessage(Message::Ptr message, Options options) override;

    virtual NodeIPEndpoint nodeIPEndpoint() const override { return m_socket->nodeIPEndpoint(); }

    virtual bool actived() const override;

    virtual std::weak_ptr<Host> host() { return m_server; }
    virtual void setHost(std::weak_ptr<Host> host) { m_server = host; }

    virtual std::weak_ptr<Host> server() { return m_server; }
    virtual void setServer(std::weak_ptr<Host> server) { m_server = server; }

    virtual std::shared_ptr<SocketFace> socket() { return m_socket; }
    virtual void setSocket(std::shared_ptr<SocketFace> socket) { m_socket = socket; }

    virtual MessageFactory::Ptr messageFactory() const { return m_messageFactory; }
    virtual void setMessageFactory(MessageFactory::Ptr _messageFactory) { m_messageFactory = _messageFactory; }

private:
    void send(std::shared_ptr<bytes> _msg);

    void doRead();
    std::vector<byte> m_data;  ///< Buffer for ingress packet data.
    byte m_recvBuffer[BUFFER_LENGTH];

    /// Drop the connection for the reason @a _r.
    void drop(DisconnectReason _r);

    /// Check error code after reading and drop peer if error code.
    bool checkRead(boost::system::error_code _ec);

    void onTimeout(const boost::system::error_code& error, uint32_t seq);

    /// Perform a single round of the write operation. This could end up calling itself
    /// asynchronously.
    void onWrite(boost::system::error_code ec, std::size_t length, std::shared_ptr<bytes> buffer);
    void write();

    /// call by doRead() to deal with mesage
    void onMessage(NetworkException const& e, std::shared_ptr<Session> session, Message::Ptr message);

    std::weak_ptr<Host> m_server;                        ///< The host that owns us. Never null.
    std::shared_ptr<SocketFace> m_socket;  ///< Socket of peer's connection.
    Mutex x_framing;                       ///< Mutex for the write queue.
    MessageFactory::Ptr m_messageFactory;

    class QueueCompare
    {
    public:
        bool operator()(const std::pair<std::shared_ptr<bytes>, u256>& lhs,
            const std::pair<std::shared_ptr<bytes>, u256>& rhs) const
        {
            return false;
        }
    };

    boost::heap::priority_queue<std::pair<std::shared_ptr<bytes>, u256>,
        boost::heap::compare<QueueCompare>, boost::heap::stable<true>>
        m_writeQueue;

    mutable Mutex x_info;

    bool m_actived = false;

    ///< A call B, the function to call after the response is received by A.
    mutable RecursiveMutex x_seq2Callback;
    std::shared_ptr<std::unordered_map<uint32_t, ResponseCallback::Ptr> > m_seq2Callback;

    std::function<void(NetworkException, Session::Ptr, Message::Ptr)> m_messageHandler;
};

}  // namespace p2p
}  // namespace dev
