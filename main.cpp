#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/inotify.h>

using namespace std;

int main(int argc, char *argv[])
{

    if (argc < 2 || argc > 2)
    {
        printf("Usage: %s PATH\n", argv[0]);
        return -1;
    }

    int fd;
    int wd;

    fd = inotify_init();
    if (fd == -1)
    {
        printf("inotify_init failed\n");
        return -1;
    }

    wd = inotify_add_watch(fd, argv[1],
                           IN_MODIFY | IN_DELETE | IN_CREATE);
    if (wd == -1)
    {
        perror("inotify_add_watch");
        return -1;
    }

    printf("Watching %s\n", argv[1]);

    const struct inotify_event *event;
    char buffer[4096];
    ssize_t len;

    while (true)
    {
        len = read(fd, buffer, sizeof(buffer));
        if (len == -1)
        {
            printf("read failed\n");
            return -1;
        }

        if (len <= 0)
            break;

        for (char *ptr = buffer; ptr < buffer + len;)
        {
            event = (const struct inotify_event *)ptr;

            if (event->mask & IN_CREATE)
                printf("file created: %s\n", event->name);
            else if (event->mask & IN_MODIFY)
                printf("file modified: %s\n", event->name);
            else if (event->mask & IN_DELETE)
                printf("file deleted: %s\n", event->name);
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
    return 0;
}