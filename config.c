#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "config.h"

#define DEFAULT_DELAY	180

const char *cfg_file = "nodesync.conf";
int delay = DEFAULT_DELAY;


static struct rnode_t *new_rnode(void)
{
        return (struct rnode_t *)malloc(sizeof(struct rnode_t));
}


static struct watch_instance *new_instance(void)
{
        return (struct watch_instance *)malloc(sizeof(struct watch_instance));
}


static struct watch_instance *init_new_instance(void)
{
        struct watch_instance *p;

        p = new_instance();

        p->n_nodes = 0;
        p->n_excludes = 0;
        p->wpath = NULL;
        p->logfile = NULL;
        p->backup_dir = NULL;
        p->r_args = NULL;
        p->excludes = NULL;
        p->rnode = NULL;
        return p;
}


static void add_instance(struct watch_instance **w, struct watch_instance *x)
{
        if(*w == NULL) {
                x->next = NULL;
                *w = x;
        } else {
                x->next = *w;
                *w = x;
        }
}


static int set_wpath(struct watch_instance *w, char *value)
{
	printf("[set_wpath] > adding \"%s\"...\n", value);
	w->wpath = strdup(value);
	printf("[set_wpath] > added\n");

	return 0;
}


static int set_r_args(struct watch_instance *w, char *value)
{
	printf("[set_r_args] > adding \"%s\"...\n", value);
	w->r_args = strdup(value);
	printf("[set_r_args] > added\n");

	return 0;
}


static int set_remote_node(struct watch_instance *x, char *value)
{

	struct rnode_t *rnode;
	char *node;
	char *dir;

	printf("Set remote\n");
	printf("[set_remote_node] > adding \"%s\"...\n", value);

	rnode = new_rnode();
	node = strtok(value, ":");
	dir = strtok(NULL, ":");
	rnode->node = strdup(node);
	rnode->dir = strdup(dir);

	if(x->rnode == NULL) {
		rnode->next = NULL;
		x->rnode = rnode;
	} else {
		rnode->next = x->rnode;
		x->rnode = rnode;
	}

	printf("[set_remote_node] > node (%s)\n", x->rnode->node);
        printf("[set_remote_node] > dir (%s)\n", x->rnode->dir);
	printf("[set_remote_node] > added\n");

	return 0;
}


static int set_exclude_dir(struct watch_instance *w, char *value)
{
	printf("[set_exclude_dir] > adding \"%s\"...\n", value);

	if(w->n_excludes == 0) {
		w->excludes = malloc(sizeof(char **));
	} else {
		w->excludes = realloc(w->excludes, w->n_excludes + 1);
	}

	w->excludes[w->n_excludes] = strdup(value);
	w->n_excludes++;	
	printf("[set_exclude_dir] > added\n", value);

	return 0;
}


static int set_logfile(struct watch_instance *w, char *value)
{
	printf("[set_logfile] > adding \"%s\"...\n", value);
	w->logfile = strdup(value);
	printf("[set_logfile] > added\n");

	return 0;
}


static int set_backup_directory(struct watch_instance *w, char *value)
{
	printf("[set_backup_directory] > adding \"%s\"...\n", value);
	w->backup_dir = strdup(value);
	printf("[set_backup_directory] > added\n");

	return 0;
}


static int get_size(int fd)
{
        struct stat st;

        if(fstat(fd, &st) == 0)
                return st.st_size;
}


static cfg_option check_directive(const char *directive)
{
        if(strcmp(directive, "watch_config") == 0)
                return GLOB_CFG;
        else if(strcmp(directive, "wpath") == 0)
                return W_PATH;
        else if(strcmp(directive, "excludes") == 0)
                return EXCLUDES;
        else if(strcmp(directive, "rnodes") == 0)
                return RNODES;
        else if(strcmp(directive, "logfile") == 0)
                return LOG_FILE;
        else if(strcmp(directive, "local_backup_directory") == 0)
                return BACKUP_DIR;
        else if(strcmp(directive, "edelay") == 0)
                return DELAY;
        else if(strcmp(directive, "args") == 0)
                return R_ARGS;
        else
                return NONE;
}


static int is_valid_pair(char c, int quoted)
{
        if(c == ' ' && quoted)
                return 1;
        else
                return (c != ',' && c != '\n' && c != '}' && c != '"' && c != ' ' && c != '\t');
}


static int is_valid_key(char c)
{
        return (c != ' ' && c != '\t' && c != '\n');
}


void walk_through(struct watch_instance *w)
{
	struct watch_instance *p;
	struct rnode_t *x;
	int i;

	printf("\n\n =========== \n");
	printf("Dump w structure: \n\n");

	for(p = w; p != NULL ;p = p->next) {
		printf("wpath > %s\n", p->wpath);
		printf("logfile > %s\n", p->logfile);
		printf("backup_dir > %s\n", p->backup_dir);
		printf("r_args > %s\n", p->r_args);
		
		for(i = 0; i < p->n_excludes; i++)
			printf("excludes (%d) > %s\n", i, p->excludes[i]);
			
		for(x = p->rnode ; x != NULL ; x = x->next) {
			printf("node > %s\n", x->node);
			printf("dir > %s\n", x->dir);
		}

		i = 0;

		printf("\n-------------------");
		printf("\n\n");
	}

}


void clean_cfg(struct watch_instance **w)
{
	struct watch_instance *x;

	while(*w != NULL) {
		x = *w;
		*w = x->next;
		free(x);
	}
}
			

struct watch_instance *load_cfg(int fd)
{
	int len;
	int size;
        int bytes;
	int block;
	int token;
	int quotes;
	int nl;
	char directive[80];
	char option[256];
	char *buffer;
	char *p;
	char *ap;
	cfg_state state;
	cfg_option cfg_directive;
	struct watch_instance *w, *x;

	w = x = NULL;
	nl = 1;
	block = quotes = token = 0;
	state = GET_GLOBAL_KEY;
	size = get_size(fd);
	buffer = calloc(size + 1, sizeof(char));

	bytes = read(fd, buffer, size);
	if(bytes != size) {
		fprintf(stderr, "Could not read the whole file\n");
		goto fatal_error;
	}
	
	p = buffer;

	while(*p != '\0') {
		p += strspn(p, " \t"); 
		
		if(*p == '\n') {
			p++;
			nl++;
			if(state == GET_PAIR) {
				if(block)
					state = GET_LOCAL_KEY;
				else
					state = GET_GLOBAL_KEY;

				token = 0;
			}
			continue;
		}

		if(*p == '#') {
			while(*p++ != '\n')
				;
			p--;
			continue;
		}

		if(*p == '"') {
			quotes = quotes ? 0 : 1;
			p++;
			continue;
		}

		if(*p == '=') {
			if(token && !block) {
				fprintf(stderr, "Error in line %d: double '=' found\n", nl);
				goto fatal_error;
			}
	
			state = GET_PAIR;

			token = 1;
			p++;
			continue;
		}

		if(*p == '{') {
			if(block) {
				fprintf(stderr, "Error in line %d: found '{', but expected '}'\n", nl);
				goto fatal_error;
			} else {
				block = 1;
				state = GET_LOCAL_KEY;
				p++;
				continue;
			}
		}

		if(*p == '}') {
			if(block) {
				block = 0;
				token = 0;
				state = GET_GLOBAL_KEY;
				p++;
				continue;
			} else {
				fprintf(stderr, "Error in line %d: found '}', but '{' was not found before\n", nl);
				goto fatal_error;
			}
		}

		if(state == GET_GLOBAL_KEY || state == GET_LOCAL_KEY) {
			if(is_valid_key(*p)) {
                                ap = p;

                                while(is_valid_key(*p++))
                                        ;

                                len = (p - ap) -1;
                                strncpy(directive, ap, len);
                                directive[len] = '\0';
				cfg_directive = check_directive(directive);

				if(state == GET_GLOBAL_KEY && cfg_directive != DELAY) {
					if(cfg_directive != GLOB_CFG) {
						fprintf(stderr, "Error in line %d: directive \"%s\" found out of \"watch_config\" block\n", nl, directive);
						goto fatal_error;
					}					

					x = init_new_instance();
					add_instance(&w, x);
					
				} else {
                                	if(cfg_directive == NONE || cfg_directive == GLOB_CFG) {
                                        	fprintf(stderr, "Error in line %d: directive \"%s\" not recognized\n", nl, directive);
	                                        goto fatal_error;
        	                        }
				}

                                p--;
                                continue;
                        }
		} else {
			if(is_valid_pair(*p, quotes)) {
				ap = p;

				while(is_valid_pair(*p++, quotes))
					;	

				len = (p - ap) -1;
				strncpy(option, ap, len);
				option[len] = '\0';
		
				if(cfg_directive != DELAY) 
					directives[cfg_directive].add_value(x, option);
					
				p--;
				continue;
			}	
		}
	}

	
	walk_through(w);
        free(buffer);
        return w;

	fatal_error:
		free(buffer);
		clean_cfg(&w);	
		return NULL;
}

