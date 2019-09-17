/*
 * Copyright (C) 2010 Andrew Kozin AKA Stanson <me@stanson.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "conf.h"

void conf_set_val( struct _conf_params * p, char * s, char end )
{
    char	*d = p->v_str;

    switch( p->type )
    {
	case 'b': *(p->v_bool)  = strncmp( "yes" , s, 3 ) ? 0 : 1; break;
	case 'u': *(p->v_ulong) = strtoul( s, NULL, 0 ); break;
	case 'l': *(p->v_long)  = strtol( s, NULL, 0 ); break;
	case 's': while( *s && *s != '\n' && *s != end ) *d++ = *s++; *d = 0; break;
    }
}

void conf_read_val( char * s, struct _conf_params * params )
{
    char	end = ' ';
    int		len;
    
    if(s)
    {
	while( *s && isspace( *s ) ) s++;
	if( !*s ) return;
	len = strlen( params->name );
	if( strncmp( s, params->name, len ) ) return;
	s += len;
	while( *s && isspace( *s ) ) s++;
	if( *s && *s != '\n' )
	{
	    if( *s == '"' )  { s++; end = '"'; }
	    if( *s == '\'' ) { s++; end = '\''; }
	    conf_set_val( params, s, end );
	}
    }
}

void conf_set_defaults( struct _conf_params * params )
{
    struct _conf_params	*p;

    for( p = params; p->name; p++ )
    {
	conf_set_val( p, p->d_val, 0 );
    }
}

void conf_parse( char * conf_name, struct _conf_params * params )
{
    FILE		*stream;
    char		str[4096];
    struct _conf_params	*p;

    conf_set_defaults( params );

    stream = fopen( conf_name, "r" );

    if( stream )
    {
	while( fgets( str, sizeof(str), stream ) )
	{
	    if(str[0] == '\n' || str[0] == '#') continue;
	    for( p = params; p->name; p++ )
		conf_read_val( str, p );
	}
	fclose(stream);
    }
}

char * conf_val2text( struct _conf_params * p, char * buf, int len )
{
    switch( p->type )
    {
	case 'b': snprintf( buf, len, "%s", *(p->v_bool) ? "yes" : "no" ); break;
	case 'u': snprintf( buf, len, "%lu", *(p->v_ulong) ); break;
	case 'l': snprintf( buf, len, "%ld", *(p->v_long) ); break;
	case 's': snprintf( buf, len, "%s", p->v_str ); break;
	default:  snprintf( buf, len, "unknown type" ); break;
    }
    return buf;
}
