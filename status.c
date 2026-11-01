/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Native metric: read battery capacity directly from sysfs (zero fork) */
void
status_battery(char *out, size_t maxlen)
{
	static const char *paths[] = {
		"/sys/class/power_supply/BATT/capacity",
		"/sys/class/power_supply/BAT0/capacity",
		"/sys/class/power_supply/BAT1/capacity",
	};
	FILE *f = NULL;
	for (size_t i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
		if ((f = fopen(paths[i], "r")))
			break;
	}
	if (!f) {
		out[0] = '\0';
		return;
	}
	int cap = 0;
	if (fscanf(f, "%d", &cap) == 1)
		snprintf(out, maxlen, "%d%%", cap);
	else
		out[0] = '\0';
	fclose(f);
}

/* Native metric: format current time (zero fork) */
void
status_datetime(char *out, size_t maxlen)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	if (tm)
		strftime(out, maxlen, "%I:%M %p", tm);
	else
		out[0] = '\0';
}

/* Initialize block runtime fields */
static void
status_init(void)
{
	for (unsigned int i = 0; i < LENGTH(status_blocks); i++) {
		status_blocks[i].pid = -1;
		status_blocks[i].fd = -1;
		status_blocks[i].last_run = 0;
		status_blocks[i].output[0] = '\0';
	}
}

/* Single-pass UTF-8 boundary validator: rewinds at most 3 bytes to prevent slicing multi-byte glyphs */
static size_t
sanitize_utf8(char *buf, size_t len)
{
	if (len == 0)
		return 0;

	if ((unsigned char)buf[len - 1] < 0x80)
		return len;

	size_t i = len;
	while (i > 0 && (unsigned char)buf[i - 1] >= 0x80 && (unsigned char)buf[i - 1] < 0xC0)
		i--;

	if (i == 0) {
		buf[0] = '\0';
		return 0;
	}

	unsigned char lead = (unsigned char)buf[i - 1];
	size_t needed = 0;
	if (lead >= 0xC0 && lead <= 0xDF)
		needed = 2;
	else if (lead >= 0xE0 && lead <= 0xEF)
		needed = 3;
	else if (lead >= 0xF0 && lead <= 0xF7)
		needed = 4;
	else {
		buf[i - 1] = '\0';
		return i - 1;
	}

	size_t available = len - (i - 1);
	if (available < needed) {
		buf[i - 1] = '\0';
		return i - 1;
	}

	return len;
}

/* Status2D escape span sanitizer: drops trailing unclosed caret escape to prevent tag hijacking */
static size_t
sanitize_status2d(char *buf, size_t len)
{
	size_t last_open = (size_t)-1;
	int in_tag = 0;

	for (size_t i = 0; i < len; i++) {
		if (buf[i] == '^') {
			if (!in_tag) {
				in_tag = 1;
				last_open = i;
			} else {
				in_tag = 0;
			}
		}
	}

	if (in_tag && last_open != (size_t)-1) {
		buf[last_open] = '\0';
		len = last_open;
	}

	return len;
}

/* Format all block outputs into stext and redraw bar if changed */
static void
status_format(void)
{
	if (LENGTH(status_blocks) == 0)
		return;

	char buf[sizeof(stext)];
	size_t pos = 0;
	int has_prev = 0;

	buf[0] = '\0';
	for (unsigned int i = 0; i < LENGTH(status_blocks); i++) {
		Block *b = &status_blocks[i];
		if (!b->output[0])
			continue;

		if (has_prev) {
			const char *sep = " ";
			size_t slen = strlen(sep);
			if (pos + slen < sizeof(buf) - 1) {
				memcpy(buf + pos, sep, slen);
				pos += slen;
				buf[pos] = '\0';
			}
		}

		if (strchr(b->output, '^')) {
			size_t olen = strlen(b->output);
			if (pos + olen < sizeof(buf) - 1) {
				memcpy(buf + pos, b->output, olen);
				pos += olen;
				buf[pos] = '\0';
			}
			/* Enforce markup isolation with closing ^d^ if needed */
			if (pos >= 3 && strcmp(buf + pos - 3, "^d^") != 0 && pos + 3 < sizeof(buf) - 1) {
				memcpy(buf + pos, "^d^", 3);
				pos += 3;
				buf[pos] = '\0';
			}
		} else {
			char block_buf[160];
			char *colon = strstr(b->output, ": ");
			if (colon) {
				*colon = '\0';
				char *val = colon + 2;
				const char *ic = b->icon ? b->icon : "";
				if (ic[0] || b->output[0]) {
					snprintf(block_buf, sizeof(block_buf),
						"^c%s^^b%s^%s%s^c%s^^b%s^%s^d^",
						cap_fg, cap_bg, ic, b->output,
						val_fg, val_bg, val);
				} else {
					snprintf(block_buf, sizeof(block_buf),
						"^c%s^^b%s^%s^d^",
						val_fg, val_bg, val);
				}
				*colon = ':';
			} else {
				if (b->icon && b->icon[0]) {
					snprintf(block_buf, sizeof(block_buf),
						"^c%s^^b%s^%s^c%s^^b%s^%s^d^",
						cap_fg, cap_bg, b->icon,
						val_fg, val_bg, b->output);
				} else {
					snprintf(block_buf, sizeof(block_buf),
						"^c%s^^b%s^%s^d^",
						val_fg, val_bg, b->output);
				}
			}
			size_t blen = strlen(block_buf);
			if (pos + blen < sizeof(buf) - 1) {
				memcpy(buf + pos, block_buf, blen);
				pos += blen;
				buf[pos] = '\0';
			}
		}
		has_prev = 1;
	}

	pos = sanitize_utf8(buf, pos);
	pos = sanitize_status2d(buf, pos);

	if (strcmp(stext, buf) != 0) {
		strncpy(stext, buf, sizeof(stext) - 1);
		stext[sizeof(stext) - 1] = '\0';
		statusw = status2d_textw(stext);
		drawbar(selmon);
	}
}

/* Spawn or execute a single block */
static int
status_spawn(Block *b, time_t now)
{
	/* 1. Native C function execution (zero-fork, synchronous microsecond) */
	if (b->func) {
		char buf[sizeof(b->output)] = {0};
		b->func(buf, sizeof(buf));
		size_t len = sanitize_utf8(buf, strlen(buf));
		len = sanitize_status2d(buf, len);
		b->last_run = now;
		if (strcmp(b->output, buf) != 0) {
			strncpy(b->output, buf, sizeof(b->output) - 1);
			b->output[sizeof(b->output) - 1] = '\0';
			return 1;
		}
		return 0;
	}

	/* 2. Shell command execution (asynchronous non-blocking pipe) */
	if (!b->cmd || !b->cmd[0])
		return 0;

	/* Safety guard: prevent duplicate child if already running */
	if (b->fd >= 0 || b->pid > 0) {
		unsigned int grace = b->interval > 5 ? b->interval * 2 : 10;
		if (b->last_run && (now - b->last_run > grace)) {
			/* Command stalled: terminate and recover */
			if (b->pid > 0) {
				kill(-b->pid, SIGKILL);
				waitpid(b->pid, NULL, WNOHANG);
				b->pid = -1;
			}
			if (b->fd >= 0) {
				close(b->fd);
				b->fd = -1;
			}
		} else {
			/* Still executing within grace period; do not double spawn */
			return 0;
		}
	}

	int pfd[2];
	if (pipe(pfd) < 0)
		return 0;

	fcntl(pfd[0], F_SETFL, O_NONBLOCK);
	fcntl(pfd[0], F_SETFD, FD_CLOEXEC);
	fcntl(pfd[1], F_SETFD, FD_CLOEXEC);

	pid_t pid = fork();
	if (pid < 0) {
		close(pfd[0]);
		close(pfd[1]);
		return 0;
	}

	if (pid == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		close(pfd[0]);
		dup2(pfd[1], STDOUT_FILENO);
		close(pfd[1]);

		int nullfd = open("/dev/null", O_RDWR);
		if (nullfd >= 0) {
			dup2(nullfd, STDIN_FILENO);
			dup2(nullfd, STDERR_FILENO);
			close(nullfd);
		}

		setsid();
		signal(SIGCHLD, SIG_DFL);
		execl("/bin/sh", "sh", "-c", b->cmd, (char *)NULL);
		_exit(127);
	}

	close(pfd[1]);
	b->fd = pfd[0];
	b->pid = pid;
	b->last_run = now;
	return 0;
}

/* Read available output from a block pipe */
static int
status_read(Block *b, int revents)
{
	if (b->fd < 0)
		return 0;

	char buf[sizeof(b->output)];
	size_t total = 0;
	ssize_t n;
	int changed = 0;

	if (revents & POLLIN) {
		while (total < sizeof(buf) - 1 &&
		       (n = read(b->fd, buf + total, sizeof(buf) - 1 - total)) > 0) {
			total += n;
		}
	}

	int has_nl = 0;
	if (total > 0) {
		buf[total] = '\0';
		/* Cut at first newline or carriage return */
		char *nl = strpbrk(buf, "\r\n");
		if (nl) {
			*nl = '\0';
			has_nl = 1;
		}

		/* Strip trailing whitespace */
		size_t len = strlen(buf);
		while (len > 0 && (buf[len - 1] == ' ' || buf[len - 1] == '\t'))
			buf[--len] = '\0';

		len = sanitize_utf8(buf, len);
		len = sanitize_status2d(buf, len);

		if (strcmp(b->output, buf) != 0) {
			strncpy(b->output, buf, sizeof(b->output) - 1);
			b->output[sizeof(b->output) - 1] = '\0';
			changed = 1;
		}
	}

	/* Close pipe and reap child if line is complete, on EOF, HUP, or error */
	if (has_nl || (revents & (POLLHUP | POLLERR)) || (total == 0 && (revents & POLLIN))) {
		close(b->fd);
		b->fd = -1;
		if (b->pid > 0) {
			pid_t r = waitpid(b->pid, NULL, WNOHANG);
			if (r > 0 || (r < 0 && errno == ECHILD))
				b->pid = -1;
		}
	} else if (b->pid > 0) {
		pid_t r = waitpid(b->pid, NULL, WNOHANG);
		if (r > 0 || (r < 0 && errno == ECHILD)) {
			close(b->fd);
			b->fd = -1;
			b->pid = -1;
		}
	}

	return changed;
}

/* Check intervals, spawn expired blocks, and return timeout in ms */
static int
status_step(time_t now)
{
	int changed = 0;
	int min_remaining = -1;

	for (unsigned int i = 0; i < LENGTH(status_blocks); i++) {
		Block *b = &status_blocks[i];

		if (b->interval == 0) {
			if (b->last_run == 0) {
				if (status_spawn(b, now))
					changed = 1;
			}
			continue;
		}

		time_t elapsed = now - b->last_run;
		if (b->last_run == 0 || elapsed >= b->interval) {
			if (status_spawn(b, now))
				changed = 1;
			elapsed = 0;
		}

		int remaining = (int)(b->interval - elapsed);
		if (remaining < 0)
			remaining = 0;
		if (min_remaining == -1 || remaining < min_remaining)
			min_remaining = remaining;
	}

	if (changed)
		status_format();

	if (min_remaining == -1)
		return -1;
	return min_remaining <= 0 ? 1000 : min_remaining * 1000;
}

/* Clean up on dwm exit */
static void
status_cleanup(void)
{
	for (unsigned int i = 0; i < LENGTH(status_blocks); i++) {
		Block *b = &status_blocks[i];
		if (b->fd >= 0) {
			close(b->fd);
			b->fd = -1;
		}
		if (b->pid > 0) {
			kill(-b->pid, SIGTERM);
			waitpid(b->pid, NULL, WNOHANG);
			b->pid = -1;
		}
	}
}
