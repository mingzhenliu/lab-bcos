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
 * @brief: empty framework for main of lab-bcos
 *
 * @file: ParamParse.h
 * @author: chaychen
 * @date 2018-10-09
 */

#pragma once
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/easylog.h>
#include <libethcore/CommonJS.h>
#include <libethcore/Protocol.h>
#include <boost/program_options.hpp>
#include <memory>
#include <libnetwork/Host.h>
#include <libp2p/Service.h>

INITIALIZE_EASYLOGGINGPP
using namespace dev;
using namespace dev::p2p;
using namespace dev::eth;
namespace js = json_spirit;
class Params
{
public:
    Params() : m_txSpeed(10), m_groupSize(1) {}

    Params(boost::program_options::variables_map const& vm,
        boost::program_options::options_description const& option)
      : m_txSpeed(10), m_groupSize(1)
    {
        initParams(vm, option);
    }

    void initParams(boost::program_options::variables_map const& vm,
        boost::program_options::options_description const& option)
    {
        if (vm.count("txSpeed") || vm.count("t"))
            m_txSpeed = vm["txSpeed"].as<float>();
        if (vm.count("groupSize") || vm.count("n"))
            m_groupSize = vm["groupSize"].as<int>();
    }

    int groupSize() const { return m_groupSize; }
    float txSpeed() { return m_txSpeed; }

private:
    float m_txSpeed;
    dev::GROUP_ID m_groupSize;
};

static Params initCommandLine(int argc, const char* argv[])
{
    boost::program_options::options_description server_options("p2p module of FISCO-BCOS");
    server_options.add_options()("txSpeed,t", boost::program_options::value<float>(),
        "transaction generate speed")("groupSize,n", boost::program_options::value<int>(),
        "group size")("help,h", "help of p2p module of FISCO-BCOS");

    boost::program_options::variables_map vm;
    try
    {
        boost::program_options::store(
            boost::program_options::parse_command_line(argc, argv, server_options), vm);
    }
    catch (...)
    {
        std::cout << "invalid input" << std::endl;
        exit(0);
    }
    /// help information
    if (vm.count("help") || vm.count("h"))
    {
        std::cout << server_options << std::endl;
        exit(0);
    }
    Params m_params(vm, server_options);
    return m_params;
}
