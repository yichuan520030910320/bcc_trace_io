/* SPDX-License-Identifier: MIT */
/*
 * Simple app that demonstrates how to setup an io_uring interface,
 * submit and complete IO against it, and then tear it down.
 *
 * gcc -Wall -O2 -D_GNU_SOURCE -o io_uring-test iouring.c -luring
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/sdt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <aio.h>
#include <libaio.h>
#include <pthread.h>
#include "liburing.h"

#define BufferSize 4096
#define BlockSize 512
#define BatchSize 32
#define IterationSize 1024

#define QD 128

char *infile;
char *outfile;

int test_iouring()
{
    struct io_uring ring;
    int i, fd, ret, pending, done;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct iovec *iovecs;
    struct stat sb;
    ssize_t fsize;
    off_t offset;
    void *buf;

    ret = io_uring_queue_init(QD, &ring, 0);
    if (ret < 0)
    {
        fprintf(stderr, "queue_init: %s\n", strerror(-ret));
        return 1;
    }


    //todo

    fd = open(infile, O_RDONLY | O_DIRECT);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    if (fstat(fd, &sb) < 0)
    {
        perror("fstat");
        return 1;
    }

    fsize = 0;
    iovecs = calloc(QD, sizeof(struct iovec));
    for (i = 0; i < QD; i++)
    {
        if (posix_memalign(&buf, 4096, 4096))
            return 1;
        iovecs[i].iov_base = buf;
        iovecs[i].iov_len = 4096;
        fsize += 4096;
    }

    offset = 0;
    i = 0;
    do
    {
        sqe = io_uring_get_sqe(&ring);
        if (!sqe)
            break;
        io_uring_prep_readv(sqe, fd, &iovecs[i], 1, offset);
        offset += iovecs[i].iov_len;
        i++;
        if (offset > sb.st_size)
            break;
    } while (1);

    ret = io_uring_submit(&ring);
    if (ret < 0)
    {
        fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
        return 1;
    }
    else if (ret != i)
    {
        fprintf(stderr, "io_uring_submit submitted less %d\n", ret);
        return 1;
    }


    done = 0;
    pending = ret;
    fsize = 0;
    for (i = 0; i < pending; i++)
    {
        ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0)
        {
            fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
            return 1;
        }

        done++;
        ret = 0;
        if (cqe->res != 4096 && cqe->res + fsize != sb.st_size)
        {
            fprintf(stderr, "ret=%d, wanted 4096\n", cqe->res);
            ret = 1;
        }
        fsize += cqe->res;
        io_uring_cqe_seen(&ring, cqe);
        if (ret)
            break;
    }

    printf("Submitted=%d, completed=%d, bytes=%lu\n", pending, done,
           (unsigned long)fsize);
    close(fd);
    io_uring_queue_exit(&ring);
    return 0;
}

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        printf("%s args efficiency: file\n", argv[0]);
        return 1;
    }
    infile = argv[1];
    outfile = argv[2];
    test_iouring();
}