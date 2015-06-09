/* 
 * Front end for DART client to work with 'file' objects.
 *
 * Ciprian Docan (2009) TASSL Rutgers University
 *
 *  The redistribution of the source code is subject to the terms of version 
 *  2 of the GNU General Public License: http://www.gnu.org/licenses/gpl.html.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "dart_rpc.h"
#include "dc_base.h"
#include "dc_files.h"

#define MAX_FILES       10024
#define MAX_FNAME_LEN   1024

#define uloga(f, a...)  fprintf(stderr, f, ##a)
#define ulog_err(f, a...) uloga(f ": %s [%d].\n", ##a, strerror(errno), errno)

/* Structure for file objects. */
struct dcf_file {
	struct list_head file_entry;

	/* Index into the open 'file' object table. */
	int index;

	enum open_mode mode;

	char *fname;
	void *buf;
	size_t size;
	size_t offset;

	int num_inst;

	/* 0 - OK, 1 - reading, <0 Error. */
	int status;

	int version;
};

struct dc_files {
	struct dart_client *dc;

	/* Array of application open 'file' objects. */
	struct dcf_file *file_tab[MAX_FILES];
};

static struct dc_files *dcf_instance = 0;

/* Forward declaration. */
static inline void dcf_file_free(struct dcf_file *);
static int dcf_file_enlarge(struct dcf_file *, size_t);

static inline struct dc_files *dcf_ref_from_rpc(struct rpc_server *rpc_s)
{
	struct dart_client *dc = dc_ref_from_rpc(rpc_s);
	return dc->dart_ref;
}

/*
  Hash distribution function.
*/
static unsigned int fname_hash(const char *key, int len)
{
	unsigned int hash = len;
	int i;

	for(i = 0; i < len; i++)
		hash = 33 * hash + key[i];

	return hash;
}


/*
  TODO: unused routine, should erase it !
  RPC routine to receive acks for 'file' object transfers.
*/
static int dcfrpc_ack(struct rpc_server *rpc, struct rpc_cmd *cmd)
{
	struct dart_client *dc = dc_ref_from_rpc(rpc);
	struct node_id *peer = dc_get_peer(dc, cmd->id);

	update_num_msg(peer, cmd->num_msg);

	return 0;
}

/* UNUSED ? */
static int dcf_send_completion(void *dart_ref, struct msg_buf *msg)
{
	struct dc_files *dcf = ((struct dart_client *) dart_ref)->dart_ref;

	dcf->dc->num_posted--;

	free(msg->msg_data);
	free(msg);

	// uloga("'%s()' transfer complete.\n", __func__);
	return 0;
}

static int dcf_send_file_completion(struct rpc_server *rpc_s, struct msg_buf *msg)
{
	struct dc_files *dcf = dcf_ref_from_rpc(rpc_s);
	struct dcf_file *dco = msg->private;

	// uloga("'%s()' transfer complete for file %s.\n", 
	//        __func__, dco->fname);

	dcf->dc->num_posted--;

	dcf_file_free(dco);
	free(msg);

	return 0;
}

static int dcf_send_file(struct dc_files *dcf, struct dcf_file *dco)
{
	struct msg_buf *msg;
	struct rfshdr *rs;
	struct node_id *peer;
	int err = -ENOMEM;

	unsigned int peer_id;
	peer_id = fname_hash(dco->fname, strlen(dco->fname)) % dcf->dc->num_sp;
	peer = dc_get_peer(dcf->dc, peer_id);

	msg = msg_buf_alloc(dcf->dc->rpc_s, peer, 1);
	if(!msg)
		goto err_out;

	msg->msg_rpc->cmd = cn_data;
	msg->msg_rpc->id = dcf->dc->self->id;

	/* Store a reference to the 'file' object for completion. */
	msg->private = dco;
	msg->cb = dcf_send_file_completion;
	msg->msg_data = dco->buf;
	msg->size = dco->size;

	rs = (struct rfshdr *) msg->msg_rpc->pad;
	strcpy((char *) rs->fname, dco->fname);
	rs->size = dco->offset;
	rs->base_offset = 0;
	rs->num_inst = dco->num_inst;

	err = rpc_send(dcf->dc->rpc_s, peer, msg);
	if(err == 0) {
		dcf->dc->num_posted++;
		return 0;
	}

	dcf_file_free(dco);
	free(msg);
      err_out:
	uloga("'%s()' failed\n", __func__);
	return err;
}

static int dcf_read_file_completion(struct rpc_server *rpc_s, struct msg_buf *msg)
{
	struct dc_files *dcf = dcf_ref_from_rpc(rpc_s);
	struct dcf_file *dco = msg->private;

	dcf->dc->num_posted--;
	dco->status = 0;

	free(msg);

	return 0;
}

/**
 * Rpc service to respond to a 'cn_read' rpc command.
 *
 * This should complete the second step of a read file operation.
 */
static int dcfrpc_read_file(struct rpc_server *rpc_s, struct rpc_cmd *cmd)
{
	struct dc_files *dcf = dcf_ref_from_rpc(rpc_s);
	struct rfshdr *rs = (struct rfshdr *) cmd->pad;
	struct dcf_file *dco = dcf->file_tab[rs->index];
	struct node_id *ppeer, peer;
	struct msg_buf *msg;

	int err;

	ppeer = dc_get_peer(dcf->dc, cmd->id);
	update_num_msg(ppeer, cmd->num_msg);

	if(rs->rc < 0) {
		dcf->dc->num_posted--;
		dco->status = rs->rc;

		uloga("'%s()' failed at server with %d.\n", __func__, rs->rc);
		return 0;
	}

	if(dco->offset + rs->size > dco->size) {
		err = dcf_file_enlarge(dco, rs->size);
		if(err < 0) {
			uloga("'%s:' not enough memory to read file, " "truncating", __func__);
			rs->size = dco->size;
		}
	}

	/* Extract the piggybacked version for m3d file. */
	dco->version = ((int *) rs->fname)[0];

	err = -ENOMEM;

	msg = msg_buf_alloc(rpc_s, ppeer, 0);
	if(!msg)
		goto err_out;

	msg->private = dco;
	msg->cb = dcf_read_file_completion;
	msg->msg_data = dco->buf;
	msg->size = rs->size;	// TODO: reconsider, use dco->size and
	// adjust it properly !
	peer = *ppeer;
	peer.mb = cmd->mbits;
	peer.pi = 0;
	err = rpc_receive_direct(rpc_s, &peer, msg);
	if(err == 0) {
		// uloga("'%s()': read %s successfull.\n", __func__, rs->fname);
		return 0;
	}

	free(msg);
      err_out:
	dco->status = err;
	uloga("'%s:' failed, this may block server.\n", __func__);
	return err;
}

/**
 * Initiate the first step of a read file operation.
 *
 * Reading a file is a two steps process. First we send a read request
 * to the 'space', and second the 'space' will send us the file.
 */
static int dcf_init_read_file(struct dc_files *dcf, struct dcf_file *dco)
{
	struct msg_buf *msg;
	struct rfshdr *rs;
	struct node_id *peer;
	int err = -ENOMEM;

	unsigned int peer_id;
	peer_id = fname_hash(dco->fname, strlen(dco->fname)) % dcf->dc->num_sp;
	peer = dc_get_peer(dcf->dc, peer_id);

	/* Mark file as reading. */
	dco->status = 1;

	msg = msg_buf_alloc(dcf->dc->rpc_s, peer, 1);
	if(!msg)
		goto err_out;

	msg->private = dco;

	msg->msg_rpc->cmd = cn_init_read;
	msg->msg_rpc->id = dcf->dc->self->id;

	rs = (struct rfshdr *) msg->msg_rpc->pad;
	strcpy((char *) rs->fname, dco->fname);
	rs->index = dco->index;

	err = rpc_send(dcf->dc->rpc_s, peer, msg);
	if(err == 0) {
		dcf->dc->num_posted++;
		return 0;
	}

	free(msg);
      err_out:
	dco->status = err;
	uloga("'%s()' failed %d.\n", __func__, err);
	return err;
}

struct dcf_file *dcf_file_alloc(void)
{
	struct dcf_file *dco;
	size_t size;

	size = sizeof(struct dcf_file) + MAX_FNAME_LEN + 1;
	dco = calloc(1, size);
	if(!dco)
		return NULL;
	dco->fname = (char *) (dco + 1);
	dco->num_inst = 1;

	/* Assume as starting point the file size is 10k. */
	dco->size = 1024 * 10;
	dco->buf = malloc(dco->size);
	if(!dco->buf) {
		free(dco);
		dco = NULL;
	}

	return dco;
}

int dcf_getid(struct dc_files *dcf)
{
	return dcf->dc->self->id;
}

static inline void dcf_file_free(struct dcf_file *dco)
{
	free(dco->buf);
	free(dco);
}

static int dcf_file_enlarge(struct dcf_file *dco, size_t by_size)
{
	size_t extra = 10 * 1024;
	size_t new_size;

	new_size = dco->size + by_size + extra;
	dco->buf = realloc(dco->buf, new_size);
	if(dco->buf)
		dco->size = new_size;

	return dco->buf ? 0 : -ENOMEM;
}

static ssize_t dcf_file_read(struct dcf_file *dco, void *buf, size_t size)
{
	if(dco->offset + size > dco->size) {
		if(dco->offset >= dco->size)
			return -1;
		size = dco->size - dco->offset;
	}

	memcpy(buf, dco->buf + dco->offset, size);
	dco->offset += size;
	return size;
}

static ssize_t dcf_file_write(struct dcf_file *dco, void *buf, size_t size)
{
	int err;

	if(dco->offset + size > dco->size) {
		err = dcf_file_enlarge(dco, size);
		if(err < 0)
			return err;
	}

	memcpy(dco->buf + dco->offset, buf, size);
	dco->offset += size;

	return size;
}

/* 
   Public API begins here. 
*/

struct dc_files *dcf_alloc(int num_peers)
{
	struct dc_files *dcf;

	if(dcf_instance)
		return dcf_instance;

	dcf = calloc(1, sizeof(struct dc_files));
	if(!dcf)
		goto err_out;

	//TODO: more initialization here.

	rpc_add_service(cn_read, dcfrpc_read_file);

	dcf->dc = dc_alloc(num_peers, dcf);
	if(!dcf->dc)
		goto err_out_free;

	dcf->dc->dart_ref = dcf;

	dcf_instance = dcf;

	return dcf;
      err_out_free:
	free(dcf);
      err_out:
	uloga("'%s()': failed.\n", __func__);
	return NULL;
}

void dcf_free(struct dc_files *dcf)
{
	int i;

	dc_free(dcf->dc);
	for(i = 0; i < MAX_FILES; i++)
		if(dcf->file_tab[i])
			dcf_file_free(dcf->file_tab[i]);
}

/* UNUSED ? */
int dcf_send(struct dc_files *dcf, void *buf, size_t size)
{
	struct msg_buf *msg;
	struct rfshdr *rs;
	struct node_id *peer;
	int err;

	peer = dc_get_peer(dcf->dc, 0);

	msg = msg_buf_alloc(dcf->dc->rpc_s, peer, 1);
	if(!msg)
		goto err_out;

	msg->msg_rpc->cmd = cn_data;
	msg->msg_rpc->id = dcf->dc->self->id;

	rs = (struct rfshdr *) msg->msg_rpc->pad;
	strcpy((char *) rs->fname, "m3d.in");
	rs->size = size;
	rs->base_offset = 0;

	msg->msg_data = buf;
	msg->size = size;
	msg->cb = dcf_send_completion;

	err = rpc_send(dcf->dc->rpc_s, peer, msg);
	if(err == 0) {
		dcf->dc->num_posted++;
		return 0;
	}

	free(msg);
      err_out:
	uloga("'%s()' failed\n", __func__);
	return -1;
}

int dcf_open(char *fname, int mode)
{
	struct dc_files *dcf = dcf_instance;
	struct dcf_file *dco;
	int index = 0, err = -ENOMEM;

	while(index < MAX_FILES && dcf->file_tab[index])
		index++;

	if(index == MAX_FILES)
		return err;

	dco = dcf_file_alloc();
	if(!dco)
		return err;

	dco->index = index;
	strncpy(dco->fname, fname, MAX_FNAME_LEN);
	dco->mode = mode;
	dcf->file_tab[index] = dco;

	if(mode == o_read) {
		err = dcf_init_read_file(dcf, dco);
		if(err < 0)
			return err;
	}

	return index;
}

ssize_t dcf_read(int index, void *buf, size_t size)
{
	struct dc_files *dcf = dcf_instance;
	struct dcf_file *dco;
	int err;

	if(index < 0 || index > MAX_FILES)
		return -EBADF;

	dco = dcf->file_tab[index];
	if(!dco)
		return -ENOENT;

	while(dco->status == 1)
		rpc_process_event(dcf->dc->rpc_s);

	if(dco->status < 0)
		return -EIO;

	err = dcf_file_read(dco, buf, size);
	if(err < 0)
		return -EIO;

	return 0;
}

ssize_t dcf_write(int index, void *buf, size_t size)
{
	struct dc_files *dcf = dcf_instance;
	struct dcf_file *dco;

	if(index < 0 || index >= MAX_FILES)
		return -EBADF;

	dco = dcf->file_tab[index];
	if(!dco)
		return -ENOENT;

	if(dco->mode != o_write)
		return -EACCES;

	return dcf_file_write(dco, buf, size);
}

/*
  Sets the number of instances that can be consumed/extracted from the
  shared space.
*/
int dcf_set_num_instances(int index, int num_inst)
{
	struct dc_files *dcf = dcf_instance;
	struct dcf_file *dco;

	if(index < 0 || index >= MAX_FILES)
		return -ENOENT;

	dco = dcf->file_tab[index];
	if(!dco)
		return -ENOENT;

	if(num_inst > 0)
		dco->num_inst = num_inst;

	return 0;
}

int dcf_close(int index)
{
	struct dc_files *dcf = dcf_instance;
	struct dcf_file *dco;

	if(index < 0 || index >= MAX_FILES)
		return -EBADF;

	dco = dcf->file_tab[index];
	if(!dco)
		return -ENOENT;

	dcf->file_tab[index] = NULL;

	if(dco->mode == o_write) {
		dco->size = dco->offset;
		dco->index = -1;
		return dcf_send_file(dcf, dco);
	} else
		dcf_file_free(dco);

	return 0;
}

int dcf_get_version(int index)
{
	struct dc_files *dcf = dcf_instance;
	struct dcf_file *dco;

	if(index < 0 || index > MAX_FILES)
		return -EBADF;

	dco = dcf->file_tab[index];
	if(!dco)
		return -ENOENT;

	return dco->version;
}

int dcf_get_credits(int index)
{
	struct dc_files *dcf = dcf_instance;
	struct dcf_file *dco;
	int peer_id;
	struct node_id *peer;

	if(index < 0 || index > MAX_FILES)
		return -EBADF;

	dco = dcf->file_tab[index];
	if(!dco)
		return -ENOENT;

	peer_id = fname_hash(dco->fname, strlen(dco->fname)) % dcf->dc->num_sp;
	peer = dc_get_peer(dcf->dc, peer_id);

	return peer->num_msg_at_peer;
}

int dcf_get_peerid(int index)
{
	struct dc_files *dcf = dcf_instance;
	struct dcf_file *dco;
	int peer_id;

	if(index < 0 || index > MAX_FILES)
		return -EBADF;

	dco = dcf->file_tab[index];
	if(!dco)
		return -ENOENT;

	peer_id = fname_hash(dco->fname, strlen(dco->fname)) % dcf->dc->num_sp;

	return peer_id;
}
