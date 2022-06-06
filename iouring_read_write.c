/* SPDX-License-Identifier: MIT */
/*
 * Very basic proof-of-concept for doing a copy with linked SQEs. Needs a
 * bit of error handling and short read love.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sdt.h>
#include "liburing.h"

#define QD 64
#define BS (4 * 1024)
char *infile;
char *outfile;
char *arg_iotype;
struct io_data
{
    size_t offset;
    int read_or_write; // 0 stand for read 1 stand for write
    int index;
    struct iovec iov;
};

static int infd, outfd;
static int inflight;
int is_read = 1;
static int setup_context(unsigned entries, struct io_uring *ring)
{
    int ret;

    ret = io_uring_queue_init(entries, ring, 0);
    if (ret < 0)
    {
        fprintf(stderr, "queue_init: %s\n", strerror(-ret));
        return -1;
    }

    return 0;
}

static int get_file_size(int fd, off_t *size)
{
    struct stat st;

    if (fstat(fd, &st) < 0)
        return -1;
    if (S_ISREG(st.st_mode))
    {
        *size = st.st_size;
        return 0;
    }
    else if (S_ISBLK(st.st_mode))
    {
        unsigned long long bytes;

        if (ioctl(fd, BLKGETSIZE64, &bytes) != 0)
            return -1;

        *size = bytes;
        return 0;
    }

    return -1;
}

static void queue_rw_pair(struct io_uring *ring, off_t size, off_t offset)
{
    struct io_uring_sqe *sqe;
    struct io_data *data;
    void *ptr;

    ptr = malloc(size + sizeof(*data));
    data = ptr + size;
    data->index = 0;
    data->offset = offset;
    data->iov.iov_base = ptr;
    data->iov.iov_len = size;

    sqe = io_uring_get_sqe(ring);
    io_uring_prep_readv(sqe, infd, &data->iov, 1, offset);
    sqe->flags |= IOSQE_IO_LINK;
    io_uring_sqe_set_data(sqe, data);

    sqe = io_uring_get_sqe(ring);
    io_uring_prep_writev(sqe, outfd, &data->iov, 1, offset);
    io_uring_sqe_set_data(sqe, data);
}

static int handle_cqe(struct io_uring *ring, struct io_uring_cqe *cqe)
{
    struct io_data *data = io_uring_cqe_get_data(cqe);
    int ret = 0;

    data->index++;

    if (cqe->res < 0)
    {
        printf("hello");
        if (cqe->res == -ECANCELED)
        {
            queue_rw_pair(ring, data->iov.iov_len, data->offset);
            inflight += 2;
        }
        else
        {
            printf("cqe error: %s\n", strerror(-cqe->res));
            ret = 1;
        }
    }

    if (data->index == 2)
    {
        void *ptr = (void *)data - data->iov.iov_len;

        free(ptr);
    }
    io_uring_cqe_seen(ring, cqe);
    return ret;
}

static int copy_file(struct io_uring *ring, off_t insize)
{
    struct io_uring_cqe *cqe;
    printf("insize : %ld ", insize);
    printf("inflight %d ", inflight);
    off_t this_size;
    off_t offset;

    offset = 0;
    while (insize)
    {
        int has_inflight = inflight;
        int depth;

        while (insize && inflight < QD)
        {
            this_size = BS;
            if (this_size > insize)
                this_size = insize;
            // queue_rw_pair(ring, this_size, offset);
            struct io_uring_sqe *sqe;
            struct io_data *data;
            void *ptr;

            printf("sizeof data %lu ", sizeof(*data));

            // int ret = posix_memalign(&ptr, 256, this_size + sizeof(*data));
            // char * buf;
            int ret = posix_memalign((void **)&ptr, 256, this_size + sizeof(*data));
            if (ret)
            {
                fprintf(stderr, "posix_memalign: %s\n",
                        strerror(ret));
                return -1;
            }
            // ptr = malloc(this_size + sizeof(*data));
            char *write;
            // write = malloc(this_size);
            ret = posix_memalign((void **)&write, 256, this_size + sizeof(*data));
            if (ret)
            {
                fprintf(stderr, "posix_memalign: %s\n",
                        strerror(ret));
                return -1;
            }
            memset(write, '?', sizeof(write));
            data = ptr + this_size;
            data->index = 0;
            data->offset = offset;
            data->iov.iov_base = ptr;
            data->iov.iov_len = this_size;

            if (is_read == 1)
            {
                sqe = io_uring_get_sqe(ring);
                io_uring_prep_readv(sqe, infd, &data->iov, 1, offset);
                sqe->flags |= IOSQE_IO_LINK;
                io_uring_sqe_set_data(sqe, data);
                inflight += 1;
            }

            else
            {
                // printf("write\n");

                // sqe = io_uring_get_sqe(ring);
                // io_uring_prep_readv(sqe, infd, &data->iov, 1, offset);
                // sqe->flags |= IOSQE_IO_LINK;
                // io_uring_sqe_set_data(sqe, data);
                data->iov.iov_base = write;

                sqe = io_uring_get_sqe(ring);
                io_uring_prep_writev(sqe, outfd, &data->iov, 1, offset);
                io_uring_sqe_set_data(sqe, data);
                inflight += 1;
            }
            offset += this_size;
            insize -= this_size;
            printf("inflight : %d ", inflight);
            printf("insize : %ld ", insize);
            printf("this_size : %ld ", this_size);
            printf("offset : %ld ", offset);
        }
        DTRACE_PROBE(yichuan, runcode_trace1_in);
        if (has_inflight != inflight)
            io_uring_submit(ring);
        DTRACE_PROBE(yichuan, runcode_trace1_out);
        if (insize)
            depth = QD;
        else
            depth = 1;
        while (inflight >= depth)
        {
            int ret;
            printf("trace");
            DTRACE_PROBE(yichuan1, runcode_trace2_in);
            ret = io_uring_wait_cqe(ring, &cqe);
            DTRACE_PROBE(yichuan1, runcode_trace2_out);

            if (ret < 0)
            {
                printf("wait cqe: %s\n", strerror(-ret));
                return 1;
            }
            if (handle_cqe(ring, cqe))
                return 1;
            inflight--;
        }
    }

    return 0;
}

int test_iouring()
{
    struct io_uring ring;
    off_t insize;
    int ret;

    printf("infile : %s", infile);
    printf("outfile : %s", outfile);
    infd = open(infile, O_RDONLY | O_CREAT);

    // infd = open(infile, O_RDONLY | O_CREAT | O_DIRECT);
    if (infd < 0)
    {
        perror("open infile");
        return 1;
    }
    outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT, 0644);
    if (outfd < 0)
    {
        perror("open outfile");
        return 1;
    }

    if (setup_context(QD, &ring))
        return 1;
    if (get_file_size(infd, &insize))
        return 1;

    printf("insize: %ld ", insize);
    ret = copy_file(&ring, insize);

    close(infd);
    close(outfd);
    io_uring_queue_exit(&ring);
    return ret;
}

int test_sync()
{

    off_t insize;
    // infd = open(infile, O_RDONLY | O_CREAT);

    infd = open(infile, O_RDONLY | O_CREAT | O_DIRECT);
    if (infd < 0)
    {
        perror("open infile");
        return 1;
    }
    // outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT, 0644);
    if (outfd < 0)
    {
        perror("open outfile");
        return 1;
    }
    if (get_file_size(infd, &insize))
        return 1;

    printf("insize: %ld ", insize);

    int cnt = 0;
    if (is_read == 1)
    {
        void *ptr;
        while (1)
        {
            /* code */

            int ret = posix_memalign((void **)&ptr, 256, BS);
            lseek(infd, 0, SEEK_SET);
            DTRACE_PROBE(yichuan, runcode_trace1_in);
            read(infd, ptr, BS);
            DTRACE_PROBE(yichuan, runcode_trace1_out);
            cnt += BS;
            if (cnt >= insize)
                break;
        }
        printf("cnt : %d ",cnt);
        free(ptr);
    }
    else
    {
        void *ptr;
        while (1)
        {
            /* code */

            int ret = posix_memalign((void **)&ptr, 256, BS);
            lseek(infd, 0, SEEK_SET);
            read(infd, ptr, BS);
            DTRACE_PROBE(yichuan, runcode_trace1_in);
            int len= write(outfd, ptr, BS);
            DTRACE_PROBE(yichuan, runcode_trace1_out);
            cnt += BS;
            if (cnt >= insize)
                break;
        }

        printf("cnt : %d ",cnt);

        free(ptr);
    }

    close(infd);
    close(outfd);
    return 0;
}

int main(int argc, char *argv[])
{
    printf("pid : %d ",getpid());

    if (argc < 3)
    {
        printf("%s args efficiency: file\n", argv[0]);
        return 1;
    }
    arg_iotype = argv[1];
    infile = argv[2];
    outfile = argv[3];
    if (strcmp(argv[4], "read") != 0)
        is_read = 0;
    FILE *file = fopen("pid_file", "w");
    printf("pid : %d ",getpid());
    fprintf(file, "%d", getpid());
    fclose(file);

    sleep(20);
    // int targetSize = 0; // size unit is byte
    // printf("please begin");
    // scanf("%s", targetSize);
    if (strcmp(arg_iotype, "uring") == 0)
    {
        test_iouring();
    }
    if (strcmp(arg_iotype, "sync") == 0)
    {
        test_sync();
    }
}