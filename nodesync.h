#ifndef NODESYNC_H
#       define NODESYNC_H 

struct rnode_t {
        char *node;
        char *dir;
        struct rnode_t *next;
};

struct watch_instance {
        int n_nodes;
        int n_excludes;
        char *wpath;
        char *logfile;
        char *backup_dir;
        char *r_args;
        char **excludes;
        struct rnode_t *rnode;
        struct watch_instance *next;
};

struct watch_instance *load_cfg(int fd);
void walk_through(struct watch_instance *w);

#endif
