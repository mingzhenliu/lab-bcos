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
/** @file Service.h
 *  @author monan
 *  @modify first draft
 *  @date 20180910
 *  @author chaychen
 *  @modify realize encode and decode, add timeout, code format
 *  @date 20180911
 */

#pragma once
#include <libdevcore/Common.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/FixedHash.h>
#include <map>
#include <unordered_map>
#include <memory>
#include <libnetwork/Host.h>
#include "P2PInterface.h"
#include "P2PSession.h"
#include "P2PMessage.h"

namespace dev
{
namespace p2p
{
class Service : public P2PInterface, public std::enable_shared_from_this<Service>
{
public:
    Service();
    virtual ~Service() {}

    typedef std::shared_ptr<Service> Ptr;

    virtual void start();
    virtual void stop();
    virtual void heartBeat();

    virtual bool actived() { return m_run; }
    virtual NodeID id() const { return m_alias.pub(); }

    virtual void onConnect(NetworkException e, NodeID nodeID, std::shared_ptr<SessionFace> session);
    virtual void onDisconnect(NetworkException e, NodeID nodeID);
    virtual void onMessage(NetworkException e, SessionFace::Ptr session, Message::Ptr message, P2PSession::Ptr p2pSession);

    virtual P2PMessage::Ptr sendMessageByNodeID(NodeID nodeID, P2PMessage::Ptr message) override;
    virtual void asyncSendMessageByNodeID(NodeID nodeID, P2PMessage::Ptr message,
        CallbackFuncWithSession callback, Options options = Options()) override;

    virtual P2PMessage::Ptr sendMessageByTopic(std::string topic, P2PMessage::Ptr message) override;
    virtual void asyncSendMessageByTopic(std::string topic, P2PMessage::Ptr message,
        CallbackFuncWithSession callback, Options options) override;

    virtual void asyncMulticastMessageByTopic(std::string topic, P2PMessage::Ptr message) override;
    virtual void asyncMulticastMessageByNodeIDList(NodeIDs nodeIDs, P2PMessage::Ptr message) override;
    virtual void asyncBroadcastMessage(P2PMessage::Ptr message, Options options) override;

    virtual void registerHandlerByProtoclID(PROTOCOL_ID protocolID, CallbackFuncWithSession handler) override;
    virtual void registerHandlerByTopic(std::string topic, CallbackFuncWithSession handler) override;

    virtual std::map<NodeIPEndpoint, NodeID> staticNodes() { return m_staticNodes; }
    virtual void setStaticNodes(std::map<NodeIPEndpoint, NodeID> staticNodes) { m_staticNodes = staticNodes; }

    virtual SessionInfos sessionInfos() override; ///< Only connected node

    virtual SessionInfos sessionInfosByProtocolID(PROTOCOL_ID _protocolID) override;

    virtual bool isConnected(NodeID nodeID) override;

    virtual h512s getNodeListByGroupID(GROUP_ID groupID) override { return m_groupID2NodeList[groupID]; }
    virtual void setGroupID2NodeList(std::map<GROUP_ID, h512s> _groupID2NodeList) override { m_groupID2NodeList = _groupID2NodeList; }

    virtual uint32_t topicSeq() { return m_topicSeq; }
    virtual void increaseTopicSeq() { ++m_topicSeq; }

    virtual std::shared_ptr<std::vector<std::string>> topics() override { return m_topics; }
    virtual void setTopics(std::shared_ptr<std::vector<std::string>> _topics) override { RecursiveMutex(x_topics); m_topics = _topics; ++m_topicSeq; }

    virtual std::shared_ptr<Host> host() { return m_host; }
    virtual void setHost(std::shared_ptr<Host> host) { m_host = host; }

    virtual std::shared_ptr<P2PMessageFactory> p2pMessageFactory() override { return m_p2pMessageFactory; }
    virtual void setP2PMessageFactory(std::shared_ptr<P2PMessageFactory> _p2pMessageFactory) { m_p2pMessageFactory = _p2pMessageFactory; }

    virtual KeyPair keyPair() { return m_alias; }
    virtual void setKeyPair(KeyPair keyPair) { m_alias = keyPair; }

private:
    NodeIDs getPeersByTopic(std::string const& topic);

    bool isSessionInNodeIDList(NodeID const& targetNodeID, NodeIDs const& nodeIDs);

    std::map<NodeIPEndpoint, NodeID> m_staticNodes;
    RecursiveMutex x_nodes;

    std::shared_ptr<Host> m_host;

    std::unordered_map<NodeID, P2PSession::Ptr> m_sessions;
    RecursiveMutex x_sessions;

    std::atomic<uint32_t> m_topicSeq = {0};
    std::shared_ptr<std::vector<std::string>> m_topics;
    RecursiveMutex x_topics;

    ///< key is the group that the node joins
    ///< value is the list of node members for the group
    ///< the data is currently statically loaded and not synchronized between nodes
    std::map<GROUP_ID, h512s> m_groupID2NodeList;

    std::shared_ptr<std::unordered_map<uint32_t, CallbackFuncWithSession>> m_protocolID2Handler;
    RecursiveMutex x_protocolID2Handler;

    ///< A call B, the function to call after the request is received by B in topic.
    std::shared_ptr<std::unordered_map<std::string, CallbackFuncWithSession>> m_topic2Handler;
    RecursiveMutex x_topic2Handler;

    std::shared_ptr<P2PMessageFactory> m_p2pMessageFactory;
    KeyPair m_alias;

    std::shared_ptr<boost::asio::deadline_timer> m_timer;

    bool m_run = false;
};

}  // namespace p2p

}  // namespace dev
