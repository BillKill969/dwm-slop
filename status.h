/* See LICENSE file for copyright and license details. */
#ifndef DWM_STATUS_H
#define DWM_STATUS_H

#include <time.h>
#include <sys/types.h>

typedef struct {
	const char *icon;
	const char *cmd;
	void (*func)(char *out, size_t maxlen);
	unsigned int interval; /* in seconds; 0 = run once at startup */
	/* internal runtime state */
	pid_t pid;
	int fd;
	time_t last_run;
	char output[64];
} Block;

/* Native metric helpers (optional zero-fork in-process metrics) */
void status_battery(char *out, size_t maxlen);
void status_datetime(char *out, size_t maxlen);

#endif /* DWM_STATUS_H */
