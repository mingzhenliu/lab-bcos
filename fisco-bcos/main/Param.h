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
#include <libdevcore/Common.h>
#include <libethcore/Common.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <memory>
class MainParams
{
public:
    MainParams(boost::program_options::variables_map const& vm) : m_vmMap(vm) { loadConfigs(); }
    std::string const& configPath() const { return m_configPath; }

private:
    void loadConfigs() { setConfigPath(); }
    void setConfigPath()
    {
        if (m_vmMap.count("config") || m_vmMap.count("c"))
            m_configPath = m_vmMap["config"].as<std::string>();
    }

private:
    std::string m_configPath = "config.ini";
    boost::program_options::variables_map m_vmMap;
};

static void version()
{
    std::cout << "FISCO-BCOS version " << dev::Version << "\n";
    std::cout << "FISCO-BCOS network protocol version: " << dev::eth::c_protocolVersion << "\n";
    std::cout << "Client database version: " << dev::eth::c_databaseVersion << "\n";
    std::cout << "Build: " << DEV_QUOTED(ETH_BUILD_PLATFORM) << "/" << DEV_QUOTED(ETH_BUILD_TYPE)
              << "\n";
    exit(0);
}

static MainParams initCommandLine(int argc, const char* argv[])
{
    boost::program_options::options_description main_options("Main for lab-bcos");
    main_options.add_options()("help,h", "help of lab-bcos")("version,v", "version of lab-bcos")(
        "config,c", boost::program_options::value<std::string>(), "configuration path");
    boost::program_options::variables_map vm;
    try
    {
        boost::program_options::store(
            boost::program_options::parse_command_line(argc, argv, main_options), vm);
    }
    catch (...)
    {
        std::cout << "invalid input" << std::endl;
        exit(0);
    }
    /// help information
    if (vm.count("help") || vm.count("h"))
    {
        std::cout << main_options << std::endl;
        exit(0);
    }
    /// version information
    if (vm.count("version") || vm.count("v"))
    {
        version();
        exit(0);
    }
    MainParams m_params(vm);
    return m_params;
}