#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include "nodesync.h"

const char *cfg_file_ = "nodesync.conf";

int check_cfg(struct watch_instance *w_cfg)
{
	struct stat st;
	int ret;
	int i;

	ret = -1;

	/* Check for: */

	/* Wpath */
	printf("Checking wpath > %s\n", w_cfg->wpath);
	if(stat(w_cfg->wpath, &st) == -1) {
		perror("Stat could not be performed");
		return ret;
	}
	
	if(	!S_ISREG(st.st_mode) &&
		!S_ISDIR(st.st_mode) &&
		!S_ISLNK(st.st_mode)	) {
		fprintf(stderr, "wpath is not a directory neither a regular file neither a link\n");
		return ret;
	}

	/* Log file */
	printf("Checking logfile > %s\n", w_cfg->logfile);
	if(stat(w_cfg->logfile, &st) == 0 && !S_ISREG(st.st_mode)) {
		fprintf(stderr, "If you want to provide a logfile by yourself, this has to be a regular file\n");
		return ret;
	}

	printf("Checking backup_dir > %s\n", w_cfg->backup_dir);
	/* Backup dir */
	if(stat(w_cfg->backup_dir, &st) == 0 && !S_ISDIR(st.st_mode)) {
		fprintf(stderr, "%s does exist but must be a directory, please provide another path for that\n");
		return ret;
	}

	/* Excluded files */
	for(i = 0; i < w_cfg->n_excludes ;i++) {
		if(stat(w_cfg->excludes[i], &st) == -1) {
			fprintf(stderr, "%s does not exist\n", w_cfg->excludes[i]);
			return ret;	
		}
	}

	printf("Configuration checked - OK!\n");
	ret = 0;
	return ret;
}

int main(void)
{
	struct watch_instance *w_cfg;
	int inotify_fd;
	int fd;
	int ret;

	fd = open(cfg_file_, O_RDONLY);
	if(fd == -1) {
		perror("Error opening config file");
		return -1;
	}
	
	w_cfg = load_cfg(fd);
	if(w_cfg == NULL) {
		clean_cfg(&w_cfg);
		close(fd);
		return -1;
	}

	walk_through(w_cfg);
	close(fd);
	
	ret = check_cfg(w_cfg);
	if(ret) {
		clean_cfg(&w_cfg);
		fprintf(stderr, "Error checking the configuration\n");
		return -1;
	}

	inotify_fd = inotify_init();
	
	return 0;
}
