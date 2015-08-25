#ifndef __nodesync_h__
#define __nodesync_h__

struct watch_item {
	char *parent;
	char *bname;
	char *name;
	char *bin;
	char **cmd;
	int type;
	unsigned int mask;
	int flags;
	int watch_id;
	struct watch_item *next;
};
typedef struct watch_item * watch_item_t;

/* char *logfile; */

struct watch {
/*      char *logfile;	*/
	int type;
        char *backup_dir;
        char **excludes;
	watch_item_t watch_list;
        struct watch *next;
};
typedef struct watch * watch_t;


struct rnode {
        char *node;
        char *dir;
        struct rnode *next;
};
typedef struct rnode* rnode_t;


struct config_item {
	char *wpath;
        char *backup_dir;
        char *rsync_path;
        char *rsync_args;
        char **excludes;
        int n_excludes;
	int n_rnodes;
        int recursive;
        unsigned int depth;
        rnode_t rnode;
        struct config_item *next;
};
typedef struct config_item* cfg_t;


cfg_t load_cfg(int fd);
void walk_through(cfg_t cfg_i);

#define ISDIR	0
#define ISFILE	1


#endif
