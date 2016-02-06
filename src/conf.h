#ifndef __CONF_H__
#define __CONF_H__

struct _conf_params {
    char	*name;
    char	type;
    union {
	void		*v_void;
	int		*v_bool;
	char		*v_str;
	long		*v_long;
	unsigned long	*v_ulong;
    };
    char	*d_val;
};

void conf_parse( char * conf_name, struct _conf_params * params );
char * conf_val2text( struct _conf_params * p, char * buf, int len );

#endif
