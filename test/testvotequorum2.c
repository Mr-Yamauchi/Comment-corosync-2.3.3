/*
 * Copyright (c) 2009-2014 Red Hat, Inc.
 *
 * All rights reserved.
 *
 * Author: Christine Caulfield (ccaulfie@redhat.com)
 *
 * This software licensed under BSD license, the text of which follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <corosync/corotypes.h>
#include <corosync/votequorum.h>

static votequorum_handle_t handle;

static int print_info(int ok_to_fail)
{
	struct votequorum_info info;
	int err;

	if ( (err=votequorum_getinfo(handle, VOTEQUORUM_QDEVICE_NODEID, &info)) != CS_OK) {
		fprintf(stderr, "votequorum_getinfo error %d: %s\n", err, ok_to_fail?"OK":"FAILED");
		return -1;
	}
	else {
		printf("name           %s\n", info.qdevice_name);
		printf("qdevice votes  %d\n", info.qdevice_votes);
		if (info.flags & VOTEQUORUM_INFO_QDEVICE_ALIVE) {
			printf("alive ");
		}
		if (info.flags & VOTEQUORUM_INFO_QDEVICE_CAST_VOTE) {
			printf("cast-vote ");
		}
		if (info.flags & VOTEQUORUM_INFO_QDEVICE_MASTER_WINS) {
			printf("master-wins");
		}
		printf("\n\n");
	}
	return 0;
}

static void usage(const char *command)
{
  printf("%s [-p <num>] [-t <time>] [-n <name>] [-c] [-m]\n", command);
        printf("      -p <num>  Number of times to poll qdevice (default 0=infinte)\n");
        printf("      -t <secs> Time (in seconds) to wait between polls (default=1)\n");
        printf("      -n <name> Name of quorum device (default QDEVICE)\n");
        printf("      -c        Cast vote (default yes)\n");
        printf("      -q        Don't print device status every poll time (default=will print)\n");
        printf("      -m        Master wins (default no)\n");
        printf("      -1        Print status once and exit\n");
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int cast_vote = 1, master_wins = 0;
	int pollcount=0, polltime=1, quiet=0, once=0;
	int err;
	int opt;
	const char *devicename = "QDEVICE";
	const char *options = "n:p:t:cmq1h";
	
	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
		case 'm':
		        master_wins = 1;
			break;
		case 'c':
		        cast_vote = 1;
			break;
		case '1':
		        once = 1;
			break;
		case 'q':
		        quiet = 1;
			break;
		case 'p':
		        pollcount = atoi(optarg)+1;
			break;
		case 'n':
		        devicename = strdup(optarg);
			break;
		case 't':
		        polltime = atoi(optarg);
			break;
		case 'h':
		        usage(argv[0]);
			exit(0);
		}
	}

	if ( (err=votequorum_initialize(&handle, NULL)) != CS_OK) {
		fprintf(stderr, "votequorum_initialize FAILED: %d\n", err);
		return -1;
	}

	if (quiet && once) {
	        fprintf(stderr, "setting both -q (quet) and -1 (once) makes no sense\n");
		usage(argv[0]);
		exit(1);
	}

	if (!quiet) {
	        print_info(1);
	}
	if (once) {
	        exit(0);
	}

	if (argc >= 2) {
		if ( (err=votequorum_qdevice_register(handle, devicename)) != CS_OK) {
			fprintf(stderr, "qdevice_register FAILED: %d\n", err);
			ret = -1;
			goto out;
		}

		if ( (err=votequorum_qdevice_master_wins(handle, devicename, master_wins)) != CS_OK) {
			fprintf(stderr, "qdevice_master_wins FAILED: %d\n", err);
			ret = -1;
			goto out;
		}

		while (--pollcount) {
		        if (!quiet) print_info(0);
			if ((err=votequorum_qdevice_poll(handle, devicename, cast_vote)) != CS_OK) {
				fprintf(stderr, "qdevice poll FAILED: %d\n", err);
				ret = -1;
				goto out;
			}
			if (!quiet) print_info(0);
			sleep(polltime);
		}
		if ((err= votequorum_qdevice_unregister(handle, devicename)) != CS_OK) {
			fprintf(stderr, "qdevice unregister FAILED: %d\n", err);
			ret = -1;
			goto out;
		}
	}

        if (!quiet) print_info(1);

out:
	votequorum_finalize(handle);
	return ret;
}
