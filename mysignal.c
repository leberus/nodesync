#include <signal.h>
#include "error.h"

int is_enabled = 1;


void sig_handler(int sig)
{
	is_enabled = is_enabled ? 0 : 1;
	log_info("sig_handler: SIGUSR1 detected");
	log_info("nodesync is %s", is_enabled ? "enabled" : "disabled");
}

int add_sig_handlers(void)
{
	int ret;

	if(signal(SIGUSR1, sig_handler) == SIG_ERR)
		ret = -1;
	else
		ret = 0;
	
	return ret;
}
