#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <poll.h>
#include <stdlib.h>

using namespace std;

static void
handle_events(int fd, int *wd, int argc, char *argv[])
{
    const struct inotify_event *event;
    char buffer[4096];
    ssize_t len;

    for (;;)
    {
        len = read(fd, buffer, sizeof(buffer));
        if (len == -1 && errno != EAGAIN)
        {
            printf("read failed\n");
            exit(EXIT_FAILURE);
            return;
        }

        if (len <= 0)
            break;

        for (char *ptr = buffer; ptr < buffer + len;)
        {
            event = (const struct inotify_event *)ptr;

            if (event->mask & IN_CREATE)
                printf("file created: ");
            else if (event->mask & IN_MODIFY)
                printf("file modified: ");
            else if (event->mask & IN_DELETE)
                printf("file deleted: ");
            ptr += sizeof(struct inotify_event) + event->len;
        }
        /* Print the name of the watched directory. */

        for (size_t i = 1; i < argc; ++i)
        {
            if (wd[i] == event->wd)
            {
                printf("%s/", argv[i]);
                break;
            }
        }

        /* Print the name of the file. */

        if (event->len)
            printf("%s", event->name);

        /* Print type of filesystem object. */

        if (event->mask & IN_ISDIR)
            printf(" [directory]\n");
        else
            printf(" [file]\n");
    }
}

int main(int argc, char *argv[])
{
    char buf;
    int fd, i, poll_num;
    int *wd;
    struct pollfd fds[2];
    nfds_t nfds;

    if (argc < 2)
    {
        printf("Usage: %s PATH\n", argv[0]);
        return -1;
    }

    wd = (int *)malloc(sizeof(int) * argc);
    if (wd == NULL)
    {
        perror("malloc failed");
        return -1;
    }

    fd = inotify_init();
    if (fd == -1)
    {
        printf("inotify_init failed\n");
        return -1;
    }

    printf("Press Enter to exit\n");
    printf("Watching ");
    for (int i = 1; i < argc; i++)
    {
        wd[i] = inotify_add_watch(fd, argv[i],
                                  IN_MODIFY | IN_DELETE | IN_CREATE);
        if (wd[i] == -1)
        {
            fprintf(stderr, "Cannot watch %s\n", argv[i]);
            return -1;
        }
        printf("%s ", argv[i]);
    }
    printf("\n");


    nfds = 2;
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    printf("Listening for events...\n");
    while (true)
    {
        poll_num = poll(fds, nfds, -1);
        if (poll_num == -1)
        {
            if (errno == EINTR)
                continue;
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        if (poll_num > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                handle_events(fd, wd, argc, argv);
            }
            
            if(fds[1].revents & POLLIN)
            {
                while( read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n')
                    continue;
                break;
            }
        }
    }
    printf("Exiting...\n");

    close(fd);
    free(wd);
    exit(EXIT_SUCCESS);
    
    return 0;
}