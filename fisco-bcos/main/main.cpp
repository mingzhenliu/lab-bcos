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
 * @file: main.cpp
 * @author: yujiechen
 * @date 2018-08-24
 */
#include "ExitHandler.h"
#include "Param.h"
#include <libdevcore/easylog.h>
#include <libinitializer/Initializer.h>
#include <libinitializer/LogInitializer.h>
#include <clocale>
INITIALIZE_EASYLOGGINGPP
using namespace dev::initializer;
void setDefaultOrCLocale()
{
#if __unix__
    if (!std::setlocale(LC_ALL, ""))
    {
        setenv("LC_ALL", "C", 1);
    }
#endif
}
int main(int argc, const char* argv[])
{
    /// set LC_ALL
    setDefaultOrCLocale();
    /// init params
    MainParams param = initCommandLine(argc, argv);
    /// callback initializer to init all ledgers
    auto initialize = std::make_shared<Initializer>();
    initialize->init(param.configPath());
    ExitHandler exitHandler;
    signal(SIGABRT, &ExitHandler::exitHandler);
    signal(SIGTERM, &ExitHandler::exitHandler);
    signal(SIGINT, &ExitHandler::exitHandler);

    while (!exitHandler.shouldExit())
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        LogInitializer::logRotateByTime();
    }
}
