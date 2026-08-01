// Following:
// https://www.youtube.com/watch?v=2HrYIl6GpYg

// From root directory, use:
// wget localhost:8080/examples/00-minimal/index.html

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <unistd.h>

ssize_t get_file_size(const char *filepath)
{
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) return -1;
    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    const ssize_t size = ftell(fp);
    fclose(fp);
    return size;
}

int main(void)
{
    const int domain = AF_INET;   // IPv4
    const int type = SOCK_STREAM; // TCP socket
    const int protocol = 0;       // set to default

    const int listen_fd = socket(domain, type, protocol);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,   // IPv4
        .sin_port = htons(8080), // in network order
        .sin_addr = {0}          // IPv4 address: localhost
    };
    if (bind(listen_fd, (void *)&addr, sizeof(addr)) != 0) {
        perror("bind() failed: ");
        return 1;
    }

    const int backlog = 10;
    listen(listen_fd, backlog);

    const int client_fd = accept(listen_fd, NULL, NULL);

    char buffer[256] = {0};
    recv(client_fd, buffer, 256, 0);

    // GET /file.html ....

    char *f = buffer + sizeof("GET /") - 1;
    *strchr(f, ' ') = '\0';

    const int opened_fd = open(f, O_RDONLY);

    ssize_t count = get_file_size(f);
    if (count == -1) {
        printf("failed to send %s\n", f);
        send(client_fd, "\r\n", sizeof("\r\n"), 0);
    }
    else {
        sendfile(client_fd, opened_fd, NULL, (size_t)count);
    }

    close(opened_fd);
    close(client_fd);
    close(listen_fd);
}
