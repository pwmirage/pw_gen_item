/* SPDX-License-Identifier: MIT
 * Copyright(c) 2021 Darek Stojaczyk for pwmirage.com
 */

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"
#include "pw_data.h"

static void *g_data_man;

struct conn_req {
	uint32_t type;
	uint32_t len;
	char *data;
};

struct conn_resp {
	uint32_t type;
	uint32_t len;
	uint32_t rc;
	char data[0];
};

#ifdef DEBUG
#define LOG_DEBUG(...) fprintf(stderr, __VAR_ARGS__);
#else
#define LOG_DEBUG(...)
#endif

#define CONN_REQ_TYPE_ITEM_ID 1

static void
gen_new_item(struct conn_req *req, struct conn_resp *resp)
{
	struct pw_item_tag_t tag = {0, 0};
	struct pw_item *item;
	uint32_t id;
	size_t len = 0;

	if (req->len < 4) {
		resp->rc = EINVAL;
		return;
	}

	id = ntohl(*(uint32_t *)req->data);

	item = pw_generate_item_from_player(g_data_man, id, &tag, sizeof(tag));
	if (!item) {
		resp->rc = EFAULT;
		return;
	}

	memcpy(resp->data, item, sizeof(*item) - 8);
	len = htonl(item->content_length) | 0xC0;
	memcpy(resp->data + sizeof(*item) - 8, &len, 4);
	memcpy(resp->data + sizeof(*item) - 4, item->item_content, item->content_length);

	pw_free_item(item);

	resp->rc = 0;
	resp->len = sizeof(*item) + item->content_length;
}

static int
parse_varint32(char *buf, int buflen, uint32_t *u32)
{
	unsigned char first_byte = buf[0];

	switch(first_byte & 0xE0) {
		case 0xE0:
			*u32 = ntohl(*(uint32_t *)&buf[1]);
			return buflen >= 5 ? 0 : EINVAL;
		case 0xC0:
			*u32 = ntohl(*(uint32_t *)&buf[0]) & 0x3FFFFFFF; 
			return buflen >= 4 ? 0 : EINVAL;
		case 0x80:
		case 0xA0:
			*u32 = ntohs(*(uint16_t *)&buf[0]) & 0x7FFF;
			return buflen >= 2 ? 0 : EINVAL;
	}

	*u32 = first_byte;
	return 1;
}

static void
on_connection(int connfd)
{
	char buf[1029];
	char resp_buf[1024];
	int rc, read_bytes, resp_bytes;
	struct conn_req req;
	struct conn_resp *resp = (void *)resp_buf;
	bool do_run = true;

	while (do_run) {
		uint32_t buf_off = 0;

		/* keep 5 spare bytes to skip parse_varint32() overflow checks */
		read_bytes = read(connfd, buf, sizeof(buf) - 5);
		if (read_bytes <= 0) {
			do_run = false;
			break;
		}


		while (buf_off < read_bytes) {
			LOG_DEBUG("buff_off=%d, read_bytes=%d\n", buf_off, read_bytes);
			uint32_t pktlen = 0;
			pktlen += parse_varint32(buf + buf_off, sizeof(buf) - buf_off, &req.type);
			pktlen += parse_varint32(buf + buf_off + pktlen, sizeof(buf) - buf_off - pktlen, &req.len);
			LOG_DEBUG("setting data off to %d\n", buf_off + pktlen);
			req.data = buf + buf_off + pktlen;

			if (read_bytes < pktlen + req.len) {
				fprintf(stderr, "got incomplete req (len=%d,buflen=%d)\n", pktlen + req.len, read_bytes);
				break;
			}

			pktlen += req.len;
			buf_off += pktlen;

			/* to be overriden */
			resp->type = req.type;
			resp->len = 0;
			resp->rc = EIO;

			LOG_DEBUG("got req.type=%d\n", req.type);
			switch (req.type) {
			case CONN_REQ_TYPE_ITEM_ID:
				gen_new_item(&req, resp);
				break;
			default:
				resp->rc = ENOSYS;
				resp->len = 0;
				break;
			}

			resp_bytes = sizeof(*resp) + resp->len;

			LOG_DEBUG("replying resp.rc=%d, resp.len=%d\n", resp->rc, resp->len);

			/* make rc a part of the dynamic data */
			resp->len += 4;

			resp->type = htonl(resp->type) | 0xC0;
			resp->len = htonl(resp->len) | 0xC0;
			rc = write(connfd, (void *)resp, resp_bytes);
			if (rc <= 0) {
				do_run = false;
				break;
			}
		}
	}
}

static void
real_main(void)
{
	int sockfd, connfd;
	unsigned len;
	struct sockaddr_in servaddr = {}, cli = {};

	g_data_man = pw_get_data_man();
	fprintf(stderr, "Detoured!\n");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket()");
		exit(1);
	}

	len = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &len, sizeof(int));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(55075);

	if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
		perror("bind()");
		exit(1);
	}

	if ((listen(sockfd, 5)) != 0) {
		perror("listen()");
		exit(1);
	}

	len = sizeof(cli);
	while (1) {
		connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
		if (connfd < 0) {
			perror("accept()");
			exit(1);
		}

		on_connection(connfd);
		close(connfd);
	}
	
	close(sockfd);
	exit(0);
}

static void __attribute__((constructor))
init(void)
{
	detour(0x804d7bf, (uintptr_t)real_main);

	fprintf(stderr, ".SO hooked\n");
}
