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
/** @file P2PInitializer.h
 *  @author chaychen
 *  @modify first draft
 *  @date 20181022
 */

#pragma once

#include "Common.h"
#include "SecureInitiailizer.h"
#include <libp2p/Service.h>

namespace dev
{
namespace initializer
{
class P2PMessageFactory : public MessageFactory
{
public:
    virtual ~P2PMessageFactory() {}
    virtual Message::Ptr buildMessage() override
    {
        std::cout << "### begine build message" << std::endl;
        return std::make_shared<Message>();
    }
};

class P2PInitializer : public std::enable_shared_from_this<P2PInitializer>
{
public:
    typedef std::shared_ptr<P2PInitializer> Ptr;

    void initConfig(boost::property_tree::ptree const& _pt);

    std::shared_ptr<Service> p2pService() { return m_p2pService; }

    void setSSLContext(std::shared_ptr<bas::context> _SSLContext) { m_SSLContext = _SSLContext; }

    void setKeyPair(KeyPair const& _keyPair) { m_keyPair = _keyPair; }

private:
    std::shared_ptr<Service> m_p2pService;
    std::shared_ptr<bas::context> m_SSLContext;
    KeyPair m_keyPair;
};

}  // namespace initializer

}  // namespace dev