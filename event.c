#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "event.h"
#include "error.h"

#define EQ(str1, str2) (strcmp(str1, str2) == 0)
#define DATE_SIZE (4 + 2 + 2 + 2 + 2 + 2)

#define CHILD(pid)	(pid == 0)
#define PARENT(pid) 	(pid > 0)

#define F_OPEN		0x1
#define F_MODIFY	0x2

extern int inotify_id;
extern int is_enabled;

static void delete_event(q_wevent_t e)
{
        debug("> entry");
        free(e);
        debug("> quit");
}

static q_wevent_t event_dequeue(void)
{
        q_wevent_t e = q_watch_list_p->head;

        debug("> entry");
        if(e) {
                q_watch_list_p->head = e->next;
                if(q_watch_list_p->head == NULL)
                        q_watch_list_p->tail = NULL;
                e->next = NULL;
        }
        debug("> quit");

        return e;
}

static void event_enqueue(q_wevent_t e)
{
        e->next = NULL;

        debug("> entry");
        if(q_watch_list_p->tail) {
                q_watch_list_p->tail->next = e;
                q_watch_list_p->tail = e;
        } else {
                q_watch_list_p->head = q_watch_list_p->tail = e;
        }
        debug("> quit");
}


static int event_close(watch_t w)
{
	return 0;
}

static int event_delete(watch_t w)
{
	return 0;
}

static int event_modify(watch_t w)
{
	int i;
	int ret;
	int status;
	pid_t pid;
	watch_item_t item = w->watch_list;

	pid = fork();
	if(pid == 0) {
		log_info("Executing: %s %s", item->bin, item->cmd[0]);
		ret = execv(item->bin, item->args);	
		if(ret == -1) {
			log_err("execl error");
			exit(-1);
		} else {
			exit(0);
		}

	} else if(pid > 0) {
		log_info("parent waiting for child...");
		status = wait(&status);
		log_info("parent detected child is done...");
	} else {
		log_err("fork failed!");
		return -1;
	} 
	
	return 0;
}

static int copy_file(char *orig, char *back)
{
	int r_fd;
	int w_fd;
	int rbytes;
	int wbytes;
	char buffer[1024];

	r_fd = open(orig, O_RDONLY);
	w_fd = open(back, O_CREAT|O_WRONLY, 0660);

	while((rbytes = read(r_fd, buffer, sizeof(buffer))) > 0) {
		wbytes = write(w_fd, &buffer, rbytes);
		if(wbytes != rbytes) {
			log_err("Write error");
			return -1;
		}
	}

	close(r_fd);
	close(w_fd);

	return 0;
}

static unsigned int get_mask(watch_item_t item)
{
	return item->mask;
}

static int event_open(watch_t w)
{
	struct tm tm_s;
	time_t t;
	char *cp;
	char *str;
	int len;
	int ret;
	unsigned int orig_mask;
	unsigned int new_mask;

	log_info("Doing backup");
	t = time(NULL);
	tm_s = *localtime(&t);

	len = 	strlen(w->backup_dir) + strlen(w->watch_list->bname) + DATE_SIZE + 2 + 6 + 5;
	str = (char *)allocate_object(len);

	snprintf(str, len, "%s/%s_%d_%s%d_%s%d-%s%d-%s%d-%s%d", w->backup_dir, w->watch_list->bname, 
							tm_s.tm_year + 1900, tm_s.tm_mon > 9 ? "" : "0", tm_s.tm_mon + 1,
							tm_s.tm_mday > 9 ? "" : "0", tm_s.tm_mday, 
   							tm_s.tm_hour > 9 ? "" : "0", tm_s.tm_hour,
							tm_s.tm_min > 9 ? "" : "0", tm_s.tm_min,
							tm_s.tm_sec > 9 ? "" : "0", tm_s.tm_sec);

	orig_mask = get_mask(w->watch_list);
	new_mask = IN_MOVED_TO;

	w->watch_list->watch_id = inotify_add_watch(inotify_id, w->watch_list->parent, new_mask);
	check(w->watch_list->watch_id != -1, "Error setting new mask");

	ret = copy_file(w->watch_list->name, str);

	w->watch_list->watch_id =  inotify_add_watch(inotify_id, w->watch_list->parent, IN_ALL_EVENTS);	
	check(w->watch_list->watch_id != -1, "Error setting old mask");

	log_info("Copied file (%s)", str);

	deallocate_object(str);

	return 0;
}

static void handle_event(q_wevent_t e, watch_t w)
{
        struct inotify_event *ievent = &e->ievent;
	event_t event;
        int is_dir;
	int ret;
        int wd;

        debug("> entry");

        if(ievent->mask & IN_ISDIR)
                is_dir = 1;
        else
                is_dir = 0;

//        log_info("Event on %s", ievent->len > 0 ? ievent->name : "Unknown");

        switch(ievent->mask & (IN_ALL_EVENTS | IN_ONESHOT | IN_IGNORED | IN_Q_OVERFLOW | IN_UNMOUNT)) {

                case IN_ACCESS:
//                        log_info("IN_ACCESS on (%d)", ievent->wd);
                        break;

                case IN_MODIFY:
                        log_info("IN_MODIFY on (%d)", ievent->wd);
			event = O_MODIFY;
			ret = events_op[event].event_triggered(w);

                        break;

                case IN_CLOSE_WRITE:
//                        log_info("IN_CLOSE_WRITE on (%d)", ievent->wd);
                        break;

                case IN_CLOSE_NOWRITE:
//                        log_info("IN_CLOSE_NOWRITE on (%d)", ievent->wd);
                        break;

                case IN_OPEN:
                        log_info("IN_OPEN on (%d)", ievent->wd);
			event = O_OPEN;
			ret = events_op[event].event_triggered(w);	

                        break;

                case IN_DELETE:
                        log_info("IN_DELETE on (%d)", ievent->wd);
                        break;

                case IN_CREATE:
                        log_info("IN_CREATE on (%d)", ievent->wd);
                        break;

                case IN_Q_OVERFLOW:
                        log_info("IN_Q_OVERFLOW (%d)", ievent->wd);
                        break;

                case IN_CLOSE:
                        log_info("IN_CLOSE (%d)", ievent->wd);
                        break;

                case IN_MOVE:
                        log_info("IN_MOVE (%d)", ievent->wd);
                        break;

                case IN_ONESHOT:
                        log_info("IN_ONESHOT (%d)", ievent->wd);
                        break;

                case IN_ATTRIB:
                        log_info("IN_ATTRIB (%d)", ievent->wd);
                        break;

                case IN_MOVED_FROM:
                        log_info("IN_MOVE_FROM (%d)", ievent->wd);
                        break;

                case IN_MOVED_TO:
                       log_info("IN_MOVE_TO (%d)", ievent->wd);
                        break;

                case IN_DELETE_SELF:
                        log_info("IN_DELETE_SELF (%d)", ievent->wd);
                        break;

                case IN_UNMOUNT:
                        log_info("IN_UNMOUNT (%d)", ievent->wd);
                        break;

                case IN_IGNORED:
                        log_info("IN_IGNORED (%d)", ievent->wd);
                        break;

                default:
                        log_info("Other type of event detected on (%d) - MASK (%x)", ievent->wd, ievent->mask);
                        break;
        }
        debug("> quit");
}

void handle_events(watch_t w)
{
        q_wevent_t event;

        debug("> entry");
        while( (event = event_dequeue()) != NULL) {
                debug("event_dequeued");
                handle_event(event, w);
                debug("event_handled");
                delete_event(event);
                debug("event_deleted");
        }
        debug("> quit");
}


static int is_our_file(watch_t w, char *filename)
{
	watch_item_t item = w->watch_list;
	int found = 0;
	
	while(item != NULL) {
		if(EQ(item->bname, filename)) {
			found = 1;
			break;
		}
		item = item->next;
	}

	return found;
}


int get_events(watch_t w, int inotify_id) /* http://www.thegeekstuff.com/2010/04/inotify-c-program-example/ */
{
        q_wevent_t wevent;
        char buffer[BUF_LEN_E] __attribute__ ((aligned(8)));
        size_t event_size, q_event_size;
	useconds_t usec;
        int events_read;
        int nbytes;
        int rbytes;
	int flag;

        debug("> entry");

	usec = 1000 * 1;
	usleep(usec);
	
        rbytes = nbytes = events_read = flag = 0;
        memset(buffer, '\0', sizeof(buffer));
        rbytes = read(inotify_id, buffer, BUF_LEN_E);

	debug("> rbytes %d", rbytes);
	debug("> sizeof(buffer) %d", sizeof(buffer));

        if(rbytes < 0) {
                log_err("Could not read from fd (%d)", inotify_id);
                return -1;
        } else if(rbytes == 0) {
                log_info("No bytes in fd (%d)", inotify_id);
                return rbytes;
        } else {
                while(nbytes  < rbytes) {
                        struct inotify_event *event = (struct inotify_event *)&buffer[nbytes];
                        event_size = sizeof(struct inotify_event) + event->len;
                        nbytes += event_size;

			if(!is_enabled) {
				log_info("nodesync is disabled. Discarting event...");
				continue;
			}
				

			if(event->mask & IN_OPEN) {
				debug("> mask IN_OPEN");
				if((flag & F_OPEN) || (flag & F_MODIFY))
					continue;
				else
					flag |= F_OPEN;
			}

			if(event->mask & IN_MODIFY) {
				debug("> mask IN_OPEN");
				if(flag & F_MODIFY)
					continue;
				else
					flag |= F_MODIFY;
			} 
			
			if(w->type == ISFILE && (event->mask & IN_ISDIR) == IN_ISDIR) 
				continue;

			if(w->type == ISFILE && event->len > 0) {
				if(!is_our_file(w, event->name)) 
					continue;
			}

                        q_event_size = sizeof(struct q_watch_event) + event->len;
                        wevent = malloc(q_event_size);
                        memmove(&(wevent->ievent), event, event_size);
                        event_enqueue(wevent);
                        events_read++;
                }
        }

        debug("> events_read (%d)", events_read);
        debug("> quit");
        return events_read;
}

int events_available(int inotify_id)
{
        fd_set rdfs;

        debug("> entry");
        FD_ZERO(&rdfs);
        FD_SET(inotify_id, &rdfs);
        debug("> quit");

        return select(FD_SETSIZE, &rdfs, NULL, NULL, NULL);
}

void create_queue(void)
{
        q_watch_list_p = malloc(sizeof(struct q_watch_list));
        q_watch_list_p->head = q_watch_list_p->tail = NULL;
}



