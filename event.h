#ifndef __event_h__
#define __event_h__

#include <sys/inotify.h>
#include <limits.h>
#include "nodesync.h"

#define INOTIFY_MASK    (IN_OPEN | IN_CREATE |  \
                        IN_MODIFY | IN_DELETE | \
                        IN_ACCESS | IN_MODIFY | \
                        IN_CLOSE_WRITE | IN_CLOSE_NOWRITE | IN_Q_OVERFLOW )


#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN_E (EVENT_SIZE + NAME_MAX + 1)*10


struct q_watch_event {
	struct q_watch_event *next;
	struct inotify_event ievent;
};
typedef struct q_watch_event * q_wevent_t;


struct q_watch_list {
	q_wevent_t head;
	q_wevent_t tail;
};
typedef struct q_watch_list * q_watch_list_t;

static q_watch_list_t q_watch_list_p;


typedef enum events{
	O_OPEN,
	O_MODIFY,
	O_DELETE,
	O_CLOSE,
	O_NONE
}event_t;


static int event_open(watch_t w);
static int event_modify(watch_t w);
static int event_delete(watch_t w);
static int event_close(watch_t w);

struct event_op_table {
	const char *name;
	int (*event_triggered)(watch_t watch);
} events_op[] = {
	{"open", event_open},
	{"modify", event_modify},
	{"delete", event_delete},
	{"close", event_close}
};

#endif
