#ifndef CONFIG_H
#       define CONFIG_H 1

#include "nodesync.h"

typedef enum {
        GET_GLOBAL_KEY,
        GET_LOCAL_KEY,
        GET_PAIR,
}cfg_state;

typedef enum {
        W_PATH,
        RNODES,
        EXCLUDES,
        LOG_FILE,
        BACKUP_DIR,
        R_ARGS,
        DELAY,
        GLOB_CFG,
        NONE
}cfg_option;


static struct rnode_t *new_rnode(void);
static struct watch_instance *new_instance(void);
static struct watch_instance *init_new_instance(void);
static void add_instance(struct watch_instance **w, struct watch_instance *x);

static int get_size(int fd);

static int set_wpath(struct watch_instance *w, char *value);
static int set_remote_node(struct watch_instance *w, char *value);
static int set_exclude_dir(struct watch_instance *w, char *value);
static int set_logfile(struct watch_instance *w, char *value);
static int set_backup_directory(struct watch_instance *w, char *value);
static int set_r_args(struct watch_instance *w, char *value);

struct directives_t {
        const char *name;
        int (*add_value)(struct watch_instance *w, char *value);
        const char *desc;
} directives[] = {
        {"wpath", set_wpath, "Path to watch"},
        {"rnodes", set_remote_node, "Remote nodes where the sync is done"},
        {"excludes", set_exclude_dir, "Which directories are excluded"},
        {"logfile", set_logfile, "File where application will log"},
        {"local_backup_directory", set_backup_directory, "Local directory where the files will be backup"},
        {"r_args", set_r_args, "Arguments for rysnc"}
};

#endif
