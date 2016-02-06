/*
 * Copyright (C) 2010 Andrew Kozin AKA Stanson <me@stanson.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <ctype.h>

extern int g_syslog;
extern int g_verbose;

void _logf( int prio, const char * format, ... )
{
    va_list  arg;

    if( g_verbose || ( prio < LOG_INFO ) )
    {
	va_start( arg, format );
	if( g_syslog ) vsyslog( prio, format, arg );
	else           vfprintf( stderr, format, arg );
	va_end( arg );
    }
}
