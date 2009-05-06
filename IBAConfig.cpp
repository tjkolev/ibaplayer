/***************************************************************************
 *   Copyright (C) 2009 by TJ Kolev                                        *
 *   tjkolev@yahoo.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "IBAConfig.h"
#include "IBALogger.h"

IBAConfig::IBAConfig()
    : m_cfgFileName(IBA_CONFIG_FILE)
{}

IBAConfig::~IBAConfig()
{}

IBAConfig IBAConfig::_theConfig;
IBAConfig& IBAConfig::Config()
{
	return _theConfig;
}

bool IBAConfig::init(string& cfgFileName)
{
    m_cfgFileName = cfgFileName;
    setDefaultParams();
    load();

    return true;
} //init()


void IBAConfig::load()
{
    if("" != m_cfgFileName)
    {
        std::ifstream inFile;
        inFile.open(m_cfgFileName.c_str());
        if(inFile.is_open())
        {
            m_cfg << inFile;
            inFile.close();
            return;
        }
    }

	Log("Unable to load configuration file. Using defaults.", IBALogger::LOGS_WARNING);
} // load()


void IBAConfig::setDefaultParams()
{

    m_defaults[PRMS_LIB_PATH] = "~";
    m_defaults[PRMS_PAGE_SIZE] = "1.0";
    m_defaults[PRMS_LOG_FILE] = "IBAPlayer.log";
    m_defaults[PRMS_SVR_LEVEL] = "7"; //IBALogger::LOGS_CRASH + IBALogger::LOGS_ERROR + IBALogger::LOGS_WARNING
    m_defaults[PRMS_ALSAPLAYER_NAME] = "IBusPlayer";
    m_defaults[PRMS_TAG_SEPARATOR] = "*";
    m_defaults[PRMS_IBUS_PORT] = "/dev/ttyS0";
    m_defaults[PRMS_IBUS_RESPONSE_DELAY] = "150";
    m_defaults[PRMS_IBUS_MONITOR_MODE] = "No";
    m_defaults[PRMS_LOG_IBUS] = "0";
    m_defaults[PRMS_IBUS_ANNOUNCE_COUNT] = "6";
    m_defaults[PRMS_IBUS_ANNOUNCE_IVL] = "30";
    m_defaults[PRMS_SCROLL_TRACK_INFO] = "No";
    m_defaults[PRMS_TRACK_POS_IVL] = "1";
    m_defaults[PRMS_TRACK_INFO_IVL] = "1";
    m_defaults[PRMS_TRACK_INFO_SCROLL_CHARS] = "2";
} // IBAConfig::fillDefaultParams()

