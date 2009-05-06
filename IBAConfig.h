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

#ifndef _IBA_CONFIG_H_
#define _IBA_CONFIG_H_

#define IBA_CONFIG_FILE                 "IBAPlayer.conf"

// parameter tags
#define PRMS_LIB_PATH                   "MusicLibPath"
#define PRMS_PAGE_SIZE                  "PageSize"
#define PRMS_LOG_FILE                   "LogFile"
#define PRMS_SVR_LEVEL                  "SeverityLevel"
#define PRMS_ALSAPLAYER_NAME            "AlsaPlayerName"
#define PRMS_TAG_SEPARATOR              "TagSeparator"
#define PRMS_IBUS_PORT                  "IBusPort"
#define PRMS_IBUS_RESPONSE_DELAY        "ResponseDelay"
#define PRMS_IBUS_MONITOR_MODE          "MonitorMode"
#define PRMS_LOG_IBUS                   "IBusLogLevel"
#define PRMS_IBUS_ANNOUNCE_COUNT        "AnnounceCount"
#define PRMS_IBUS_ANNOUNCE_IVL          "AnnounceInterval"
#define PRMS_SCROLL_TRACK_INFO          "ScrollTrackInfo"
#define PRMS_TRACK_POS_IVL              "TrackPositionInterval"
#define PRMS_TRACK_INFO_IVL             "TrackInfoInterval"
#define PRMS_TRACK_INFO_SCROLL_CHARS    "TrackInfoScrollChars"

#include <string>

#include "ConfigFile.h"

using namespace std;

class IBAConfig
{
public:

	static IBAConfig&	Config();
	virtual         ~IBAConfig();

    bool            init(string& cfgFileName);

	template <typename T>
	T GetValue(const string& paramName);

private:

	IBAConfig();
	static IBAConfig	_theConfig;

	std::map<string,string> 	m_defaults;
	ConfigFile					m_cfg;

    string           m_cfgFileName;

    void            setDefaultParams();
    void            load();

}; // class IBAConfig

template <typename T>
T IBAConfig::GetValue(const string& paramName)
{
	T defaultValue = ConfigFile::string_as_T<T>(m_defaults[paramName]);
	return m_cfg.read<T>(paramName, defaultValue);
}

template <typename T>
T GetConfigValue(const string& paramName)
{
	return IBAConfig::Config().GetValue<T>(paramName);
}

#endif // IBACONFIG_H
