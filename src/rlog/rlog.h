/*
    (C) Copyright Milan Svoboda 2009.
    
    This file is part of FuseCompress.

    FuseCompress is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    FuseCompress is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FuseCompress.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RLOG_H
#define RLOG_H

#include <cstdio>
#include <syslog.h>
#include <stdarg.h>
#include <string>

namespace rlog {

class RLog
{
	int m_level;
	bool m_toConsole;
public:
	RLog(const char *name, int level, bool toConsole)
	{
		m_level = level;
		m_toConsole = toConsole;

		if (!m_toConsole)
			openlog(name, 0, 0);
	}

	~RLog()
	{
		if (!m_toConsole)
			closelog();
	}

	void setLevel(int level)
	{
		m_level = level;
	}

	void log(int level, const char *fmt, ...)
	{
		if (level > m_level)
			return;

		va_list ap;

		va_start(ap, fmt);

		// Send debug messages only to console.

		if (m_toConsole || (level == LOG_DEBUG))
		{
			vprintf(fmt, ap);
		}
		else
		{
			vsyslog(level, fmt, ap);
		}

		va_end(ap);
	}
};

}

extern rlog::RLog *g_RLog;

#define rError(fmt, ...)   g_RLog->log(LOG_ERR, fmt "\n", ## __VA_ARGS__)
#define rWarning(fmt, ...) g_RLog->log(LOG_WARNING, fmt "\n", ## __VA_ARGS__)
#define rDebug(fmt, ...)   g_RLog->log(LOG_DEBUG, fmt "\n", ## __VA_ARGS__)
#define rInfo(fmt, ...)    g_RLog->log(LOG_INFO, fmt "\n", ## __VA_ARGS__)

#endif

