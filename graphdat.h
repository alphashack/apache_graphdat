#ifndef GRAPHDAT_H
#define GRAPHDAT_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*logger_delegate_t)(void * user, const char * fmt, ...);

typedef struct {
	char* method;
	int methodlen;
	char* uri;
	int urilen;
	double msec;
	logger_delegate_t logger;
	void * log_context;
} request_t;

void graphdat_init(char * file, int filelen, char* source, int sourcelen, logger_delegate_t logger, void * log_context);
void graphdat_term(logger_delegate_t logger, void * log_context);
void graphdat_store(char* method, int methodlen, char* uri, int urilen, double msec, logger_delegate_t logger, void * log_context, int log_context_len);

#endif /* GRAPHDAT_H */

