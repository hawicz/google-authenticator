#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "base32.h"

/*
 * Convert (almost) arbitrary input on the command line to a base-32 encoded string.
 *
 * ~/google-authenticator/libpam/base32 e $(awk '{ print $2 }' < gauth.seed | xxd -r -p)
 */

static void usage(const char *argv0, int exitval, char *errmsg) __attribute__((noreturn));
static void usage(const char *argv0, int exitval, char *errmsg)
{
	FILE *f = stdout;
	if (errmsg)
	{
		fprintf(stderr, "ERROR: %s\n", errmsg);
		f = stderr;
	}
	
	fprintf(f, "Usage: %s -e [<file>]\n", argv0);
	fprintf(f, "Usage: %s -D [<file>]\n", argv0);
	fprintf(f, "Usage: %s -d <value>\n", argv0);
	fprintf(f, "  Emits <value> encoded in/decoded from base-32 on stdout.\n");
	fprintf(f, "  When encoding, the file should contain raw binary data.\n");
	fprintf(f, "   If no filename is specified, it reads from stdin.\n");
	fprintf(f, "  All output is written to stdout.\n");

	exit(exitval);
}

int main(int argc, char *argv[]) {
	int c;
	int mode = 0;
	while ((c = getopt(argc, argv, "edDh")) != -1)
	{
		switch (c)
		{
		case 'e':
			mode = 1;
			break;
		case 'd':
			mode = 2;
			break;
		case 'D':
			mode = 3;
			break;
		case 'h':
			usage(argv[0], 0, NULL);
			break;
		default:
			usage(argv[0], 1, "Unknown command line argument");
			break;
		}
	}

	if (mode == 0)
	{
		usage(argv[0], 1, "A mode of operation must be chosen.");
	}

	int retval;
	if (mode == 1 || mode == 2)
	{
		if (argc - optind > 1)
			usage(argv[0], 1, "Too many args");

		const char *binfile = (optind < argc) ? argv[optind] : "-";
		int d = strcmp(binfile, "-") == 0 ? STDIN_FILENO : open(binfile, O_RDONLY);
		if (d < 0)
			err(1, "Failed to open %s: %s\n", binfile, strerror(errno));
		struct stat st;
		memset(&st, 0, sizeof(st));
		if (fstat(d, &st) < 0 || st.st_size == 0)
			st.st_size = 5 * 1024;  // multiple of 5 to avoid internal padding
			                        // AND multiple of 8 to ensure we feed
			                        //  valid data to base32_decode().
		uint8_t *input = malloc(st.st_size + 1);
		int amt_read;
		int amt_to_read = st.st_size;
		errno = 0;
		while ((amt_read = read(d, input, amt_to_read)) > 0 || errno == EINTR)
		{
			if (errno == EINTR)
				continue;

			// Encoding: 8 bytes out for every 5 input, plus up to 6 padding, and nul
			// Decoding: up to 5 bytes out for every 8 input.
			int result_avail = (mode == 1) ?
			                     ((amt_read + 4) / 5 * 8 + 6 + 1) :
			                     ((amt_read + 7) / 8 * 5) ;
			uint8_t *result = malloc(result_avail);

			input[amt_read] = '\0';

			if (mode == 1)
			{
				retval = base32_encode(input, amt_read, result, result_avail, 1);
			}
			else
			{
				retval = base32_decode(input, result, result_avail);
			}
			if (retval < 0)
			{
				fprintf(stderr, "%s failed.  Input too long?\n", (mode == 1) ? "base32_encode" : "base32_decode");
				exit(1);
			}
			//printf("%s", result);
			if (write(STDOUT_FILENO, result, retval)) { /* squash gcc warnings */ }
			fflush(stdout);
			free(result);
		}
		if (amt_read < 0)
			err(1, "Failed to read from %s: %s\n", binfile, strerror(errno));
		if (mode == 1)
			printf("\n");
	} else { // mode == 3
		if (argc - optind < 1)
			usage(argv[0], 1, "Not enough args");

		const char *base32_value = argv[2];
		int result_avail = strlen(base32_value) + 1;
		uint8_t *result = malloc(result_avail);

		retval = base32_decode((uint8_t *)base32_value, result, result_avail);
		if (retval < 0)
		{
			fprintf(stderr, "base32_decode failed.  Input too long?\n");
			exit(1);
		}
		if (write(STDOUT_FILENO, result, retval)) { /* squash gcc warnings */ }
	}
	return 0;
}
