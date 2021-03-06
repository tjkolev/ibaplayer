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
#ifndef _IBA_LOGGER_H_
#define _IBA_LOGGER_H_

#include <iostream>
#include <fstream>

using namespace std;

class IBALogger
{
public:

	static IBALogger& Logger();

    virtual ~IBALogger();

    enum Severity
    {
        LOGS_CRASH     = 5,
        LOGS_ERROR     = 4,
        LOGS_WARNING   = 3,
        LOGS_INFO      = 2,
        LOGS_DEBUG     = 1,
        LOGS_NONE      = 0,
    };

    enum Output
    {
        LOGO_NONE = 0,
        LOGO_CONSOLE,
        LOGO_STDERR,
        LOGO_FILE
    };

    void		log(const char*, Severity svr = LOGS_INFO);

    void		setLogOutput(Output, const string& fileName);
    void		setLogLevel(Severity);
    Severity	getLogLevel();


private:

    IBALogger();
	static IBALogger	_theLogger;
    ostream*		m_out;
    ofstream		m_fout;
    Severity		m_svrLevel;

    char            getSvrLevelChar(Severity);
    void			LogMessage(const char*, Severity svr = LOGS_INFO);
};

void Log(const char*, IBALogger::Severity svr = IBALogger::LOGS_INFO);
void Log(const string&, IBALogger::Severity svr = IBALogger::LOGS_INFO);

#endif // IBALOGGER_H
