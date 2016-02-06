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

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>

#include <strophe.h>

#include "conf.h"
#include "log.h"

#include "../config.h"

struct _xmpp {
    char		server[PATH_MAX];
    unsigned long	port;
    char		jid[256];
    char		pass[256];
    long		persist;
    long		disable_tls;
    long		require_tls;
    long		legacy_ssl;
    char		spool[PATH_MAX];
    char		user[NAME_MAX];
    char		group[NAME_MAX];
    long		ka_time;
    long		ka_intvl;

    xmpp_ctx_t		*ctx;
    xmpp_conn_t		*conn;
    int			connected;
    long		loop;

    int			ifd;
    int			wd;
};

long		g_daemon;
long		g_verbose;
long		g_syslog;

struct _xmpp	xmpp;

struct _conf_params	params[] =
{
    /*  name                type    variable             default value   */
    { "server",             's', { &xmpp.server },       "" },
    { "port",               'u', { &xmpp.port },         "0" },
    { "jid",                's', { &xmpp.jid },          "" },
    { "password",           's', { &xmpp.pass },         "" },
    { "persist",            'b', { &xmpp.persist },      "yes" },
    { "disable-tls",        'b', { &xmpp.disable_tls },  "no" },
    { "require-tls",        'b', { &xmpp.require_tls },  "no" },
    { "legacy-ssl",         'b', { &xmpp.legacy_ssl },   "no" },
    { "spool",              's', { &xmpp.spool },        XMPPCD_SPOOL },
    { "user",               's', { &xmpp.user },         "nobody" },
    { "group",              's', { &xmpp.group },        "nogroup" },
    { "keepalive-timeout",  'l', { &xmpp.ka_time },      "90" },
    { "keepalive-interval", 'l', { &xmpp.ka_intvl },     "90" },

    { "verbose",            'b', { &g_verbose },         "yes" },
    { "daemon",             'b', { &g_daemon },          "no" },
    { "syslog",             'b', { &g_syslog },          "no" },

    { NULL, 0, { NULL }, NULL }
};

/* dump config */
void dump_config( struct _conf_params * params )
{
    struct _conf_params	*p;
    char		v_txt[256];

    if( !g_verbose ) return;

    _logf( LOG_DEBUG, "----- config -----\n" );
    for( p = params; p->name; p++ )
    {
	_logf( LOG_DEBUG, "%s: %s\n", p->name, conf_val2text( p, v_txt, sizeof(v_txt) ) );
    }
    _logf( LOG_DEBUG, "--- config end ---\n" );
    return;
}

void file_send( struct _xmpp *x, char *name )
{
    xmpp_stanza_t	*msg, *body, *text;
    char		oldpath[PATH_MAX],newpath[PATH_MAX],id[NAME_MAX];
    int			fd, ret;
    char		buf[4096], *s, *to = "";
    struct timeval	now;

    _logf( LOG_NOTICE, "found file '%s'\n", name );

    gettimeofday( &now, NULL );
    snprintf( id, sizeof(id), "%ld.%06ld", now.tv_sec, now.tv_usec );

    snprintf( oldpath, sizeof(oldpath), "%s/out/%s", x->spool, name );
    snprintf( newpath, sizeof(newpath), "%s/sent/%s", x->spool, id );

    fd = open( oldpath, O_RDONLY );
    if( fd < 0 )
    {
	_logf( LOG_ERR, "Can't open file '%s': %s\n", oldpath, strerror(errno) );
	return;
    }
    ret = read( fd, buf, sizeof(buf) - 1 );
    close( fd );

    buf[ret] = 0;
    s = buf;

    if( !strncmp( s, "To: ", 4 ) )
    {
	s += 4;
	to = s;
	while( *s && !isspace(*s) ) s++;
	if( *s )
	{
	    *s++ = 0;
	    while( *s && isspace(*s) ) s++;
	}
    }

    if( !to[0] )
    {
    	_logf( LOG_NOTICE, "No recipients in '%s', can't send\n", name );
    	goto out_rename;
    }

    if( !s[0] )
    {
    	_logf( LOG_NOTICE, "File '%s' is empty, nothing to send\n", name );
    	goto out_rename;
    }

    _logf( LOG_NOTICE, "Sending file '%s'\n", name );

    msg = xmpp_stanza_new( x->ctx );
    xmpp_stanza_set_name( msg, "message" );
    xmpp_stanza_set_type( msg, "chat" );
    xmpp_stanza_set_to( msg, to );
    xmpp_stanza_set_id( msg, id );

    body = xmpp_stanza_new( x->ctx );
    xmpp_stanza_set_name( body, "body" );

    text = xmpp_stanza_new( x->ctx );

    xmpp_stanza_set_text( text, s );

    xmpp_stanza_add_child( body, text );
    xmpp_stanza_release( text );

    xmpp_stanza_add_child( msg, body );
    xmpp_stanza_release( body );

    xmpp_send( x->conn, msg );
    xmpp_stanza_release( msg );

out_rename:
    _logf( LOG_NOTICE, "Moving file '%s' to '%s'\n", name, newpath );
    ret = rename( oldpath, newpath );
    if( ret < 0 )
    {
	_logf( LOG_ERR, "Can't move file '%s' to '%s': %s\n", oldpath, newpath, strerror(errno) );
    }
}

int spool_scan( struct _xmpp *x )
{
    DIR			*dir;
    struct dirent	*de;
    char		spool_out[PATH_MAX];

    snprintf( spool_out, sizeof(spool_out), "%s/out", x->spool );
    dir = opendir( spool_out );
    if( !dir )
    {
	_logf( LOG_ERR, "Can't open directory '%s': %s\n", spool_out, strerror( errno ) );
	return -1;
    }

    while( ( de = readdir( dir ) ) != NULL )
    {
	if( de->d_type == DT_REG )
	{
	    file_send( x, de->d_name );
	}
    }

    closedir( dir );
    return 0;
}

int spool_check( struct _xmpp *x )
{
    char	ibuf[4096];
    int		ret;
    size_t	sz = sizeof(struct inotify_event) + NAME_MAX + 1;

    while( ( ret = read( x->ifd, &ibuf, sz ) ) > 0 )
    {
	if( ret >= sizeof(struct inotify_event) )
	{
	    struct inotify_event	*ie = (struct inotify_event *)ibuf;

	    if( ( ie->wd == x->wd ) && ( ie->len > 0 ) )
	    {
		file_send( x, ie->name );
	    }
	}
	else
	{
	    _logf( LOG_ERR, "read from inotify fd returned weird %d bytes, ignoring\n", ret );
	}
    }
    if( ( ret < 0 ) && ( errno != EAGAIN ) )
    {
	_logf( LOG_ERR, "inotify read error %s\n", strerror( errno ) );
	return ret;
    }
    return 0;
}

int spool_set_iwatch( struct _xmpp *x )
{
    char	spool_out[PATH_MAX];

    x->ifd = inotify_init();
    if( x->ifd < 0 )
    {
	_logf( LOG_ERR, "inotify_init error: %s\n", strerror(errno) );
	return x->ifd;
    }

    fcntl( x->ifd, F_SETFL, O_NONBLOCK );

    snprintf( spool_out, sizeof(spool_out), "%s/out", x->spool );
    x->wd = inotify_add_watch( x->ifd, spool_out, IN_MOVED_TO );
    if( x->wd < 0 )
    {
	_logf( LOG_ERR, "inotify_add_watch on '%s' error: %s\n", spool_out, strerror(errno) );
	close( x->ifd );
	x->ifd = -1;
	return x->wd;
    }
    return 0;
}

int daemon_start(void)
{
  pid_t  cpid;

  cpid = fork();
  if(cpid < 0)
  {
    fprintf(stderr, "Can't fork to become a daemon!\n");
    return 0;
  }
  else if(cpid > 0) /* parent exits */
  {
    exit(0);
  }
  setsid();
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  return 1;
}

void xmpp_cleanup( struct _xmpp *x )
{
    _logf( LOG_INFO, "Freeing context.\n" );
    xmpp_ctx_free( x->ctx );
    _logf( LOG_INFO, "Shutdown libstrophe.\n" );
    xmpp_shutdown();
}

void sig_handler( int sig )
{
    switch( sig )
    {
	case SIGINT:
	    _logf( LOG_NOTICE, "Got signal INT, exiting.\n" );
	    break;
	case SIGTERM:
	    _logf( LOG_NOTICE, "Got signal TERM, exiting.\n" );
	    break;
	default:
	    return;
    }

    _logf( LOG_INFO, "Releasing connection.\n" );
    xmpp_conn_release( xmpp.conn );
    xmpp_cleanup( &xmpp );
    if( xmpp.ifd >= 0 ) close( xmpp.ifd );
    if( g_syslog ) closelog();

    exit( 0 );
}

int presence_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	struct _xmpp	*x = (struct _xmpp *)userdata;
	char 		*from, *type, *stype;
	xmpp_stanza_t	*presence;

	type = xmpp_stanza_get_type( stanza );

	if( type == NULL ) return 1;

	     if( !strcmp( type, "error" ) ) return 1;
	else if( !strcmp( type, "subscribe" ) ) stype = "subscribed";
	else if( !strcmp( type, "unsubscribe" ) ) stype = "unsubscribed";
	else return 1;

	from = xmpp_stanza_get_from( stanza );

	presence = xmpp_stanza_new( x->ctx );
	xmpp_stanza_set_name( presence, "presence" );
	xmpp_stanza_set_type( presence, stype );
	xmpp_stanza_set_to( presence, from );
	xmpp_send( conn, presence );
	xmpp_stanza_release( presence );

	return 1;
}

int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	struct _xmpp	*x = (struct _xmpp *)userdata;
	xmpp_stanza_t	*inbody;
	char 		*intext, *from, *type;
	FILE		*f;
	char		fname[PATH_MAX];
	struct tm	now_s;
	struct timeval	now;
	char		date[256];

	type = xmpp_stanza_get_type( stanza );

	if( ( type != NULL ) && !strcmp( type, "error" ) ) return 1;

	inbody = xmpp_stanza_get_child_by_name( stanza, "body" );

	if( !inbody ) return 1;

	gettimeofday( &now, NULL );
	localtime_r( &now.tv_sec, &now_s );
	strftime( date, sizeof(date), "%a, %d %b %Y %H:%M:%S %z", &now_s );

	from = xmpp_stanza_get_from( stanza );
	intext = xmpp_stanza_get_text( inbody );

	_logf( LOG_NOTICE, "Incoming message from %s: %s\n", from, intext);

	snprintf( fname, sizeof(fname), "%s/in/%ld.%06ld", x->spool, now.tv_sec, now.tv_usec );

	f = fopen( fname, "w" );
	if( f )
	{
	    fprintf( f, "From: %s\n", from );
	    fprintf( f, "Date: %s\n", date );
	    fprintf( f, "Type: %s\n", type );
	    fprintf( f, "Id: %s\n", xmpp_stanza_get_id( stanza ) );
	    fprintf( f, "\n" );
	    fprintf( f, "%s\n", intext );
	    fclose( f );
	}
	else
	{
	    _logf( LOG_ERR, "Can't create file '%s': %s\n", fname, strerror(errno) );
	}
	xmpp_free(x->ctx, intext);
	return 1;
}

/* define a handler for connection events */
void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
		  const int error, xmpp_stream_error_t * const stream_error,
		  void * const userdata)
{
    struct _xmpp	*x = (struct _xmpp *)userdata;

    if (status == XMPP_CONN_CONNECT) {
	xmpp_stanza_t* pres;
	_logf( LOG_NOTICE, "connected\n" );
	xmpp_handler_add( conn, message_handler, NULL, "message", NULL, x );
	xmpp_handler_add( conn, presence_handler, NULL, "presence", NULL, x );
	/* Send initial <presence/> so that we appear online to contacts */
	pres = xmpp_stanza_new( x->ctx );
	xmpp_stanza_set_name( pres, "presence" );
	xmpp_send( conn, pres );
	xmpp_stanza_release( pres );
	x->connected = 1;
	spool_scan( x );
    }
    else {
	_logf( LOG_NOTICE, "disconnected\n" );
	x->loop = 0;
	x->connected = 0;
    }
}

/* replacement logger */
static void xmpp_logger(void * const userdata,
			 const xmpp_log_level_t level,
			 const char * const area,
			 const char * const msg)
{
    int prio = LOG_DEBUG;
    xmpp_log_level_t filter_level = * (xmpp_log_level_t*)userdata;
    if( level >= filter_level )
    {
	switch( level )
	{
	    case XMPP_LEVEL_ERROR:	prio = LOG_ERR; break;
	    case XMPP_LEVEL_WARN:	prio = LOG_WARNING; break;
	    case XMPP_LEVEL_INFO:	prio = LOG_INFO; break;
	    case XMPP_LEVEL_DEBUG:	prio = LOG_DEBUG; break;
	}
	_logf( prio, "%s %s\n", area, msg );
    }
}


int main(int argc, char **argv)
{
    xmpp_log_level_t		ll = XMPP_LEVEL_DEBUG;
    xmpp_log_t			log = { &xmpp_logger, &ll };
    long			flags = 0;
    int				ret;
    char			*prog;
    struct passwd		*pw;
    struct group		*gr;

    struct option long_options[] =
    {
	{"server",   required_argument, 0, 's'},
	{"jid",      required_argument, 0, 'j'},
	{"password", required_argument, 0, 'p'},
	{"nodaemon", no_argument,       0, 'n'},
	{"verbose",  no_argument,       0, 'v'},
	{"version",  no_argument,       0, 'V'},
	{"help",     no_argument,       0, 'h'},
	{0, 0, 0, 0}
    };

    prog = strrchr(argv[0],'/');
    if(!prog) prog = argv[0];
    else      prog++;

    memset( &xmpp, 0, sizeof(xmpp) );
    xmpp.ifd = -1;
    xmpp.wd = -1;

    conf_parse( XMPPCD_CONFIG, params );

    while( ( ret = getopt_long( argc, argv, "s:j:p:nvVh", long_options, NULL ) ) != -1 )
    {
	switch(ret)
	{
	    case 's': strcpy( xmpp.server, optarg ); break;
	    case 'j': strcpy( xmpp.jid, optarg ); break;
	    case 'p': strcpy( xmpp.pass, optarg ); break;
	    case 'v': g_verbose = 1; break;
	    case 'n': g_daemon = 0; break;
	    case 'V': printf( PACKAGE " version " VERSION "\n" ); return 0;
	    case 'h':
	    default:
		printf( PACKAGE " version " VERSION "\n" );
		printf( "Usage: %s [options]\n", prog );
		printf( "Options:\n" );
		printf( "\t-s, --server xmpp.example.com Connect to server xmpp.example.com\n" );
		printf( "\t-j, --jid user@example.com    Use jid user@example.com\n" );
		printf( "\t-p, --password foobar         Use password foobar\n" );
		printf( "\t-n, --nodaemon                Run in foreground\n" );
		printf( "\t-v, --verbose                 Verbose messages\n" );
		printf( "\t-V, --version                 Print version and exit\n" );
		printf( "\t-h, --help                    Show this help\n" );
		return 1;
	}
    }

    /* drop privileges */
    errno = 0;
    gr = getgrnam( xmpp.group );
    if( !gr )
    {
	if( errno ) fprintf( stderr, "Can't get gid for '%s': %s\n", xmpp.group, strerror( errno ) );
	else fprintf( stderr, "Group '%s' not found\n", xmpp.group );
	return 1;
    }

    errno = 0;
    pw = getpwnam( xmpp.user );
    if( !pw )
    {
	if( errno ) fprintf( stderr, "Can't get uid for '%s': %s\n", xmpp.user, strerror( errno ) );
	else fprintf( stderr, "User '%s' not found\n", xmpp.user );
	return 1;
    }

    if( setgid( gr->gr_gid ) )
    {
	fprintf( stderr, "Can't set group '%s(%d)': %s\n", xmpp.group, gr->gr_gid, strerror( errno ) );
	return 1;
    }

    if( setuid( pw->pw_uid ) )
    {
	fprintf( stderr, "Can't set user '%s(%d)': %s\n", xmpp.user, pw->pw_uid,  strerror( errno ) );
	return 1;
    }

    /* handle signals */
    signal( SIGINT,  sig_handler );
    signal( SIGTERM, sig_handler );
    signal( SIGPIPE, SIG_IGN );
    signal( SIGCHLD, SIG_IGN );

    if( g_daemon )
    { /* daemonize */
	if( !daemon_start() ) return 0;
	if( !g_syslog )
	{ /* open syslog since we are daemon */
	    g_syslog = 1;
	}
    }

    if( g_syslog ) openlog( prog, 0, LOG_USER );

    dump_config( params );

    /* set inotify watch on spool directory */
    ret = spool_set_iwatch( &xmpp );
    if( ret < 0 ) goto out_close_log;

    /* init library */
    xmpp_initialize();

    /* create a context */
    xmpp.ctx = xmpp_ctx_new( NULL, &log );

reconnect:
    /* create a connection */
    xmpp.conn = xmpp_conn_new( xmpp.ctx );

    /* configure connection properties (optional) */
    if( xmpp.disable_tls ) flags |= XMPP_CONN_FLAG_DISABLE_TLS;
    if( xmpp.require_tls ) flags |= XMPP_CONN_FLAG_MANDATORY_TLS;
    if( xmpp.legacy_ssl )  flags |= XMPP_CONN_FLAG_LEGACY_SSL;
    xmpp_conn_set_flags( xmpp.conn, flags );

    /* setup authentication information */
    xmpp_conn_set_jid( xmpp.conn, xmpp.jid );
    xmpp_conn_set_pass( xmpp.conn, xmpp.pass );

    /* initiate connection */
    xmpp_connect_client( xmpp.conn, xmpp.server[0] ? xmpp.server : NULL, xmpp.port, conn_handler, &xmpp );

    /* set TCP keepalive */
    xmpp_conn_set_keepalive( xmpp.conn, xmpp.ka_time, xmpp.ka_intvl );

    /* enter the event loop - our connect handler will trigger an exit */

    xmpp.loop = 1;

    while( xmpp.loop )
    {
	if( xmpp.connected )
	{
	    ret = spool_check( &xmpp );
	    if( ret < 0 ) goto out_xmpp_cleanup;
	}
	xmpp_run_once( xmpp.ctx, 100 );
    }

    _logf( LOG_INFO, "Releasing connection.\n" );
    xmpp_conn_release( xmpp.conn );

    if( xmpp.persist )
    {
	_logf( LOG_INFO, "Connection lost, reconnecting in 5 seconds...\n" );
	sleep( 5 );
	goto reconnect;
    }

out_xmpp_cleanup:
    xmpp_cleanup( &xmpp );

    if( xmpp.ifd >= 0 ) close( xmpp.ifd );

out_close_log:
    if( g_syslog ) closelog();

    return 0;
}
