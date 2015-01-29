#ifndef NOTIFY_H
	#define NOTIFY_H

#include <sys/inotify.h>


#define W_FLAGS	(	IN_CREATE|IN_OPEN| \
			IN_MODIFY|IN_CLOSE_WRITE| \
			IN_DELETE|IN_DELETE_SELF )

struct __q_events {
        int event_handled;
        struct inotify_event i_event;
};

typedef struct __q_events q_events_t;


#endif
