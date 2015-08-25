#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "error.h"
#include "config.h"

#define DEFAULT_DELAY	180
#define DEFAULT_LOGFILE "nodesync.log"

#define EQ(str1, str2) (strcmp(str1, str2) == 0)

int delay = DEFAULT_DELAY;

static rnode_t cfg_new_rnode(void)
{
        return (rnode_t)malloc(sizeof(struct rnode));
}


static cfg_t cfg_new_item(void)
{
        return (cfg_t)malloc(sizeof(struct config_item));
}


static cfg_t cfg_create_item(void)
{
        cfg_t p;


        p = cfg_new_item();
	check(p , "Could not create the cfg_t structure")
	if(error_isset)
		return NULL;

        p->n_excludes = 0;
	p->n_rnodes = 0;
        p->wpath = NULL;
        p->backup_dir = NULL;
        p->rsync_args = NULL;
        p->excludes = NULL;
        p->rnode = NULL;

        return p;
}


static void cfg_add_item(cfg_t *cfg_l, cfg_t x)
{
        if(*cfg_l == NULL) {
                x->next = NULL;
                *cfg_l = x;
        } else {
                x->next = *cfg_l;
                *cfg_l = x;
        }
}


static int cfg_set_wpath(cfg_t cfg_i, char *value)
{
	debug("> adding (%s)", value);
	cfg_i->wpath = strdup(value);
	debug("> added");

	return 0;
}


static int cfg_set_rsync_path(cfg_t cfg_i, char *value)
{
	debug("> adding (%s)", value);
	cfg_i->rsync_path = strdup(value);
	debug("> added");

	return 0;
}

static int cfg_set_rsync_args(cfg_t cfg_i, char *value)
{
	debug("> adding (%s)", value);
	cfg_i->rsync_args = strdup(value);
	debug("> added");

	return 0;
}


static int cfg_set_remote_node(cfg_t x, char *value)
{

	rnode_t rnode;
	char *node;
	char *dir;

	debug("> adding (%s)", value);

	rnode = cfg_new_rnode();
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

	x->n_rnodes++;

	debug("> node (%s)", x->rnode->node);
	debug("> dir (%s)", x->rnode->dir);
	debug("> added");

	return 0;
}


static int cfg_set_exclude_dir(cfg_t cfg_i, char *value)
{
 	debug("> adding (%s)", value);

	if(cfg_i->n_excludes == 0) {
		cfg_i->excludes = malloc(sizeof(char **));
	} else {
		cfg_i->excludes = realloc(cfg_i->excludes, cfg_i->n_excludes + 1);
	}

	cfg_i->excludes[cfg_i->n_excludes] = strdup(value);
	cfg_i->n_excludes++;	

 	debug("> added");

	return 0;
}


static int cfg_set_backup_directory(cfg_t cfg_i, char *value)
{
	debug("> adding (%s)", value);
	cfg_i->backup_dir = strdup(value);
	debug("> added");

	return 0;
}


static nodesync_f cfg_set_logfile(char *name)
{
        return fopen(name, "wa");
}


static int get_size(int fd)
{
        struct stat st;

	return (fstat(fd, &st) == 0) ? st.st_size : -1;
}


static int directive_is_global(const char *directive)
{
	int i;
	int found;

	for(found = 0, i = 0; glob_directives[i] != NULL && found != 1 ; i++)
		if(EQ(glob_directives[i], directive))
			found = 1;

	return found;
}


static int directive_is_local(const char *directive)
{
	int i;
	int found;
	
	for(found = 0, i = 0; locl_directives[i] != NULL && found != 1 ; i++)
		if(EQ(locl_directives[i], directive))
			found = 1;

	return found;
}


static cfg_option check_directive(const char *directive)
{
        if(EQ(directive, "watch_config"))
                return GLOB_CFG;
        else if(EQ(directive, "wpath"))
                return W_PATH;
        else if(EQ(directive, "excludes"))
                return EXCLUDES;
        else if(EQ(directive, "rnodes"))
                return RNODES;
        else if(EQ(directive, "logfile"))
                return LOG_FILE;
        else if(EQ(directive, "local_backup_directory"))
                return BACKUP_DIR;
        else if(EQ(directive, "edelay"))
                return DELAY;
        else if(EQ(directive, "rsync"))
		return RSYNC;
        else if(EQ(directive, "args"))
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


#ifdef DEBUG
void walk_through(cfg_t cfg_i)
{
	cfg_t p;
	rnode_t x;
	int i;

	printf("\n\n-------------------\n");
	printf("Dump w structure: \n\n");

	for(p = cfg_i; p != NULL ;p = p->next) {
		printf("wpath > %s\n", p->wpath);
		printf("backup_dir > %s\n", p->backup_dir);
		printf("r_args > %s\n", p->rsync_args);
		
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
#endif

void clean_cfg(cfg_t *w)
{
	cfg_t x;

	while(*w != NULL) {
		x = *w;
		*w = x->next;
		free(x);
	}
}
			

cfg_t load_cfg(int fd)
{
	int ret;
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
	cfg_t cfg_i, x;

	cfg_i = x = NULL;
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

	logfile = cfg_set_logfile(DEFAULT_LOGFILE);
	check(logfile, "Could not open the file (%s)", DEFAULT_LOGFILE);

	if(error_isset)
		goto fatal_error;
	
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
				
				if(cfg_directive == NONE) { 	/* Bad directive */
					log_err("\"%s\" not recognized as a directive", directive);
					goto fatal_error;
				} 

				if(state == GET_GLOBAL_KEY) {	/* Is really global? */
					ret = directive_is_global(directive);
					check(ret, "\"%s\" out of scope", directive);


					if(error_isset)
						goto fatal_error;

					if(cfg_directive == GLOB_CFG) {					
						x = cfg_create_item();
						cfg_add_item(&cfg_i, x);
					}

				} else {			/* Is really local? */
					ret = directive_is_local(directive);
					check(ret, "\"%s\" out of scope", directive);
					if(error_isset)
						goto fatal_error;	
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
		
				if(cfg_directive != DELAY && cfg_directive != LOG_FILE) 
					directives[cfg_directive].add_value(x, option);
				
				p--;
				continue;
			}	
		}
	}

        free(buffer);
        return cfg_i;

	fatal_error:
		free(buffer);
		clean_cfg(&cfg_i);	
		return NULL;
}

