#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "arpa/inet.h"
//#include "config.h"
//#include "list.h"
#include <getopt.h>
#include "unistd.h"
#include <stdio.h>
#include <signal.h>
#include "ds_gspace.h"

static int num_sp;
static int num_cp;
static char *conf;

static struct ds_gspace *dsg;

struct {
	int		a, b, c;
	double		x, y, z;
} f_point_;


static void usage(void)
{
        printf("Usage: server OPTIONS\n"
		"OPTIONS: \n"
		"--server, -s    Number of server instance/staging nodes\n"
	        "--cnodes, -c    Number of compute nodes\n"
		"--conf, -f	 Define configuration file\n");
}

static int parse_args(int argc, char *argv[])
{
        const char opt_short[] = "s:c:";
        const struct option opt_long[] = {
		{"server",	1,	NULL,	's'},
		{"cnodes",	1,	NULL,	'c'},
		{"conf",	1,	NULL,	'f'},
		{NULL,		0,	NULL,	0}
	};

        int opt;

	while ((opt = getopt_long(argc, argv, opt_short, opt_long, NULL)) != -1) {
		switch (opt) {
                case 's':
			num_sp = (optarg) ? atoi(optarg) : -1;
                        break;
	
                case 'c':
			num_cp = (optarg) ? atoi(optarg) : -1;
                        break;

		case 'f':
			conf = (optarg) ? optarg : NULL;
			break;

                default:
			printf("Unknown argument \n");
                }
	}


	if (num_sp <= 0)
		num_sp = 1;
	if (num_cp == 0)
		num_cp = 1;
	if (!conf)
		conf = "dataspaces.conf";
	return 0;
}

static int srv_init(void)
{
	dsg = dsg_alloc(num_sp, num_cp, conf);
	if (!dsg) {
		return -1;
	}
	return 0;
}

static int srv_run(void)
{
	int err;

        while (!dsg_complete(dsg)) {
//printf("I am running.\n");
                err = dsg_process(dsg);
                if (err < 0) {
			/* If there is an error on the execution path,
			   I should stop the server. */

			dsg_free(dsg);

			/* TODO:  implement an  exit method  to signal
			   other servers to stop. */

			printf("Server exits due to error %d.\n", err);

			return err;
		}
        }

	return 0;
}

static int srv_finish(void)
{
        dsg_barrier(dsg);

        dsg_free(dsg);

	return 0;
}

int main(int argc, char *argv[])
{
	if (parse_args(argc, argv) < 0) {
		usage();
                return -1;
        }

	if (srv_init() < 0) {
		printf("DART server init failed!\n");
		return -1;
	}

	if (srv_run() < 0) {
		printf("DART server got an error at runtime!\n");
		return -1;
	}

	srv_finish();

	printf("All ok.\n");

        return 0;
}
