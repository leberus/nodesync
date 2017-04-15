#ifndef __error_h__
#define __error_h__


#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: [%s] " M "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif

int error_isset;

#define get_errno() (errno == 0 ? "" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: %s) " M "\n", __FILE__, __LINE__, get_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d: %s) " M "\n", __FILE__, __LINE__, get_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; error_isset=1; }

#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; error_isset=1; }

#endif
