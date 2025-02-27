#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>

#define BUFLEN 256

#define _READ(b, l)             \
    rv = read(fd, b, l);                    \
    if (rv < 0) {                           \
        perror("read");                 \
        close(fd);          \
        exit(EXIT_FAILURE);     \
    }

#define _LSEEK(o, w)                \
    rv = lseek(fd, o, w);                   \
    if (rv < 0) {                           \
        perror("lseek");                \
        close(fd);          \
        exit(EXIT_FAILURE);     \
    }

int main()
{
    struct iovec iovs[3];
    ssize_t rv;
    char buf[BUFLEN];
    char arr1[] = "This seems ", arr2[] = "to have ", arr3[] = "worked";
    char *str = "This seems to have worked";

    iovs[0].iov_base = arr1;
    iovs[0].iov_len = strlen(arr1);

    iovs[1].iov_base = arr2;
    iovs[1].iov_len = strlen(arr2);

    iovs[2].iov_base = arr3;
    iovs[2].iov_len = strlen(arr3);

    int total_write_len = iovs[0].iov_len + iovs[1].iov_len + iovs[2].iov_len;
    assert(total_write_len == strlen(str));

    int fd = open("hello", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    _READ(buf, BUFLEN);

    if (rv == 0) {
        printf("Source file empty\n");
    } else {
        buf[rv] = '\0';
        printf("Source file content: \"%s\"\n", buf);
    }

    int startpos = 10;

    _LSEEK(startpos, SEEK_SET);
    printf("Writev start position: %d, length: %d\n", startpos, total_write_len);

    rv = writev(fd, iovs, 3);
    if (rv < 0) {
        printf("Write unsuccessful (writev returned %ld)\n", rv);
        exit(EXIT_FAILURE);
    }

    if (rv != total_write_len) {
        printf("Written bytes number %ld is not equal to expected %d", rv, total_write_len);
        exit(EXIT_FAILURE);
    }

    int endpos = lseek(fd, 0, SEEK_CUR);
    if (startpos + total_write_len != endpos)
    {
        printf("File offset at the end of writev is not correct: expected %d != actual %d\n",
            startpos + total_write_len, endpos);
        exit(EXIT_FAILURE);
    }

    _LSEEK(startpos, SEEK_SET);

    memset(buf, 0, BUFLEN);
    _READ(buf, total_write_len);

    if (rv != total_write_len) {
        printf("read fail: expecting %d bytes, rv: %ld \n", total_write_len, rv);
        exit(EXIT_FAILURE);
    }

    if (strncmp(str, buf, strlen(str))) {
        printf("write fail: string mismatch\n");
        buf[rv] = '\0';
        printf("Expected: \"%s\", actual: \"%s\"\n", str, buf);
        exit(EXIT_FAILURE);
    }

    rv = pwritev(fd, iovs, 3, -1);
    if ((rv != -1) || (errno != EINVAL)) {
        printf("pwritev with invalid offset returned %ld (errno %d)\n", rv, errno);
        exit(EXIT_FAILURE);
    }

    startpos += 10;
    rv = pwritev(fd, iovs, 3, startpos);
    if (rv != total_write_len) {
        printf("Bytes written with pwritev: %ld (expected %d)\n", rv, total_write_len);
        exit(EXIT_FAILURE);
    }
    rv = lseek(fd, 0, SEEK_CUR);
    if (rv != endpos) {
        printf("File offset at the end of pwritev: %ld (expected %d)\n", rv, endpos);
        exit(EXIT_FAILURE);
    }
    _LSEEK(startpos, SEEK_SET);
    _READ(buf, total_write_len);
    if (rv != total_write_len) {
        printf("read after pwritev fail: expecting %d bytes, rv: %ld \n", total_write_len, rv);
        exit(EXIT_FAILURE);
    }
    if (strncmp(str, buf, strlen(str))) {
        printf("pwritev fail: string mismatch\n");
        buf[rv] = '\0';
        printf("Expected: \"%s\", actual: \"%s\"\n", str, buf);
        exit(EXIT_FAILURE);
    }

    close(fd);

    fd = open("hello", O_RDONLY);
    if (fd < 0) {
        perror("open read-only");
        exit(EXIT_FAILURE);
    }
    if (writev(fd, iovs, 3) != -1) {
        printf("Could writev to read-only file\n");
        exit(EXIT_FAILURE);
    } else if (errno != EBADF) {
        perror("writev to read-only file: unexpected error");
        exit(EXIT_FAILURE);
    }
    if (close(fd) < 0) {
        perror("close read-only");
        exit(EXIT_FAILURE);
    }

    printf("write test passed\n");

    return 0;
}
