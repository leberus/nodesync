#ifndef CONFIG_H
#       define CONFIG_H 1

#include "nodesync.h"
#include "error.h"

typedef enum {
        GET_GLOBAL_KEY,
        GET_LOCAL_KEY,
        GET_PAIR,
}cfg_state;

typedef enum {
        W_PATH,
        RNODES,
        EXCLUDES,
        BACKUP_DIR,
        RSYNC,
        R_ARGS,
        LOG_FILE,
        DELAY,
        GLOB_CFG,
        NONE
}cfg_option;

typedef FILE* nodesync_f;
nodesync_f logfile;



static rnode_t cfg_new_rnode(void);
static cfg_t cfg_new_item(void);
static cfg_t cfg_create_item(void);
static void cfg_add_item(cfg_t *cfg_l, cfg_t x);

static int get_size(int fd);

static int cfg_set_wpath(cfg_t cfg_i, char *value);
static int cfg_set_remote_node(cfg_t cfg_i, char *value);
static int cfg_set_exclude_dir(cfg_t cfg_i, char *value);
static int cfg_set_backup_directory(cfg_t cfg_i, char *value);
static int cfg_set_rsync_path(cfg_t cfg_i, char *value);
static int cfg_set_rsync_args(cfg_t cfg_i, char *value);

static nodesync_f cfg_set_logfile(char *value);

struct directives_t {
        const char *name;
        int (*add_value)(cfg_t i, char *value);
        const char *desc;
} directives[] = {
        {"wpath", cfg_set_wpath, "Path to watch"},
        {"rnodes", cfg_set_remote_node, "Remote nodes where the sync is done"},
        {"excludes", cfg_set_exclude_dir, "Which directories are excluded"},
        {"local_backup_directory", cfg_set_backup_directory, "Local directory where the files will be backup"},
        {"rsync", cfg_set_rsync_path, "Path to rsync binary"},
        {"args", cfg_set_rsync_args, "Arguments for rysnc"}
};

static const char *glob_directives[] = 	{	
						"edelay",
						"logfile",
						"watch_config",
						NULL
					};

static const char *locl_directives[] = 	{	
						"wpath",
						"rnodes",
						"excludes",
						"local_backup_directory",
						"rsync",
						"args",
						NULL
					};
#endif
