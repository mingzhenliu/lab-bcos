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

/**
 * @brief : class for pbft request cache
 * @file: PBFTReqCache.h
 * @author: yujiechen
 *
 * @date: 2018-09-30
 */
#pragma once
#include <libconsensus/pbft/Common.h>
#include <libdevcore/easylog.h>
namespace dev
{
namespace consensus
{
class PBFTReqCache : public std::enable_shared_from_this<PBFTReqCache>
{
public:
    /// specified prepareRequest exists in raw-prepare-cache or not?
    /// @return true : the prepare request exists in the  raw-prepare-cache
    /// @return false : the prepare request doesn't exist in the  raw-prepare-cache
    inline bool isExistPrepare(PrepareReq const& req)
    {
        return m_rawPrepareCache.block_hash == req.block_hash;
    }

    /// specified SignReq exists in the sign-cache or not?
    inline bool isExistSign(SignReq const& req)
    {
        return cacheExists(m_signCache, req.block_hash, req.sig.hex());
    }

    /// specified commitReq exists in the commit-cache or not?
    inline bool isExistCommit(CommitReq const& req)
    {
        return cacheExists(m_commitCache, req.block_hash, req.sig.hex());
    }

    /// specified viewchangeReq exists in the viewchang-cache or not?
    inline bool isExistViewChange(ViewChangeReq const& req)
    {
        return cacheExists(m_recvViewChangeReq, req.view, req.idx);
    }

    /// get the size of the cached sign requests according to given block hash
    inline u256 const getSigCacheSize(h256 const& blockHash)
    {
        return getSizeFromCache(blockHash, m_signCache);
    }
    /// get the size of the cached commit requests according to given block hash
    inline u256 const getCommitCacheSize(h256 const& blockHash)
    {
        return getSizeFromCache(blockHash, m_commitCache);
    }
    /// get the size of cached viewchange requests according to given view
    inline u256 const getViewChangeSize(u256 const& toView)
    {
        return getSizeFromCache(toView, m_recvViewChangeReq);
    }

    template <typename T, typename S>
    inline u256 const getSizeFromCache(T const& key, S& cache)
    {
        if (cache.count(key) > 0)
            return u256(cache[key].size());
        return u256(0);
    }

    inline PrepareReq const& rawPrepareCache() { return m_rawPrepareCache; }
    inline PrepareReq const& prepareCache() { return m_prepareCache; }
    inline PrepareReq const& committedPrepareCache() { return m_committedPrepareCache; }
    PrepareReq* mutableCommittedPrepareCache() { return &m_committedPrepareCache; }
    inline PrepareReq const& futurePrepareCache() { return m_futurePrepareCache; }
    /// add specified raw-prepare-request into the raw-prepare-cache
    /// reset the prepare-cache
    inline void addRawPrepare(PrepareReq const& req)
    {
        m_rawPrepareCache = req;
        LOG(DEBUG) << "addRawPrepare: current raw_prepare:" << req.block_hash.abridged()
                   << "| reset prepare cache";
        m_prepareCache = PrepareReq();
    }

    /// add prepare request to prepare-cache
    /// remove cached request with the same block_hash but inconsistent view compaired with the
    /// specified prepare-request from the sign-cache and commit-cache
    inline void addPrepareReq(PrepareReq const& req)
    {
        m_prepareCache = req;
        removeInvalidSignCache(req.block_hash, req.view);
        removeInvalidCommitCache(req.block_hash, req.view);
    }
    /// add specified signReq to the sign-cache
    inline void addSignReq(SignReq const& req) { m_signCache[req.block_hash][req.sig.hex()] = req; }
    /// add specified commit cache to the commit-cache
    inline void addCommitReq(CommitReq const& req)
    {
        m_commitCache[req.block_hash][req.sig.hex()] = req;
    }
    /// add specified viewchange cache to the viewchange-cache
    inline void addViewChangeReq(ViewChangeReq const& req)
    {
        m_recvViewChangeReq[req.view][req.idx] = req;
    }

    template <typename T, typename S>
    inline void addReq(T const& req, S& cache)
    {
        cache[req.block_hash][req.sig.hex()] = req;
    }

    /// add future-prepare cache
    inline void addFuturePrepareCache(PrepareReq const& req)
    {
        if (m_futurePrepareCache.block_hash != req.block_hash)
        {
            m_futurePrepareCache = req;
            LOG(INFO) << "recvFutureBlock, blk=" << req.height << ", hash=" << req.block_hash
                      << ", idx=" << req.idx;
        }
    }
    /// update m_committedPrepareCache to m_rawPrepareCache before broadcast the commit-request
    inline void updateCommittedPrepare() { m_committedPrepareCache = m_rawPrepareCache; }
    /// obtain the sig-list from m_commitCache, and append the sig-list to given block
    bool generateAndSetSigList(dev::eth::Block& block, u256 const& minSigSize);
    ///  determine can trigger viewchange or not
    bool canTriggerViewChange(u256& minView, u256 const& minInvalidNodeNum, u256 const& toView,
        dev::eth::BlockHeader const& highestBlock, int64_t const& consensusBlockNumber);

    /// trigger viewchange
    inline void triggerViewChange(u256 const& curView)
    {
        m_rawPrepareCache.clear();
        m_prepareCache.clear();
        m_signCache.clear();
        m_commitCache.clear();
        removeInvalidViewChange(curView);
    }
    /// delete requests cached in m_signCache, m_commitCache and m_prepareCache according to hash
    void delCache(h256 const& hash);
    inline void collectGarbage(dev::eth::BlockHeader const& highestBlockHeader)
    {
        removeInvalidEntryFromCache(highestBlockHeader, m_signCache);
        removeInvalidEntryFromCache(highestBlockHeader, m_commitCache);
    }
    /// remove invalid view-change requests according to view and the current block header
    void removeInvalidViewChange(u256 const& view, dev::eth::BlockHeader const& highestBlock);
    inline void delInvalidViewChange(dev::eth::BlockHeader const& curHeader)
    {
        removeInvalidEntryFromCache(curHeader, m_recvViewChangeReq);
    }
    inline void clearAllExceptCommitCache()
    {
        m_prepareCache.clear();
        m_signCache.clear();
        m_recvViewChangeReq.clear();
    }

    inline void clearAll()
    {
        m_rawPrepareCache.clear();
        clearAllExceptCommitCache();
        m_commitCache.clear();
    }
    void resetFuturePrepare() { m_futurePrepareCache = PrepareReq(); }
    /// complemented functions for UTs
    std::unordered_map<h256, std::unordered_map<std::string, SignReq>>& mutableSignCache()
    {
        return m_signCache;
    }
    std::unordered_map<h256, std::unordered_map<std::string, CommitReq>>& mutableCommitCache()
    {
        return m_commitCache;
    }
    std::unordered_map<u256, std::unordered_map<u256, ViewChangeReq>>& mutableViewChangeCache()
    {
        return m_recvViewChangeReq;
    }

private:
    /// remove invalid requests cached in cache according to curretn block
    template <typename T, typename U, typename S>
    void inline removeInvalidEntryFromCache(dev::eth::BlockHeader const& highestBlockHeader,
        std::unordered_map<T, std::unordered_map<U, S>>& cache)
    {
        for (auto it = cache.begin(); it != cache.end();)
        {
            for (auto cache_entry = it->second.begin(); cache_entry != it->second.end();)
            {
                /// delete expired cache
                if (cache_entry->second.height < highestBlockHeader.number())
                    cache_entry = it->second.erase(cache_entry);
                /// in case of faked block hash
                else if (cache_entry->second.height == highestBlockHeader.number() &&
                         cache_entry->second.block_hash != highestBlockHeader.hash())
                    cache_entry = it->second.erase(cache_entry);
                else
                    cache_entry++;
            }
            if (it->second.size() == 0)
                it = cache.erase(it);
            else
                it++;
        }
    }

    inline void removeInvalidViewChange(u256 const& curView)
    {
        for (auto it = m_recvViewChangeReq.begin(); it != m_recvViewChangeReq.end();)
        {
            if (it->first <= curView)
                it = m_recvViewChangeReq.erase(it);
            else
                it++;
        }
    }
    /// remove sign cache according to block hash and view
    void removeInvalidSignCache(h256 const& blockHash, u256 const& view);
    /// remove commit cache according to block hash and view
    void removeInvalidCommitCache(h256 const& blockHash, u256 const& view);

    template <typename T, typename U, typename S>
    inline bool cacheExists(T const& cache, U const& mainKey, S const& key)
    {
        auto it = cache.find(mainKey);
        if (it == cache.end())
            return false;
        return (it->second.find(key)) != (it->second.end());
    }

private:
    /// cache for prepare request
    PrepareReq m_prepareCache = PrepareReq();
    /// cache for raw prepare request
    PrepareReq m_rawPrepareCache;
    /// cache for signReq(maps between hash and sign requests)
    std::unordered_map<h256, std::unordered_map<std::string, SignReq>> m_signCache;
    /// cache for received-viewChange requests(maps between view and view change requests)
    std::unordered_map<u256, std::unordered_map<u256, ViewChangeReq>> m_recvViewChangeReq;
    /// cache for commited requests(maps between hash and commited requests)
    std::unordered_map<h256, std::unordered_map<std::string, CommitReq>> m_commitCache;
    /// cache for prepare request need to be backup and saved
    PrepareReq m_committedPrepareCache;
    /// cache for the future prepare cache
    PrepareReq m_futurePrepareCache;
};
}  // namespace consensus
}  // namespace dev
