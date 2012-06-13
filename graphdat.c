#include "graphdat.h"
#include "list.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <msgpack.h>
#include <pthread.h>
#include <netinet/in.h>

#ifndef GRAPHDAT_WORKER_LOOP_SLEEP
// GRAPHDAT_WORKER_LOOP_SLEEP usec
#define GRAPHDAT_WORKER_LOOP_SLEEP 100000
#endif

static bool s_init = false;
static bool s_running = true;
static char * s_sockfile = NULL;
static char * s_source = NULL;
static int s_sourcelen;
static int s_sockfd = -1;
static bool s_lastwaserror = false;
static list_t s_requests;

static pthread_mutex_t s_mux = PTHREAD_MUTEX_INITIALIZER;
static pthread_t s_thread;

void graphdat_send(char* method, int methodlen, char* uri, int urilen, double msec, logger_delegate_t logger, void * log_context);

void socket_close() {
	close(s_sockfd);
	s_sockfd = -1;
}

void del_request(request_t * req) {
	free(req->log_context);
	free(req->uri);
	free(req->method);
	free(req);	
}

void dlg_del_request(void * item) {
	request_t * req = (request_t *)item;
	del_request(req);
}

void socket_term(logger_delegate_t logger, void * log_context) {
        if(s_init) {
            s_running = false;
            pthread_join(s_thread, NULL);
	    socket_close();
	    free(s_sockfile);
	    free(s_source);
            listDel(s_requests, dlg_del_request);
        }
}

bool socket_connect(logger_delegate_t logger, void * log_context) {
	if(s_sockfd < 0)
	{
		s_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (s_sockfd < 0)
		{
			if(!s_lastwaserror) {
				logger(log_context, "graphdat error: could not create socket (%s)", strerror(s_sockfd));
				s_lastwaserror = true;
			}
			return false;
		}
		fcntl(s_sockfd, F_SETFL, O_NONBLOCK);

		struct sockaddr_un serv_addr;
		int servlen;
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sun_family = AF_UNIX;
		strcpy(serv_addr.sun_path, s_sockfile);
		servlen = sizeof(serv_addr.sun_family) + strlen(serv_addr.sun_path) + 1;
		int result = connect(s_sockfd, (struct sockaddr *)&serv_addr, servlen);
		if(result < 0)
		{
			if(!s_lastwaserror) {
				logger(log_context, "graphdat error: could not connect socket (%s)", strerror(result));
				s_lastwaserror = true;
			}
			socket_close();
			return false;
		}
	}
	return true;
}

bool socket_check(logger_delegate_t logger, void * log_context) {
	if(!s_init) {
		if(!s_lastwaserror) {
			logger(log_context, "graphdat error: not initialised");
			s_lastwaserror = true;
		}
		return false;
	}
	return socket_connect(logger, log_context);
}

void* worker(void* arg)
{
	request_t *req;

	while(s_running) {
		pthread_mutex_lock(&s_mux);
		req = listRemoveFront(s_requests);
		pthread_mutex_unlock(&s_mux);

		if(req != NULL) {
			graphdat_send(req->method, req->methodlen, req->uri, req->urilen, req->msec, req->logger, req->log_context);
			del_request(req);
		}
		else
		{
			usleep(GRAPHDAT_WORKER_LOOP_SLEEP);
		}
	}
	return NULL;
}

void socket_init(char * file, int filelen, char* source, int sourcelen, logger_delegate_t logger, void * log_context) {
	s_sockfile = malloc(filelen + 1);
	memcpy(s_sockfile, file, filelen);
	s_sockfile[filelen] = 0;
	s_sockfd = -1;
	s_source = malloc(sourcelen + 1);
	s_sourcelen = sourcelen;
	memcpy(s_source, source, sourcelen);
	s_source[sourcelen] = 0;
	s_requests = listNew();
	pthread_create(&s_thread, NULL, worker, NULL);
	s_init = true;
}

void socket_send(char * data, int len, logger_delegate_t logger, void * log_context) {
	if(!socket_check(logger, log_context)) return;

	int nlen = htonl(len);

	int wrote = write(s_sockfd, &nlen, sizeof(nlen));
	if(wrote < 0)
	{
		logger(log_context, "graphdat error: could not write socket (%s)", strerror(wrote));
		socket_close();
	}
        else
	{
		wrote = write(s_sockfd, data, len);
		if(wrote < 0)
		{
			logger(log_context, "graphdat error: could not write socket (%s)", strerror(wrote));
			socket_close();
		}
		else
		{
			//logger(log_context, "socket_send (%d bytes)", wrote);
			s_lastwaserror = false;
		}
        }
}

void graphdat_init(char * file, int filelen, char* source, int sourcelen, logger_delegate_t logger, void * log_context) {
	socket_init(file, filelen, source, sourcelen, logger, log_context);
}

void graphdat_term(logger_delegate_t logger, void * log_context) {
	socket_term(logger, log_context);
}

void graphdat_send(char* method, int methodlen, char* uri, int urilen, double msec, logger_delegate_t logger, void * log_context) {
	msgpack_sbuffer* buffer = msgpack_sbuffer_new();
        msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

	msgpack_pack_map(pk, 4); // timestamp, type, route, responsetime, source
	// timestamp
	msgpack_pack_raw(pk, 9);
        msgpack_pack_raw_body(pk, "timestamp", 9);
	msgpack_pack_int(pk, 1);
	// type
	msgpack_pack_raw(pk, 4);
        msgpack_pack_raw_body(pk, "type", 4);
	msgpack_pack_raw(pk, 6);
        msgpack_pack_raw_body(pk, "Sample", 6);
	// route
	msgpack_pack_raw(pk, 5);
        msgpack_pack_raw_body(pk, "route", 5);
	msgpack_pack_raw(pk, urilen);
        msgpack_pack_raw_body(pk, uri, urilen);
	// responsetime
	msgpack_pack_raw(pk, 12);
        msgpack_pack_raw_body(pk, "responsetime", 12);
	msgpack_pack_double(pk, msec);
	// source
	msgpack_pack_raw(pk, 6);
        msgpack_pack_raw_body(pk, "source", 6);
	msgpack_pack_raw(pk, 5);
        msgpack_pack_raw_body(pk, s_source, s_sourcelen);
        
	socket_send(buffer->data, buffer->size, logger, log_context);

	msgpack_sbuffer_free(buffer);
        msgpack_packer_free(pk);
}

void graphdat_store(char* method, int methodlen, char* uri, int urilen, double msec, logger_delegate_t logger, void * log_context, int log_context_len) {
	request_t *req = malloc(sizeof(request_t));
	// method
	req->method = malloc(methodlen);
	memcpy(req->method, method, methodlen);
	req->methodlen = methodlen;
	// uri
	req->uri = malloc(urilen);
	memcpy(req->uri, uri, urilen);
	req->urilen = urilen;
	// msec
	req->msec = msec;
	// log
	req->logger = logger;
	if(log_context != NULL)
	{
		req->log_context = malloc(log_context_len);
		memcpy(req->log_context, log_context, log_context_len);
	}
	else
	{
		req->log_context = NULL;
	}

	pthread_mutex_lock(&s_mux);
	listAppendBack(s_requests, req);
	pthread_mutex_unlock(&s_mux);
}

