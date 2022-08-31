#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define _GNU_SOURCE
#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>

/* from kernel-sources(samples/statx/test-statx.c) */
static void print_time(const char *field, struct statx_timestamp *ts)
{
        struct tm tm;
        time_t tim;
        char buffer[100];
        int len;

        tim = ts->tv_sec;
        if (!localtime_r(&tim, &tm)) {
                perror("localtime_r");
                exit(1);
        }
        len = strftime(buffer, 100, "%F %T", &tm);
        if (len == 0) {
                perror("strftime");
                exit(1);
        }
        printf("%s", field);
        fwrite(buffer, 1, len, stdout);
        printf(".%09u", ts->tv_nsec);
        len = strftime(buffer, 100, "%z", &tm);
        if (len == 0) {
                perror("strftime2");
                exit(1);
        }
        fwrite(buffer, 1, len, stdout);
        printf("\n");
}

/* from kernel-sources(samples/statx/test-statx.c) */
static void dump_statx(struct statx *stx)
{
	char buffer[256], ft = '?';

	printf("results=%x\n", stx->stx_mask);

	printf(" ");
	if (stx->stx_mask & STATX_SIZE)
		printf(" Size: %-15llu", (unsigned long long)stx->stx_size);
	if (stx->stx_mask & STATX_BLOCKS)
		printf(" Blocks: %-10llu", (unsigned long long)stx->stx_blocks);
	printf(" IO Block: %-6llu", (unsigned long long)stx->stx_blksize);
	if (stx->stx_mask & STATX_TYPE) {
		switch (stx->stx_mode & S_IFMT) {
		case S_IFIFO:	printf("  FIFO\n");			ft = 'p'; break;
		case S_IFCHR:	printf("  character special file\n");	ft = 'c'; break;
		case S_IFDIR:	printf("  directory\n");		ft = 'd'; break;
		case S_IFBLK:	printf("  block special file\n");	ft = 'b'; break;
		case S_IFREG:	printf("  regular file\n");		ft = '-'; break;
		case S_IFLNK:	printf("  symbolic link\n");		ft = 'l'; break;
		case S_IFSOCK:	printf("  socket\n");			ft = 's'; break;
		default:
			printf(" unknown type (%o)\n", stx->stx_mode & S_IFMT);
			break;
		}
	} else {
		printf(" no type\n");
	}

	sprintf(buffer, "%02x:%02x", stx->stx_dev_major, stx->stx_dev_minor);
	printf("Device: %-15s", buffer);
	if (stx->stx_mask & STATX_INO)
		printf(" Inode: %-11llu", (unsigned long long) stx->stx_ino);
	if (stx->stx_mask & STATX_NLINK)
		printf(" Links: %-5u", stx->stx_nlink);
	if (stx->stx_mask & STATX_TYPE) {
		switch (stx->stx_mode & S_IFMT) {
		case S_IFBLK:
		case S_IFCHR:
			printf(" Device type: %u,%u",
			       stx->stx_rdev_major, stx->stx_rdev_minor);
			break;
		}
	}
	printf("\n");

	if (stx->stx_mask & STATX_MODE)
		printf("Access: (%04o/%c%c%c%c%c%c%c%c%c%c)  ",
		       stx->stx_mode & 07777,
		       ft,
		       stx->stx_mode & S_IRUSR ? 'r' : '-',
		       stx->stx_mode & S_IWUSR ? 'w' : '-',
		       stx->stx_mode & S_IXUSR ? 'x' : '-',
		       stx->stx_mode & S_IRGRP ? 'r' : '-',
		       stx->stx_mode & S_IWGRP ? 'w' : '-',
		       stx->stx_mode & S_IXGRP ? 'x' : '-',
		       stx->stx_mode & S_IROTH ? 'r' : '-',
		       stx->stx_mode & S_IWOTH ? 'w' : '-',
		       stx->stx_mode & S_IXOTH ? 'x' : '-');
	if (stx->stx_mask & STATX_UID)
		printf("Uid: %5d   ", stx->stx_uid);
	if (stx->stx_mask & STATX_GID)
		printf("Gid: %5d\n", stx->stx_gid);

	if (stx->stx_mask & STATX_ATIME)
		print_time("Access: ", &stx->stx_atime);
	if (stx->stx_mask & STATX_MTIME)
		print_time("Modify: ", &stx->stx_mtime);
	if (stx->stx_mask & STATX_CTIME)
		print_time("Change: ", &stx->stx_ctime);
	if (stx->stx_mask & STATX_BTIME)
		print_time(" Birth: ", &stx->stx_btime);

	if (stx->stx_attributes_mask) {
		unsigned char bits, mbits;
		int loop, byte;

		static char attr_representation[64 + 1] =
			/* STATX_ATTR_ flags: */
			"????????"	/* 63-56 */
			"????????"	/* 55-48 */
			"????????"	/* 47-40 */
			"????????"	/* 39-32 */
			"????????"	/* 31-24	0x00000000-ff000000 */
			"????????"	/* 23-16	0x00000000-00ff0000 */
			"???me???"	/* 15- 8	0x00000000-0000ff00 */
			"?dai?c??"	/*  7- 0	0x00000000-000000ff */
			;

		printf("Attributes: %016llx (", stx->stx_attributes);
		for (byte = 64 - 8; byte >= 0; byte -= 8) {
			bits = stx->stx_attributes >> byte;
			mbits = stx->stx_attributes_mask >> byte;
			for (loop = 7; loop >= 0; loop--) {
				int bit = byte + loop;

				if (!(mbits & 0x80))
					putchar('.');	/* Not supported */
				else if (bits & 0x80)
					putchar(attr_representation[63 - bit]);
				else
					putchar('-');	/* Not set */
				bits <<= 1;
				mbits <<= 1;
			}
			if (byte)
				putchar(' ');
		}
		printf(")\n");
	}
}

int main(void){
	int dirfd = AT_FDCWD;
	int flags = AT_SYMLINK_NOFOLLOW;
	unsigned int mask = STATX_ALL;
	struct statx stxbuf;
	int ret;

	ret = statx(dirfd,"./statx-test",flags,mask,&stxbuf);
	if( ret < 0)
		{
			perror("statx");
			return EXIT_FAILURE;
		}
		dump_statx(&stxbuf);
		printf("\n");

	return EXIT_SUCCESS;
}
