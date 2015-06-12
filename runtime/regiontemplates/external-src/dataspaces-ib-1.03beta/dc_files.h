/*
 * Ciprian Docan (2009) TASSL Rutgers University
 *
 * The redistribution of the source code is subject to the terms of version 
 * 2 of the GNU General Public License: http://www.gnu.org/licenses/gpl.html.
 */

#ifndef __DC_FILES_H_
#define __DC_FILES_H_

struct dc_files;

enum open_mode {
	o_read = 1,
	o_write
};

struct dc_files *dcf_alloc(int);
void dcf_free(struct dc_files *);
int dcf_send(struct dc_files *, void *, size_t);

int dcf_getid(struct dc_files *);

int dcf_open(char *, int);
ssize_t dcf_write(int, void *, size_t);
ssize_t dcf_read(int, void *, size_t);
int dcf_set_num_instances(int, int);
int dcf_close(int);

int dcf_get_version(int);

int dcf_get_credits(int);
int dcf_get_peerid(int);

#endif
