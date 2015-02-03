#ifndef NODESYNC_H
#       define NODESYNC_H 

struct rnode_t {
        char *node;
        char *dir;
        struct rnode_t *next;
};

struct watch_list {
	int w_id;
	char *path;
	struct watch_list *next;
};

struct watch_instance {
        char *wpath;
        char *logfile;
        char *backup_dir;
        char **excludes;
	char **cmds;
	struct watch_list *w_list;
        struct watch_instance *next;
};

struct config {
	char *wpath;
        char *logfile;
        char *backup_dir;
        char *rsync_path;
        char *rsync_args;
        char **excludes;
        char **cmds;
        int n_excludes;
        int recursive;
        unsigned int depth;
        struct rnode_t *rnode;
        struct watch_instance *next;
};


struct watch_instance *load_cfg(int fd);
void walk_through(struct watch_instance *w_instance);

#endif
