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
/** @file SecureInitializer.h
 *  @author chaychen
 *  @modify first draft
 *  @date 20181022
 */

#pragma once

#include "Common.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace bas = boost::asio::ssl;
namespace dev
{
namespace initializer
{
DEV_SIMPLE_EXCEPTION(PrivateKeyError);
DEV_SIMPLE_EXCEPTION(PrivateKeyNotExists);
DEV_SIMPLE_EXCEPTION(CertificateError);
DEV_SIMPLE_EXCEPTION(CertificateNotExists);

class SecureInitializer : public std::enable_shared_from_this<SecureInitializer>
{
public:
    typedef std::shared_ptr<SecureInitializer> Ptr;

    void initConfig(const boost::property_tree::ptree& _pt);

    std::shared_ptr<bas::context> SSLContext() { return m_sslContext; }
    const KeyPair& keyPair() { return m_key; }

private:
    void completePath(std::string& _path);
    KeyPair m_key;
    std::shared_ptr<boost::asio::ssl::context> m_sslContext;
};

}  // namespace initializer

}  // namespace dev
