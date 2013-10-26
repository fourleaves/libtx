#include <time.h>
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#endif

#ifdef WIN32
#ifndef WSAID_TRANSMITFILE
#define WSAID_TRANSMITFILE \
{0xb5367df0,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#endif

#ifndef WSAID_ACCEPTEX
#define WSAID_ACCEPTEX \
{0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
typedef BOOL (PASCAL *LPFN_ACCEPTEX)(SOCKET, SOCKET, PVOID, DWORD,
        DWORD, DWORD, LPDWORD, LPOVERLAPPED);
#endif

#ifndef WSAID_CONNECTEX
#define WSAID_CONNECTEX \
{0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}
typedef BOOL (PASCAL *LPFN_CONNECTEX)(SOCKET, const struct sockaddr *,
        int, PVOID, DWORD, LPDWORD, LPOVERLAPPED);
#endif

static LPFN_ACCEPTEX lpAcceptEx = NULL;
static LPFN_CONNECTEX lpConnectEx = NULL;
#endif

#include "txall.h"

#ifdef WIN32
typedef struct tx_completion_port_t {
	HANDLE port_handle;
	tx_poll_t port_poll;
	tx_task_t port_task;
	tx_loop_t *port_loop;
} tx_completion_port_t;

#define ENTRIES_COUNT 10

static void tx_completion_port_polling(void *up)
{
	int i;
	int timeout;
	BOOL result;
	ULONG count;
	tx_loop_t *loop;
	tx_completion_port_t *port;

	DWORD transfered_bytes;
	ULONG_PTR completion_key;
	LPOVERLAPPED overlapped = {0};

	port = (tx_completion_port_t *)up;
	loop = port->port_loop;
	timeout = 0; // get_from_loop

	for ( ; ; ) {
		result = GetQueuedCompletionStatus(port->port_handle,
				&transfered_bytes, &completion_key, &overlapped, 0);
		if (result == FALSE &&
			overlapped == NULL &&
			GetLastError() == WAIT_TIMEOUT) {
			TX_PRINT(TXL_MESSAGE, "completion port is clean");
			break;
		}

		TX_CHECK(overlapped == NULL, "could not get any event from port");

	}

	result = GetQueuedCompletionStatus(port->port_handle,
			&transfered_bytes, &completion_key, &overlapped, timeout);

	tx_poll(loop, &port->port_task);
	return;

}
#endif

tx_poll_t* tx_completion_port_init(tx_loop_t *loop)
{
#ifdef WIN32
	WSADATA wsadata;
	HANDLE handle = INVALID_HANDLE_VALUE;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	tx_completion_port_t *poll = (tx_completion_port_t *)malloc(sizeof(tx_completion_port_t));
	TX_CHECK(poll == NULL, "create completion port failure");

	handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	TX_CHECK(handle == INVALID_HANDLE_VALUE, "create completion port failure");

	if (poll != NULL && handle != INVALID_HANDLE_VALUE) {
		poll->port_loop = loop;
		poll->port_handle = handle;
		tx_task_init(&poll->port_task, tx_completion_port_polling, poll);
		tx_poll(loop, &poll->port_task);
		return &poll->port_poll;
	}

	CloseHandle(handle);
	free(poll);
#endif

	return NULL;
}
