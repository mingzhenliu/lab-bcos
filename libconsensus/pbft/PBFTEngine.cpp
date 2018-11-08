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
 * @brief : implementation of PBFT consensus
 * @file: PBFTEngine.cpp
 * @author: yujiechen
 * @date: 2018-09-28
 */
#include "PBFTEngine.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonJS.h>
#include <libdevcore/Worker.h>
#include <libethcore/CommonJS.h>
#include <libstorage/Storage.h>
using namespace dev::eth;
using namespace dev::db;
using namespace dev::blockverifier;
using namespace dev::blockchain;
using namespace dev::p2p;
using namespace dev::storage;
namespace dev
{
namespace consensus
{
const std::string PBFTEngine::c_backupKeyCommitted = "committed";
const std::string PBFTEngine::c_backupMsgDirName = "pbftMsgBackup";

void PBFTEngine::start()
{
    initPBFTEnv(3 * getIntervalBlockTime());
    ConsensusEngineBase::start();
    PBFTENGINE_LOG(INFO) << "[#Start PBFTEngine...]" << std::endl;
    PBFTENGINE_LOG(INFO) << "[#ConsensusStatus]:  " << consensusStatus() << std::endl;
}

void PBFTEngine::initPBFTEnv(unsigned view_timeout)
{
    Guard l(m_mutex);
    resetConfig();
    m_consensusBlockNumber = 0;
    m_view = m_toView = u256(0);
    m_leaderFailed = false;
    initBackupDB();
    m_timeManager.initTimerManager(view_timeout);
    m_connectedNode = m_nodeNum;
    PBFTENGINE_LOG(INFO) << "[#PBFT init env success]" << std::endl;
}

bool PBFTEngine::shouldSeal()
{
    if (m_cfgErr || m_accountType != NodeAccountType::MinerAccount)
        return false;
    /// check leader
    std::pair<bool, u256> ret = getLeader();
    if (!ret.first)
        return false;
    /// fast view change
    if (ret.second != m_idx)
    {
        /// If the node is a miner and is not the leader, then will trigger fast viewchange if it
        /// is not connect to leader.
        h512 node_id = getMinerByIndex(ret.second.convert_to<size_t>());
        if (node_id != h512() && !m_service->isConnected(node_id))
        {
            /// PBFTENGINE_LOG(DEBUG) << "[shouldSeal:Leader Unconnected] trigger fastview change"
            ///                      << std::endl;
            m_timeManager.m_lastConsensusTime = 0;
            m_timeManager.m_lastSignTime = 0;
            m_signalled.notify_all();
        }
        return false;
    }
    if (m_reqCache->committedPrepareCache().height == m_consensusBlockNumber)
    {
        if (m_reqCache->rawPrepareCache().height != m_consensusBlockNumber)
        {
            rehandleCommitedPrepareCache(m_reqCache->committedPrepareCache());
        }
        return false;
    }
    return true;
}

/**
 * @brief: rehandle the unsubmitted committedPrepare
 * @param req: the unsubmitted committed prepareReq
 */
void PBFTEngine::rehandleCommitedPrepareCache(PrepareReq const& req)
{
    PBFTENGINE_LOG(INFO) << "[#shouldSeal:rehandleCommittedPrepare] Post out "
                            "committed-but-not-saved block: [hash/height]:  "
                         << req.block_hash.abridged() << "/" << req.height << std::endl;
    m_broadCastCache->clearAll();
    PrepareReq prepare_req(req, m_keyPair, m_view, m_idx);
    bytes prepare_data;
    prepare_req.encode(prepare_data);
    /// broadcast prepare message
    broadcastMsg(PrepareReqPacket, prepare_req.block_hash.hex(), ref(prepare_data));
    handlePrepareMsg(prepare_req, true);
}

/// recalculate m_nodeNum && m_f && m_cfgErr(must called after setSigList)
void PBFTEngine::resetConfig()
{
    m_idx = u256(-1);
    updateMinerList();
    {
        ReadGuard l(m_minerListMutex);
        for (size_t i = 0; i < m_minerList.size(); i++)
        {
            if (m_minerList[i] == m_keyPair.pub())
            {
                m_accountType = NodeAccountType::MinerAccount;
                m_idx = u256(i);
                break;
            }
        }
        m_nodeNum = u256(m_minerList.size());
    }
    m_f = (m_nodeNum - u256(1)) / u256(3);
    m_cfgErr = (m_idx == u256(-1));
}

/// init pbftMsgBackup
void PBFTEngine::initBackupDB()
{
    /// try-catch has already been considered by libdevcore/LevelDB.*
    std::string path = getBackupMsgPath();
    boost::filesystem::path path_handler = boost::filesystem::path(path);
    if (!boost::filesystem::exists(path_handler))
    {
        boost::filesystem::create_directories(path_handler);
    }
    m_backupDB = std::make_shared<LevelDB>(path);
    if (!isDiskSpaceEnough(path))
    {
        PBFTENGINE_LOG(ERROR)
            << "[#initBackupDB] Not enough available of disk, please free the space and run again"
            << std::endl;
        BOOST_THROW_EXCEPTION(NotEnoughAvailableSpace());
    }
    // reload msg from db to commited-prepare-cache
    reloadMsg(c_backupKeyCommitted, m_reqCache->mutableCommittedPrepareCache());
}

/**
 * @brief: reload PBFTMsg from DB to msg according to specified key
 * @param key: key used to index the PBFTMsg
 * @param msg: save the PBFTMsg readed from the DB
 */
void PBFTEngine::reloadMsg(std::string const& key, PBFTMsg* msg)
{
    if (!m_backupDB || !msg)
        return;
    try
    {
        bytes data = fromHex(m_backupDB->lookup(key));
        if (data.empty())
        {
            LOG(ERROR) << "reloadMsg failed";
            PBFTENGINE_LOG(WARNING) << "[reloadMsg] Empty message stored" << std::endl;
            return;
        }
        msg->decode(ref(data), 0);
        PBFTENGINE_LOG(DEBUG) << "[#reloadMsg] [height/idx/hash]:  " << msg->height << "/"
                              << msg->block_hash.abridged() << "/" << msg->idx << std::endl;
    }
    catch (std::exception& e)
    {
        PBFTENGINE_LOG(ERROR) << "[#reloadMsg] Reload PBFT message from db failed:"
                              << boost::diagnostic_information(e) << std::endl;
        return;
    }
}

/**
 * @brief: backup specified PBFTMsg with specified key into the DB
 * @param _key: key of the PBFTMsg
 * @param _msg : data to backup in the DB
 */
void PBFTEngine::backupMsg(std::string const& _key, PBFTMsg const& _msg)
{
    if (!m_backupDB)
        return;
    bytes message_data;
    _msg.encode(message_data);
    try
    {
        m_backupDB->insert(_key, toHex(message_data));
    }
    catch (std::exception& e)
    {
        PBFTENGINE_LOG(ERROR) << "[#backupMsg] backupMsg for PBFT failed:  "
                              << boost::diagnostic_information(e) << std::endl;
    }
}

/// sealing the generated block into prepareReq and push its to msgQueue
bool PBFTEngine::generatePrepare(Block const& block)
{
    Guard l(m_mutex);
    PrepareReq prepare_req(block, m_keyPair, m_view, m_idx);
    bytes prepare_data;
    prepare_req.encode(prepare_data);
    /// broadcast the generated preparePacket
    bool succ = broadcastMsg(PrepareReqPacket, prepare_req.sig.hex(), ref(prepare_data));
    if (succ)
    {
        if (block.getTransactionSize() == 0 && m_omitEmptyBlock)
        {
            m_timeManager.changeView();
            m_leaderFailed = true;
            m_signalled.notify_all();
        }
        handlePrepareMsg(prepare_req);
    }
    /// reset the block according to broadcast result
    PBFTENGINE_LOG(DEBUG) << "[#generateLocalPrepare] [prepHash/prepHeight]:  "
                          << prepare_req.block_hash << "/" << prepare_req.height << std::endl;
    return succ;
}

/**
 * @brief : 1. generate and broadcast signReq according to given prepareReq,
 *          2. add the generated signReq into the cache
 * @param req: specified PrepareReq used to generate signReq
 */
bool PBFTEngine::broadcastSignReq(PrepareReq const& req)
{
    SignReq sign_req(req, m_keyPair, m_idx);
    bytes sign_req_data;
    sign_req.encode(sign_req_data);
    bool succ = broadcastMsg(SignReqPacket, sign_req.sig.hex(), ref(sign_req_data));
    if (succ)
        m_reqCache->addSignReq(sign_req);
    return succ;
}

bool PBFTEngine::getNodeIDByIndex(h512& nodeID, const u256& idx) const
{
    nodeID = getMinerByIndex(idx.convert_to<size_t>());
    if (nodeID == h512())
    {
        PBFTENGINE_LOG(ERROR) << "[#getNodeIDByIndex] Not miner [idx]:  " << idx << std::endl;
        return false;
    }
    return true;
}

bool PBFTEngine::checkSign(PBFTMsg const& req) const
{
    h512 node_id;
    if (getNodeIDByIndex(node_id, req.idx))
    {
        Public pub_id = jsToPublic(toJS(node_id.hex()));
        return dev::verify(pub_id, req.sig, req.block_hash) &&
               dev::verify(pub_id, req.sig2, req.fieldsWithoutBlock());
    }
    return false;
}

/**
 * @brief: 1. generate commitReq according to prepare req
 *         2. broadcast the commitReq
 * @param req: the prepareReq that used to generate commitReq
 */
bool PBFTEngine::broadcastCommitReq(PrepareReq const& req)
{
    CommitReq commit_req(req, m_keyPair, m_idx);
    bytes commit_req_data;
    commit_req.encode(commit_req_data);
    bool succ = broadcastMsg(CommitReqPacket, commit_req.sig.hex(), ref(commit_req_data));
    if (succ)
        m_reqCache->addCommitReq(commit_req);
    return succ;
}

bool PBFTEngine::broadcastViewChangeReq()
{
    ViewChangeReq req(m_keyPair, m_highestBlock.number(), m_toView, m_idx, m_highestBlock.hash());
    PBFTENGINE_LOG(DEBUG) << "[#broadcastViewChangeReq] [hash/higNumber]:  " << req.block_hash
                          << "/" << m_highestBlock.number() << std::endl;
    bytes view_change_data;
    req.encode(view_change_data);
    return broadcastMsg(ViewChangeReqPacket, req.sig.hex() + toJS(req.view), ref(view_change_data));
}

/**
 * @brief: broadcast specified message to all-peers with cache-filter and specified filter
 *         broadcast solutions:
 *         1. peer is not the miner: stop broadcasting
 *         2. peer is in the filter list: mark the message as broadcasted, and stop broadcasting
 *         3. the packet has been broadcasted: stop broadcast
 * @param packetType: the packet type of the broadcast-message
 * @param key: the key of the broadcast-message(is the signature of the message in common)
 * @param data: the encoded data of to be broadcasted(RLP encoder now)
 * @param filter: the list that shouldn't be broadcasted to
 */
bool PBFTEngine::broadcastMsg(unsigned const& packetType, std::string const& key,
    bytesConstRef data, std::unordered_set<h512> const& filter)
{
    auto sessions = m_service->sessionInfosByProtocolID(m_protocolId);
    m_connectedNode = u256(sessions.size());
    for (auto session : sessions)
    {
        /// get node index of the miner from m_minerList failed ?
        if (getIndexByMiner(session.nodeID) < 0)
            continue;
        /// peer is in the _filter list ?
        if (filter.count(session.nodeID))
        {
            broadcastMark(session.nodeID, packetType, key);
            continue;
        }
        /// packet has been broadcasted?
        if (broadcastFilter(session.nodeID, packetType, key))
            continue;
        /// send messages
        m_service->asyncSendMessageByNodeID(
            session.nodeID, transDataToMessage(data, packetType), nullptr);
        broadcastMark(session.nodeID, packetType, key);
    }
    return true;
}

/**
 * @brief: check the specified prepareReq is valid or not
 *       1. should not be existed in the prepareCache
 *       2. if allowSelf is false, shouldn't be generated from the node-self
 *       3. hash of committed prepare should be equal to the block hash of prepareReq if their
 * height is equal
 *       4. sign of PrepareReq should be valid(public key to verify sign is obtained according to
 * req.idx)
 * @param req: the prepareReq need to be checked
 * @param allowSelf: whether can solve prepareReq generated by self-node
 * @param oss
 * @return true: the specified prepareReq is valid
 * @return false: the specified prepareReq is invalid
 */
bool PBFTEngine::isValidPrepare(
    PrepareReq const& req, bool allowSelf, std::ostringstream& oss) const
{
    if (m_reqCache->isExistPrepare(req))
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidPrepare] Duplicated Prep: [INFO]:  " << oss.str();
        return false;
    }
    if (!allowSelf && req.idx == m_idx)
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidPrepare] Own Req: [INFO]:  " << oss.str();
        return false;
    }
    if (hasConsensused(req))
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidPrepare] Consensused Prep: [INFO]:  " << oss.str();
        return false;
    }

    if (isFutureBlock(req))
    {
        PBFTENGINE_LOG(INFO) << "[#FutureBlock] [INFO]:  " << oss.str();
        m_reqCache->addFuturePrepareCache(req);
        return false;
    }
    if (!isValidLeader(req))
    {
        return false;
    }
    if (!isHashSavedAfterCommit(req))
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidPrepare] Not saved after commit: [INFO]:  "
                                << oss.str();
        return false;
    }
    if (!checkSign(req))
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidPrepare] Invalid sig: [INFO]:  " << oss.str();
        return false;
    }
    return true;
}

/// check miner list
void PBFTEngine::checkMinerList(Block const& block)
{
    ReadGuard l(m_minerListMutex);
    if (m_minerList != block.blockHeader().sealerList())
    {
#if DEBUG
        std::string miners;
        for (auto miner : m_minerList)
            miners += miner + " ";
        LOG(DEBUG) << "Miner list = " << miners;
        PBFTENGINE_LOG(DEBUG) << "[checkMinerList] [miners]: " << miners << std::endl;
#endif
        PBFTENGINE_LOG(ERROR) << "[#checkMinerList] Wrong miners: [Cminers/CblockMiner/hash]:  "
                              << m_minerList.size() << "/"
                              << block.blockHeader().sealerList().size() << "/"
                              << block.blockHeader().hash() << std::endl;
        BOOST_THROW_EXCEPTION(
            BlockMinerListWrong() << errinfo_comment("Wrong Miner List of Block"));
    }
}

void PBFTEngine::execBlock(Sealing& sealing, PrepareReq const& req, std::ostringstream& oss)
{
    auto start_exec_time = utcTime();
    Block working_block(req.block);
    PBFTENGINE_LOG(TRACE) << "[#execBlock] [number/hash/idx]:  " << working_block.header().number()
                          << "/" << working_block.header().hash().abridged() << "/" << req.idx
                          << std::endl;
    checkBlockValid(working_block);
    m_blockSync->noteSealingBlockNumber(working_block.header().number());
    sealing.p_execContext = executeBlock(working_block);
    sealing.block = working_block;
    m_timeManager.updateTimeAfterHandleBlock(sealing.block.getTransactionSize(), start_exec_time);
}

/// check whether the block is empty
bool PBFTEngine::needOmit(Sealing const& sealing)
{
    if (sealing.block.getTransactionSize() == 0 && m_omitEmptyBlock)
    {
        PBFTENGINE_LOG(TRACE) << "[#needOmit] [number/hash]:  "
                              << sealing.block.blockHeader().number() << "/"
                              << sealing.block.blockHeader().hash() << std::endl;
        return true;
    }
    return false;
}

/**
 * @brief: this function is called when receive-given-protocol related message from the network
 *        1. check the validation of the network-received data(include the account type of the
 * sender and receiver)
 *        2. decode the data into PBFTMsgPacket
 *        3. push the message into message queue to handler later by workLoop
 * @param exception: exceptions related to the received-message
 * @param session: the session related to the network data(can get informations about the sender)
 * @param message: message constructed from data received from the network
 */
void PBFTEngine::onRecvPBFTMessage(
    P2PException exception, std::shared_ptr<Session> session, Message::Ptr message)
{
    PBFTMsgPacket pbft_msg;
    bool valid = decodeToRequests(pbft_msg, message, session);
    if (!valid)
        return;
    if (pbft_msg.packet_id <= ViewChangeReqPacket)
    {
        m_msgQueue.push(pbft_msg);
    }
    else
    {
        PBFTENGINE_LOG(WARNING) << "[#onRecvPBFTMessage] Illegal msg: [idx]:  "
                                << pbft_msg.packet_id << std::endl;
    }
}

void PBFTEngine::handlePrepareMsg(PrepareReq& prepare_req, PBFTMsgPacket const& pbftMsg)
{
    bool valid = decodeToRequests(prepare_req, ref(pbftMsg.data));
    if (!valid)
        return;
    handlePrepareMsg(prepare_req);
}

/**
 * @brief: handle the prepare request:
 *       1. check whether the prepareReq is valid or not
 *       2. if the prepareReq is valid:
 *       (1) add the prepareReq to raw-prepare-cache
 *       (2) execute the block
 *       (3) sign the prepareReq and broadcast the signed prepareReq
 *       (4) callback checkAndCommit function to determin can submit the block or not
 * @param prepare_req: the prepare request need to be handled
 * @param self: if generated-prepare-request need to handled, then set self to be true;
 *              else this function will filter the self-generated prepareReq
 */
void PBFTEngine::handlePrepareMsg(PrepareReq const& prepareReq, bool self)
{
    Timer t;
    std::ostringstream oss;
    oss << "[#handlePrepareMsg] [idx/view/number/highNum/consNum/hash]:  " << prepareReq.idx << "/"
        << prepareReq.view << "/" << prepareReq.height << "/" << m_highestBlock.number() << "/"
        << m_consensusBlockNumber << "/" << prepareReq.block_hash.abridged() << "\n";
    /// check the prepare request is valid or not
    if (!isValidPrepare(prepareReq, self, oss))
        return;
    /// add raw prepare request
    m_reqCache->addRawPrepare(prepareReq);
    Sealing workingSealing;
    try
    {
        execBlock(workingSealing, prepareReq, oss);
    }
    catch (std::exception& e)
    {
        PBFTENGINE_LOG(WARNING) << "[#handlePrepareMsg] Block execute failed: [EINFO]:  "
                                << boost::diagnostic_information(e) << "  [INFO]: " << oss.str()
                                << std::endl;
        return;
    }
    /// whether to omit empty block
    if (needOmit(workingSealing))
    {
        m_timeManager.changeView();
        m_timeManager.m_changeCycle = 0;
        m_signalled.notify_all();
        return;
    }

    /// generate prepare request with signature of this node to broadcast
    /// (can't change prepareReq since it may be broadcasted-forwarded to other nodes)
    PrepareReq sign_prepare(prepareReq, workingSealing, m_keyPair);
    m_reqCache->addPrepareReq(sign_prepare);
    /// broadcast the re-generated signReq(add the signReq to cache)
    if (!broadcastSignReq(sign_prepare))
    {
        PBFTENGINE_LOG(WARNING) << "[#broadcastSignReq failed] [INFO]:  " << oss.str();
    }
    checkAndCommit();
    PBFTENGINE_LOG(DEBUG) << "[#handlePrepareMsg Succ] [Timecost]:  " << 1000 * t.elapsed()
                          << "  [INFO]:  " << oss.str();
}


void PBFTEngine::checkAndCommit()
{
    u256 sign_size = m_reqCache->getSigCacheSize(m_reqCache->prepareCache().block_hash);
    /// must be equal to minValidNodes:in case of callback checkAndCommit repeatly in a round of
    /// PBFT consensus
    if (sign_size == minValidNodes())
    {
        PBFTENGINE_LOG(TRACE) << "[#checkAndCommit:SignReq enough] [number/sigSize/hash]:  "
                              << m_reqCache->prepareCache().height << "/" << sign_size << "/"
                              << m_reqCache->prepareCache().block_hash.abridged() << std::endl;
        if (m_reqCache->prepareCache().view != m_view)
        {
            PBFTENGINE_LOG(WARNING)
                << "[#checkAndCommit: InvalidView] [prepView/view/prepHeight/hash]:  "
                << m_reqCache->prepareCache().view << "/" << m_view << "/"
                << m_reqCache->prepareCache().height << "/"
                << m_reqCache->prepareCache().block_hash.abridged() << std::endl;
            return;
        }
        /// update and backup the commit cache
        m_reqCache->updateCommittedPrepare();
        backupMsg(c_backupKeyCommitted, m_reqCache->committedPrepareCache());
        if (!broadcastCommitReq(m_reqCache->prepareCache()))
        {
            PBFTENGINE_LOG(WARNING) << "[#checkAndCommit: broadcastCommitReq failed]" << std::endl;
        }
        m_timeManager.m_lastSignTime = utcTime();
        checkAndSave();
    }
}

/// if collect >= 2/3 SignReq and CommitReq, then callback this function to commit block
/// check whether view and height is valid, if valid, then commit the block and clear the context
void PBFTEngine::checkAndSave()
{
    u256 sign_size = m_reqCache->getSigCacheSize(m_reqCache->prepareCache().block_hash);
    u256 commit_size = m_reqCache->getCommitCacheSize(m_reqCache->prepareCache().block_hash);
    if (sign_size >= minValidNodes() && commit_size >= minValidNodes())
    {
        PBFTENGINE_LOG(TRACE) << "[#checkAndSave: CommitReq enough] [number/comitSize/hash]:  "
                              << m_reqCache->prepareCache().height << "/" << commit_size << "/"
                              << m_reqCache->prepareCache().block_hash.abridged() << std::endl;
        if (m_reqCache->prepareCache().view != m_view)
        {
            PBFTENGINE_LOG(WARNING)
                << "[#checkAndSave: InvalidView] [prepView/view/prepHeight/hash]:  "
                << m_reqCache->prepareCache().view << "/" << m_view << "/"
                << m_reqCache->prepareCache().height << "/"
                << m_reqCache->prepareCache().block_hash.abridged() << std::endl;
            return;
        }
        /// add sign-list into the block header
        if (m_reqCache->prepareCache().height > m_highestBlock.number())
        {
            Block block(m_reqCache->prepareCache().block);
            m_reqCache->generateAndSetSigList(block, minValidNodes());
            PBFTENGINE_LOG(DEBUG) << "[#checkAndSave: Consensus Succ] [number/hash/idx]:  "
                                  << m_reqCache->prepareCache().height << "/"
                                  << m_reqCache->prepareCache().block_hash.abridged() << "/"
                                  << m_reqCache->prepareCache().idx << std::endl;
            /// callback block chain to commit block
            CommitResult ret = m_blockChain->commitBlock(
                block, std::shared_ptr<ExecutiveContext>(m_reqCache->prepareCache().p_execContext));
            PBFTENGINE_LOG(DEBUG) << "[#commitBlock Succ]" << std::endl;
            /// drop handled transactions
            if (ret == CommitResult::OK)
                dropHandledTransactions(block);
            else
                m_txPool->handleBadBlock(block);
            resetConfig();
        }
        else
        {
            PBFTENGINE_LOG(WARNING)
                << "[#checkAndSave: Consensus Failed] Block already exists:  "
                   "[blkNum/number/blkHash/highHash]: "
                << m_reqCache->prepareCache().height << "/" << m_highestBlock.number() << "/"
                << m_reqCache->prepareCache().block_hash.abridged() << "/"
                << m_highestBlock.hash().abridged() << std::endl;
        }
    }
}

/// update the context of PBFT after commit a block into the block-chain
/// 1. update the highest to new-committed blockHeader
/// 2. update m_view/m_toView/m_leaderFailed/m_lastConsensusTime/m_consensusBlockNumber
/// 3. delete invalid view-change requests according to new highestBlock
/// 4. recalculate the m_nodeNum/m_f according to newer MinerList
/// 5. clear all caches related to prepareReq and signReq
void PBFTEngine::reportBlock(BlockHeader const& blockHeader)
{
    Guard l(m_mutex);
    if (m_blockChain->number() == 0 || m_highestBlock.number() < blockHeader.number())
    {
        /// update the highest block
        m_highestBlock = blockHeader;
        if (m_highestBlock.number() >= m_consensusBlockNumber)
        {
            m_view = m_toView = u256(0);
            m_leaderFailed = false;
            m_timeManager.m_lastConsensusTime = utcTime();
            m_timeManager.m_changeCycle = 0;
            m_consensusBlockNumber = m_highestBlock.number() + 1;
            /// delete invalid view change requests from the cache
            m_reqCache->delInvalidViewChange(m_highestBlock);
        }
        /// clear caches
        m_reqCache->clearAllExceptCommitCache();
        m_reqCache->delCache(m_highestBlock.hash());
        PBFTENGINE_LOG(INFO) << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^Report: number= "
                             << m_highestBlock.number() << ", idx= " << m_highestBlock.sealer()
                             << " , hash= " << m_highestBlock.hash().abridged()
                             << ", next= " << m_consensusBlockNumber;
    }
}

/**
 * @brief: 1. decode the network-received PBFTMsgPacket to signReq
 *         2. check the validation of the signReq
 *         3. submit the block into blockchain if the size of collected signReq and
 *            commitReq is over 2/3
 * @param sign_req: return value, the decoded signReq
 * @param pbftMsg: the network-received PBFTMsgPacket
 */
void PBFTEngine::handleSignMsg(SignReq& sign_req, PBFTMsgPacket const& pbftMsg)
{
    Timer t;
    bool valid = decodeToRequests(sign_req, ref(pbftMsg.data));
    if (!valid)
        return;
    std::ostringstream oss;
    oss << "[#handleSignMsg] [number/highNum/idx/Sview/view/from/hash]:  " << sign_req.height << "/"
        << m_highestBlock.number() << "/" << sign_req.idx << "/" << sign_req.view << "/" << m_view
        << "/" << pbftMsg.node_id << "/" << sign_req.block_hash.abridged() << "\n";

    valid = isValidSignReq(sign_req, oss);
    if (!valid)
        return;
    m_reqCache->addSignReq(sign_req);
    checkAndCommit();
    PBFTENGINE_LOG(DEBUG) << "[#handleSignMsg Succ] [Timecost]:  " << 1000 * t.elapsed()
                          << "  [INFO]:  " << oss.str();
}

/**
 * @brief: check the given signReq is valid or not
 *         1. the signReq shouldn't be existed in the cache
 *         2. callback checkReq to check the validation of given request
 * @param req: the given request to be checked
 * @param oss: log to debug
 * @return true: check succeed
 * @return false: check failed
 */
bool PBFTEngine::isValidSignReq(SignReq const& req, std::ostringstream& oss) const
{
    if (m_reqCache->isExistSign(req))
    {
        PBFTENGINE_LOG(WARNING) << "[#InValidSignReq] Duplicated sign: [INFO]:  " << oss.str();
        return false;
    }
    CheckResult result = checkReq(req, oss);
    if (result == CheckResult::FUTURE)
    {
        m_reqCache->addSignReq(req);
        PBFTENGINE_LOG(INFO) << "[#FutureBlock] [INFO]:  " << oss.str();
        return false;
    }
    if (result == CheckResult::INVALID)
        return false;
    return true;
}

/**
 * @brief : 1. decode the network-received message into commitReq
 *          2. check the validation of the commitReq
 *          3. add the valid commitReq into the cache
 *          4. submit to blockchain if the size of collected commitReq is over 2/3
 * @param commit_req: return value, the decoded commitReq
 * @param pbftMsg: the network-received PBFTMsgPacket
 */
void PBFTEngine::handleCommitMsg(CommitReq& commit_req, PBFTMsgPacket const& pbftMsg)
{
    Timer t;
    bool valid = decodeToRequests(commit_req, ref(pbftMsg.data));
    if (!valid)
        return;
    std::ostringstream oss;
    oss << "[#handleCommitMsg] [number/highNum/idx/Cview/view/from/hash]:  " << commit_req.height
        << "/" << m_highestBlock.number() << "/" << commit_req.idx << "/" << commit_req.view << "/"
        << m_view << "/" << pbftMsg.node_id << "/" << commit_req.block_hash.abridged() << "\n";

    valid = isValidCommitReq(commit_req, oss);
    if (!valid)
        return;
    m_reqCache->addCommitReq(commit_req);
    checkAndSave();
    PBFTENGINE_LOG(DEBUG) << "[#handleCommitMsg Succ] [Timecost]:  " << 1000 * t.elapsed()
                          << "  [INFO]:  " << oss.str();
    return;
}

/**
 * @brief: check the given commitReq is valid or not
 * @param req: the given commitReq need to be checked
 * @param oss: info to debug
 * @return true: the given commitReq is valid
 * @return false: the given commitReq is invalid
 */
bool PBFTEngine::isValidCommitReq(CommitReq const& req, std::ostringstream& oss) const
{
    if (m_reqCache->isExistCommit(req))
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidCommitReq] Duplicated: [INFO]:  " << oss.str();
        return false;
    }
    CheckResult result = checkReq(req, oss);
    if (result == CheckResult::FUTURE)
    {
        m_reqCache->addCommitReq(req);
        return false;
    }
    if (result == CheckResult::INVALID)
        return false;
    return true;
}

void PBFTEngine::handleViewChangeMsg(ViewChangeReq& viewChange_req, PBFTMsgPacket const& pbftMsg)
{
    bool valid = decodeToRequests(viewChange_req, ref(pbftMsg.data));
    if (!valid)
        return;
    std::ostringstream oss;
    oss << "[handleViewChangeMsg] [number/highNum/idx/Cview/view/from/hash]:  "
        << viewChange_req.height << "/" << m_highestBlock.number() << "/" << viewChange_req.idx
        << "/" << viewChange_req.view << "/" << m_view << "/" << pbftMsg.node_id << "/"
        << viewChange_req.block_hash.abridged() << "\n";

    valid = isValidViewChangeReq(viewChange_req, oss);
    if (!valid)
        return;

    m_reqCache->addViewChangeReq(viewChange_req);
    if (viewChange_req.view == m_toView)
        checkAndChangeView();
    else
    {
        u256 min_view = u256(0);
        bool should_trigger = m_reqCache->canTriggerViewChange(
            min_view, m_f, m_toView, m_highestBlock, m_consensusBlockNumber);
        if (should_trigger)
        {
            m_timeManager.changeView();
            m_toView = min_view - u256(1);
            PBFTENGINE_LOG(INFO)
                << "[#handleViewChangeMsg] Tigger fast-viewchange: [view/Toview/minView]:  "
                << m_view << "/" << m_toView << "/" << min_view << "  [INFO]:  " << oss.str();
            m_signalled.notify_all();
        }
    }
}

bool PBFTEngine::isValidViewChangeReq(ViewChangeReq const& req, std::ostringstream& oss)
{
    if (m_reqCache->isExistViewChange(req))
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidViewChangeReq] Duplicated: [INFO]  " << oss.str();
        return false;
    }
    if (req.idx == m_idx)
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidViewChangeReq] Own Req: [INFO]  " << oss.str();
        return false;
    }
    catchupView(req, oss);
    /// check view and block height
    if (req.height < m_highestBlock.number() || req.view <= m_view)
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidViewChangeReq] Invalid view or height: [INFO]:  "
                                << oss.str();
        return false;
    }
    /// check block hash
    if ((req.height == m_highestBlock.number() && req.block_hash != m_highestBlock.hash()) ||
        (m_blockChain->getBlockByHash(req.block_hash) == nullptr))
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidViewChangeReq] Invalid hash [highHash]:  "
                                << m_highestBlock.hash().abridged() << " [INFO]:  " << oss.str();
        return false;
    }
    if (!checkSign(req))
    {
        PBFTENGINE_LOG(WARNING) << "[#InvalidViewChangeReq] Invalid Sign [INFO]:  " << oss.str();
        return false;
    }
    return true;
}

void PBFTEngine::catchupView(ViewChangeReq const& req, std::ostringstream& oss)
{
    if (req.view + u256(1) < m_toView)
    {
        PBFTENGINE_LOG(INFO) << "[#catchupView] [toView]: " << m_toView
                             << " [INFO]:  " << oss.str();
        broadcastViewChangeReq();
    }
}

void PBFTEngine::checkAndChangeView()
{
    u256 count = m_reqCache->getViewChangeSize(m_toView);
    if (count >= minValidNodes() - u256(1))
    {
        PBFTENGINE_LOG(INFO) << "[#checkAndChangeView] [Reach consensus, to_view]:  " << m_toView
                             << std::endl;
        m_leaderFailed = false;
        m_view = m_toView;
        m_reqCache->triggerViewChange(m_view);
        m_broadCastCache->clearAll();
    }
}

/// collect all caches
void PBFTEngine::collectGarbage()
{
    Guard l(m_mutex);
    if (!m_highestBlock)
        return;
    Timer t;
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    if (now - m_timeManager.m_lastGarbageCollection >
        std::chrono::seconds(TimeManager::CollectInterval))
    {
        m_reqCache->collectGarbage(m_highestBlock);
        m_timeManager.m_lastGarbageCollection = now;
        PBFTENGINE_LOG(TRACE) << "[#collectGarbage] [Timecost]:  " << 1000 * t.elapsed()
                              << std::endl;
    }
}

void PBFTEngine::checkTimeout()
{
    bool flag = false;
    {
        Guard l(m_mutex);
        if (m_timeManager.isTimeout())
        {
            Timer t;
            m_toView += u256(1);
            m_leaderFailed = true;
            m_timeManager.updateChangeCycle();
            m_timeManager.m_lastConsensusTime = utcTime();
            flag = true;
            m_reqCache->removeInvalidViewChange(m_toView, m_highestBlock);
            PBFTENGINE_LOG(DEBUG)
                << "[#checkTimeout: broadcastViewChangeReq] [highNum/view/toView]:  "
                << m_highestBlock.number() << "/" << m_view << "/" << m_toView;
            if (!broadcastViewChangeReq())
                return;
            checkAndChangeView();
            PBFTENGINE_LOG(DEBUG) << "[#checkTimeout Succ] [timecost/view/toView]:  "
                                  << t.elapsed() * 1000 << "/" << m_view << "/" << m_toView;
        }
    }
    if (flag && m_onViewChange)
        m_onViewChange();
}

void PBFTEngine::handleMsg(PBFTMsgPacket const& pbftMsg)
{
    Guard l(m_mutex);
    PBFTMsg pbft_msg;
    std::string key;
    switch (pbftMsg.packet_id)
    {
    case PrepareReqPacket:
    {
        PrepareReq prepare_req;
        handlePrepareMsg(prepare_req, pbftMsg);
        key = prepare_req.block_hash.hex();
        pbft_msg = prepare_req;
        break;
    }
    case SignReqPacket:
    {
        SignReq req;
        handleSignMsg(req, pbftMsg);
        key = req.sig.hex();
        pbft_msg = req;
        break;
    }
    case CommitReqPacket:
    {
        CommitReq req;
        handleCommitMsg(req, pbftMsg);
        key = req.sig.hex();
        pbft_msg = req;
        break;
    }
    case ViewChangeReqPacket:
    {
        ViewChangeReq req;
        handleViewChangeMsg(req, pbftMsg);
        key = req.sig.hex() + toJS(req.view);
        pbft_msg = req;
        break;
    }
    default:
    {
        PBFTENGINE_LOG(WARNING) << "[#handleMsg] Err pbft message: [from]:  " << pbftMsg.node_idx
                                << std::endl;
        return;
    }
    }
    bool height_flag = (pbft_msg.height > m_highestBlock.number()) ||
                       (m_highestBlock.number() - pbft_msg.height < 10);
    if (key.size() > 0 && height_flag)
    {
        std::unordered_set<h512> filter;
        filter.insert(pbftMsg.node_id);
        /// get the origin gen node id of the request
        h512 gen_node_id = getMinerByIndex(pbft_msg.idx.convert_to<size_t>());
        if (gen_node_id != h512())
            filter.insert(gen_node_id);
        broadcastMsg(pbftMsg.packet_id, key, ref(pbftMsg.data), filter);
    }
}

/// start a new thread to handle the network-receivied message
void PBFTEngine::workLoop()
{
    while (isWorking())
    {
        try
        {
            std::pair<bool, PBFTMsgPacket> ret = m_msgQueue.tryPop(c_PopWaitSeconds);
            if (ret.first)
            {
                PBFTENGINE_LOG(TRACE)
                    << "[#workLoop: handleMsg] [type/idx]:  " << ret.second.packet_id << "/"
                    << ret.second.node_idx << std::endl;
                handleMsg(ret.second);
            }
            else
            {
                std::unique_lock<std::mutex> l(x_signalled);
                m_signalled.wait_for(l, std::chrono::milliseconds(5));
            }
            checkTimeout();
            handleFutureBlock();
            collectGarbage();
        }
        catch (std::exception& _e)
        {
            LOG(ERROR) << _e.what();
        }
    }
}


/// handle the prepareReq cached in the futurePrepareCache
void PBFTEngine::handleFutureBlock()
{
    Guard l(m_mutex);
    PrepareReq future_req = m_reqCache->futurePrepareCache();
    if (future_req.height == m_consensusBlockNumber && future_req.view == m_view)
    {
        PBFTENGINE_LOG(INFO) << "[#handleFutureBlock] [number/highNum/hash]:  "
                             << m_reqCache->futurePrepareCache().height << "/"
                             << m_highestBlock.number() << "/"
                             << m_reqCache->futurePrepareCache().block_hash.abridged() << std::endl;
        handlePrepareMsg(future_req);
        m_reqCache->resetFuturePrepare();
    }
}

/// get the status of PBFT consensus
const std::string PBFTEngine::consensusStatus() const
{
    json_spirit::Array status;
    json_spirit::Object statusObj;
    getBasicConsensusStatus(statusObj);
    /// get other informations related to PBFT
    /// get connected node
    statusObj.push_back(
        json_spirit::Pair("connectedNodes", m_connectedNode.convert_to<uint64_t>()));
    /// get the current view
    statusObj.push_back(json_spirit::Pair("currentView", m_view.convert_to<uint64_t>()));
    /// get toView
    statusObj.push_back(json_spirit::Pair("toView", m_toView.convert_to<uint64_t>()));
    /// get leader failed or not
    statusObj.push_back(json_spirit::Pair("leaderFailed", m_leaderFailed));
    statusObj.push_back(json_spirit::Pair("cfgErr", m_cfgErr));
    statusObj.push_back(json_spirit::Pair("omitEmptyBlock", m_omitEmptyBlock));
    status.push_back(statusObj);
    /// get cache-related informations
    m_reqCache->getCacheConsensusStatus(status);
    json_spirit::Value value(status);
    std::string status_str = json_spirit::write_string(value, true);
    return status_str;
}

void PBFTEngine::updateMinerList()
{
    if (m_storage == nullptr)
        return;
    if (m_highestBlock.number() == m_lastObtainMinerNum)
        return;
    try
    {
        UpgradableGuard l(m_minerListMutex);
        auto miner_list = m_minerList;
        int64_t curBlockNum = m_highestBlock.number();
        /// get node from storage DB
        auto nodes = m_storage->select(m_highestBlock.hash(), curBlockNum, "_sys_miners_", "node");
        /// obtain miner list
        if (!nodes)
            return;
        for (size_t i = 0; i < nodes->size(); i++)
        {
            auto node = nodes->get(i);
            if (!node)
                return;
            if ((node->getField("type") == "miner") &&
                (boost::lexical_cast<int>(node->getField("enable_num")) <= curBlockNum))
            {
                h512 nodeID = h512(node->getField("node_id"));
                if (find(miner_list.begin(), miner_list.end(), nodeID) != miner_list.end())
                {
                    miner_list.push_back(nodeID);
                    PBFTENGINE_LOG(INFO)
                        << "[#updateMinerList] Add nodeID [nodeID/idx]: " << toHex(nodeID) << "/"
                        << i << std::endl;
                }
            }
        }
        /// remove observe nodes
        for (size_t i = 0; i < miner_list.size(); i++)
        {
            auto node = nodes->get(i);
            if (!node)
                return;
            if ((node->getField("type") == "observer") &&
                (boost::lexical_cast<int>(node->getField("enable_num")) <= curBlockNum))
            {
                h512 nodeID = h512(node->getField("node_id"));
                auto it = find(miner_list.begin(), miner_list.end(), nodeID);
                if (it != miner_list.end())
                {
                    miner_list.erase(it);
                    PBFTENGINE_LOG(INFO)
                        << "[#updateMinerList] erase nodeID [nodeID/idx]:  " << toHex(nodeID) << "/"
                        << i;
                }
            }
        }

        UpgradeGuard ul(l);
        m_minerList = miner_list;
    }
    catch (std::exception& e)
    {
        PBFTENGINE_LOG(ERROR) << "[#updateMinerList] update minerList failed [EINFO]:  "
                              << boost::diagnostic_information(e);
    }
}

}  // namespace consensus
}  // namespace dev
