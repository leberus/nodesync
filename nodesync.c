#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "nodesync.h"
#include "error.h"
#include <sys/inotify.h>


#define RET_F	-1
#define RET_OK	0

#define MISSING_ARGUMENT -2
#define INCORRENT_ARGUMENT -3

#define D_PERM (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
#define F_PERM (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

#define ADD_WATCHNODE(list, node)	do { 					\
						if(list == NULL) {		\
							node->next = NULL;	\
							list = node;		\
						} else {			\
							node->next = NULL;	\
							list = node;		\
						}				\
					} while(0)				


static const char *prog_name;
int inotify_id;
static int logfile_isready = 0;

extern FILE *logfile;


void missing_argument(void)
{
	fprintf(stderr, "%s: missing argument\n", prog_name);
	exit(MISSING_ARGUMENT);
}


void incorrect_argument(char c)
{
	fprintf(stderr, "%s: '%c' is an unknown argument\n", prog_name, c);
	exit(INCORRENT_ARGUMENT);
}


void print_help(void)
{
	printf("%s - usage:\n", prog_name);
	printf("Arguments:\n");
	printf("\t-f [config_file]\n");
	printf("\t-h (Help)\n");
	exit(EXIT_SUCCESS);
}

FILE *open_file(char *file)
{
	return fopen(file, "wa");
}

int create_dir(char *dir)
{
	return mkdir(dir, D_PERM);
}

int create_file(char *file)
{
	return creat(file, F_PERM);
}

int check_cfg(struct config_item *w_cfg)
{
	struct stat st;
	int ret;

	/* Check for: */

	 /* Log file */
        /*printf("Checking logfile > %s\n", w_cfg->logfile);
        ret = access(w_cfg->logfile, F_OK); */
	/*if(ret) {*/ /* Cannot access to a file: either not exist either we don't have right to check */
/*		if(errno == EACCES) {
			fprintf(stderr, "Cannot check for (%s), please check permissions\n", w_cfg->logfile);
			return RET_F;
		}

		ret = create_file(w_cfg->logfile);
		if(ret == -1) {
			fprintf(stderr, "Could not create the file (%s): %s\n", w_cfg->logfile, strerror(errno));
			return RET_F;
		}

		w_cfg->flog = open_file(w_cfg->logfile);
		if(w_cfg->flog == NULL) {
			fprintf(stderr, "Could not open the file (%s): %s\n", w_cfg->logfile, strerror(errno));
			return RET_F;
		}

	} else { *//* Can access to a file, check if is a regular file and if we have read/write perms for it*/
/*		ret = stat(w_cfg->logfile, &st);
		if(ret) {
			fprintf(stderr, "Could not perform stat on %s\n", w_cfg->logfile);
			return RET_F;
		}

		if(!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
			fprintf(stderr, "(%s) is not a file nor a symlink\n", w_cfg->logfile);
			return RET_F;
		} else {
			ret = access(w_cfg->logfile, R_OK|W_OK);
			if(ret) {
				fprintf(stderr, "(%s) does not have read/write permissions, please check the file\n", w_cfg->logfile);
				return RET_F;
			}
		}
	}
	printf("logfile... OK\n"); */


	/* Wpath */
	printf("Checking wpath > %s\n", w_cfg->wpath);
	ret = stat(w_cfg->wpath, &st);	
	if( ret ) {

		if(errno == ENOENT)
			fprintf(stderr, "We do not have the correct permissions, please check the rights on (%s)\n", w_cfg->wpath);
		else if(errno == ENOENT)
			fprintf(stderr, "(%s) does not exist. (%s) must be a diretory/file/symlink\n", w_cfg->wpath, w_cfg->wpath);
		else
			fprintf(stderr, "Coult not perform a stat on (%s)", w_cfg->wpath);

		return RET_F;

	} else {
		if(	!S_ISREG(st.st_mode) &&
			!S_ISDIR(st.st_mode) &&
			!S_ISLNK(st.st_mode)	) {
			fprintf(stderr, "wpath is not a directory neither a regular file neither a link\n");
			return RET_F;
		}
	}
	printf("wpath... OK\n");


	/* Backup dir */
	printf("Checking backup_dir > %s\n", w_cfg->backup_dir);
	ret = stat(w_cfg->backup_dir, &st);
	if(ret) {
		if(errno == ENOENT) {
			ret = create_dir(w_cfg->backup_dir);
			if(ret == RET_F) {
				fprintf(stderr, "Could not create the directory (%s)\n", w_cfg->backup_dir);
			}
                } else if(errno == EACCES) {
                        fprintf(stderr, "(%s) does not exist. (%s) must be a diretory/file/symlink\n", w_cfg->backup_dir, w_cfg->backup_dir);
			return RET_F;
                } else {
                        fprintf(stderr, "Coult not perform a stat on (%s)", w_cfg->backup_dir);
			return RET_F;
		}
	
	} else {
		if(!S_ISDIR(st.st_mode)) {
			fprintf(stderr, "(%s) must be a directory, please provide another path\n", w_cfg->backup_dir);
			return RET_F;
		}

		ret = access(w_cfg->backup_dir, R_OK|W_OK);
		if(ret) {
			fprintf(stderr, "(%s) is not readable/writable\n", w_cfg->backup_dir);
			return RET_F;
		}
	}
	printf("local_backup_dir... OK\n");


	printf("Configuration checked - OK!\n");
	return RET_OK;
}


void start_the_watch(watch_t w, int inotify_id)
{
	int ret;
	
	debug("> entry");

	create_queue();

	while(1) {
		debug("> calling events_available");
		ret = events_available(inotify_id);
		if(ret > 0) {
			debug("> calling get_events");
			ret = get_events(w, inotify_id);
			if(ret > 0 ) {
				debug("> calling handle_events");
				handle_events(w);
			} else if(ret == 0) {
				continue;
			} else {
				break;
			}
		}
	}
	
	debug("> quit");
}


static char **add_rsync_cmd(cfg_t cfg)
{
	int i;
	int len;
	char **cmd;
	
	log_info("n_rnodes > %d", cfg->n_rnodes);

	cmd = allocate_object(sizeof(char *) * (cfg->n_rnodes + 1));
	if(cmd == NULL)
		return cmd;

	for(i = 0 ; i < cfg->n_rnodes ; i++) {

                len = strlen(cfg->wpath) + strlen(cfg->rsync_args) + strlen(cfg->rnode->node) + strlen(cfg->rnode->dir) + strlen(":") + 4;
                cmd[i] = allocate_object(len);
                snprintf(cmd[i], len, "%s %s %s:%s",            cfg->rsync_args,
								cfg->wpath,
                                                                cfg->rnode->node,
                                                                cfg->rnode->dir);
                log_info("item->cmd[%d] > %s", i, cmd[i]);
        }

	log_info("i-> %d", i);
	cmd[i] = NULL;
	return cmd;
}

watch_item_t set_watch_list(cfg_t cfg)
{
        watch_item_t item;
        struct dirent *ep;
        struct stat st;
	char *fname;
	char *dname;
        int watch_d;
        DIR *dp;
	
	debug("> entry");

        item = (watch_item_t)allocate_object(sizeof(struct watch_item)); 
	item->next = NULL;
        check(item, "Could not malloc");
        if(error_isset)
                goto error;

        item->name = strdup(cfg->wpath);

        if(stat(item->name, &st) == -1) {
                fprintf(stderr, "Could not get a stat call\n");
                goto error;
        }

	item->bin = strdup(cfg->rsync_path);
	item->cmd = add_rsync_cmd(cfg);
	check(item->cmd, "add_rsync_cmd failed");
	if(error_isset)
		goto error;

        if(S_ISREG(st.st_mode)) {

		item->type =  ISFILE;
		dname = dirname(item->name);
		fname = basename(item->name);		
		item->bname = fname;
		item->parent = dname;
		item->flags = 0;
			
                watch_d = inotify_add_watch(inotify_id, dname, IN_ALL_EVENTS);
                if(watch_d > 0) {
                        debug("(%s) > was added to inotify events with wd (%d)", item->name, watch_d);
                        item->watch_id = watch_d;
                }

        } else if(S_ISDIR(st.st_mode)) {/* */

		item->type = ISDIR;
                log_info("Skipping...");
		watch_d = inotify_add_watch(inotify_id, item->name, IN_ALL_EVENTS);
		if(watch_d > 0) {
			log_info("(%s) > was added to inotif events with wd (%d)", item->name, watch_d);
			item->watch_id = watch_d;
		}

		/*                
                         directorio = directorio_principal
                         mientras directorio tenga subdirectorios
                                . leer nuevo directorio
                                {
                                       crear structura watch_list
                                       poner el nombre
                                       poner el parent
                                       crear el comando
                                       crear watch_id
                                }
		*/
                
        } else {
                log_err("Unsuported type of file for (%s)", item->name);
                goto error;
        }

	debug("> quit");
        return item;

        error:
		debug("> quit");
                return NULL;
}



void init_watchnode(watch_t w, cfg_t cfg)
{
        int i;
	
	debug("> entry");

        w->backup_dir = strdup(cfg->backup_dir);

	debug("> quit");

}



watch_t create_watchlist(cfg_t cfg)
{
        watch_t list;
        cfg_t p_cfg;

	debug("> entry");

        for(p_cfg = cfg ; p_cfg != NULL ; p_cfg = p_cfg->next) {
                watch_t p = (watch_t)allocate_object(sizeof(struct watch));
                init_watchnode(p, p_cfg);
                p->watch_list = set_watch_list(p_cfg);
		p->type = p->watch_list->type;
                ADD_WATCHNODE(list, p);
        }
	
	debug("> quit");

        return list;
}


int main(int argc, char **argv)
{
	watch_t watch;
	cfg_t cfg;
	char *cfg_file;
	int fd;
	int ret;
	int opt;

	prog_name = argv[0];
	cfg_file = NULL;

	while((opt = getopt(argc, argv, "f:h")) != -1) {
		switch(opt) {
			case 'f':
				cfg_file = optarg;
				break;
			case 'h':
				print_help();
				break;
			case ':':
				missing_argument();
				break;
			case '?':
				incorrect_argument(opt);
				break;
		}
	}			

	check(cfg_file, "%s: The config file is missing", prog_name);
	if(error_isset)
		exit(EXIT_FAILURE);
			

	fd = open(cfg_file, O_RDONLY);
	if(fd == -1) {
		log_err("Error opening file (%s)", cfg_file);
		exit(EXIT_FAILURE);
	}
	
	cfg = load_cfg(fd);
	close(fd);

	if(cfg == NULL) {
		clean_cfg(&cfg);
		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	walk_through(cfg);
#endif
	
	ret = check_cfg(cfg);
	if(ret) {
		clean_cfg(&cfg);
		fprintf(stderr, "Error checking the configuration\n");
		return -1;
	}

	inotify_id = inotify_init();
	watch = create_watchlist(cfg);
	start_the_watch(watch, inotify_id);

	log_info("Watch ended");
	
	return 0;
}
