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
#include "IBALogger.h"

#include <sys/time.h>
#include <time.h>

IBALogger IBALogger::_theLogger;
IBALogger& IBALogger::Logger()
{
	return _theLogger;
}

IBALogger::IBALogger()
    : m_out(&cout),
      m_svrLevel(LOGS_NONE)
{
}

IBALogger::~IBALogger()
{
    if(m_fout.is_open())
        m_fout.close();
}

char IBALogger::getSvrLevelChar(Severity svr)
{
    switch(svr)
    {
        case LOGS_CRASH:    return 'X';
        case LOGS_ERROR:    return 'E';
        case LOGS_WARNING:  return 'W';
        case LOGS_INFO:     return 'I';
        case LOGS_DEBUG:    return 'D';
        default:            return '?';
    }
}

IBALogger::Severity IBALogger::getLogLevel()
{
    return m_svrLevel;
}

void IBALogger::setLogLevel(Severity severityLevel)
{
	string msg("Severity level set to ");
	msg += getSvrLevelChar(severityLevel);
	Log(msg, LOGS_DEBUG);
    m_svrLevel = severityLevel;
}

void IBALogger::setLogOutput(Output out, const string& fileName)
{
    if(m_fout.is_open())
        m_fout.close();

    switch(out)
    {
        case LOGO_NONE:
            m_out = NULL;
            break;
        case LOGO_CONSOLE:
            m_out = &cout;
            break;
        case LOGO_STDERR:
            m_out = &cerr;
            break;
        case LOGO_FILE:
        {
            if("" != fileName)
            {
                m_fout.open(fileName.c_str(), ofstream::out | ofstream::app);
                if(m_fout.is_open())
                {
                    m_out = &m_fout;
                    break;
                }
            }

            m_out = &cout;
            break;
        }
    }

	LogMessage(""); //separator
}

void IBALogger::log(const char* log_msg, Severity sev)
{
    if(NULL == m_out || NULL == log_msg)
        return;

    if(sev <= LOGS_NONE || sev < m_svrLevel)
		return;

	LogMessage(log_msg, sev);
}

// logs the message regardless of set severity level
void IBALogger::LogMessage(const char* log_msg, Severity sev)
{
    time_t now;
    time(&now);
    struct tm tmResult;
    localtime_r(&now, &tmResult);
    char buf[21];
    strftime(buf, 21, "%F %T", &tmResult);

    (*m_out) << buf;
    (*m_out) << ' ';
    (*m_out) << getSvrLevelChar(sev);
    (*m_out) << ": ";
    (*m_out) << log_msg;
    (*m_out) << endl;
}

void Log(const string& msg, IBALogger::Severity svr)
{
	IBALogger::Logger().log(msg.c_str(), svr);
}

void Log(const char* msg, IBALogger::Severity svr)
{
	IBALogger::Logger().log(msg, svr);
}

