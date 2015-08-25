#include "allocate.h"

void *allocate_object(size_t size)
{
	return (void *)malloc(size);
}

void deallocate_object(void *item)
{
	free(item);
}
