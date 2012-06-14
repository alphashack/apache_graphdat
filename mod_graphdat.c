#include <stdbool.h>

#include "httpd.h"
#include "http_config.h"

#include "lib/module_graphdat/graphdat.h"

#ifndef MOD_GRAPHDAT_DEFAULT_SOCKET_FILE
#define MOD_GRAPHDAT_DEFAULT_SOCKET_FILE "/tmp/gd.agent.sock"
#endif

#ifndef MOD_GRAPHDAT_SOURCE
#define MOD_GRAPHDAT_SOURCE "apache"
#endif

static bool s_enabled = false;

module AP_MODULE_DECLARE_DATA graphdat_module;

typedef struct {
	bool enable;
	char * socket_file;
} mod_graphdat_config;

static const char * set_mod_graphdat_enable(cmd_parms *parms, void *config, int flag)
{
	mod_graphdat_config * cfg = ap_get_module_config(parms->server->module_config, &graphdat_module);
 
	cfg->enable = flag;

	return NULL;
}

static const char * set_mod_graphdat_socket(cmd_parms *parms, void *config, const char *arg)
{
	mod_graphdat_config * cfg = ap_get_module_config(parms->server->module_config, &graphdat_module);
 
	cfg->socket_file = (char *)arg;

	return NULL;
}

void delegate_logger(void * user, const char * fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);

	vfprintf(stderr, fmt, argp);
	fprintf(stderr, "\n");
	fflush(stderr);

	va_end(argp);
}

static apr_status_t mod_graphdat_child_exit(void *data)
{
	if(s_enabled)
	{
        graphdat_term(delegate_logger, NULL);
	}

    return APR_SUCCESS;
}

static void mod_graphdat_child_init(apr_pool_t *p, server_rec *s)
{
	mod_graphdat_config * cfg = ap_get_module_config(s->module_config, &graphdat_module);

	s_enabled = cfg->enable;

	if(s_enabled)
	{
        graphdat_init(cfg->socket_file, strlen(cfg->socket_file), MOD_GRAPHDAT_SOURCE, strlen(MOD_GRAPHDAT_SOURCE), delegate_logger, NULL);

	    apr_pool_cleanup_register(p, s, mod_graphdat_child_exit, mod_graphdat_child_exit);
	 }
}

static int mod_graphdat_log_transaction_handler(request_rec *r)
{
	if(s_enabled)
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		// resolution of request time is usec
		int now_usec = tv.tv_sec * 1000000 + tv.tv_usec;
		int start_usec = r->request_time;
		int diff_usec = now_usec - start_usec;
		double diff_msec = (double)diff_usec / 1000;

		graphdat_store((char *)r->method, strlen(r->method), r->uri, strlen(r->uri), diff_msec, delegate_logger, NULL, 0);

		return OK;
	 }

	return DECLINED;
}

static void * mod_graphdat_create_config(apr_pool_t *p, server_rec *s)
{
	mod_graphdat_config * cfg;
 
	// allocate memory for the config struct from the provided pool p.
	cfg = (mod_graphdat_config *)apr_pcalloc(p, sizeof(mod_graphdat_config));
 
	// set the default values
	cfg->enable = false;
	cfg->socket_file = MOD_GRAPHDAT_DEFAULT_SOCKET_FILE;

	// return the new server configuration structure.
	return (void *)cfg;
}

static const command_rec mod_graphdat_cmds[] =
{
    AP_INIT_FLAG(
        "Graphdat",
        set_mod_graphdat_enable,
        NULL,
        RSRC_CONF, 
        "Enable graphdat"
    ),
	AP_INIT_TAKE1(
		"GraphdatSocketFile",
		set_mod_graphdat_socket,
		NULL,
		RSRC_CONF,
		"Graphdat socket file descriptor (optional)"
	),
	{NULL}
};

static void mod_graphdat_register_hooks (apr_pool_t *p)
{
    ap_hook_child_init(mod_graphdat_child_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_log_transaction(mod_graphdat_log_transaction_handler, NULL, NULL, APR_HOOK_LAST);
}

module AP_MODULE_DECLARE_DATA graphdat_module =
{
	STANDARD20_MODULE_STUFF,
	NULL,	/* per-directory config creator */
	NULL,	/* dir config merger */
	mod_graphdat_create_config,	/* server config creator */
	NULL,	/* server config merger */
	mod_graphdat_cmds,	/* command table */
	mod_graphdat_register_hooks	/* set up other request processing hooks */
};

