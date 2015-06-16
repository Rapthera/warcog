#include <stdlib.h>
#include <stdio.h>

#include "../main.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

static int sock;

int sendtoaddr(const void *data, int len, const addr_t addr)
{
    /*static int i;

    if (++i == 10)
        return (i = 0);*/

    return sendto(sock, data, len, 0, (struct sockaddr*)&addr, sizeof(addr));
}

static int udp_init(uint16_t port)
{
    int sock;

    addr_t addr = {
        .family = AF_INET,
        .port = htons(port),
    };

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock < 0) {
        return sock;
    }

    if(bind(sock, (struct sockaddr*)&addr, sizeof(addr))) {
        printf("bind failed\n");
        close(sock);
        return -1;
    }

    return sock;
}

static int timer_init(void)
{
    static const struct itimerspec itimer = {
        .it_interval = {.tv_nsec = 1000000000 / FPS},
        .it_value = {.tv_nsec = 1000000000 / FPS},
    };

    int fd;

    fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if(fd < 0) {
        return fd;
    }

    timerfd_settime(fd, 0, &itimer, NULL);
    return fd;
}

static int epoll_init(int sock, int tfd)
{
    int fd;
    struct epoll_event ev;

    fd = epoll_create(1);
    if(fd < 0) {
        return fd;
    }

    ev.events = EPOLLIN;// | EPOLLET; TODO: nonblocking+EPOLLET
    ev.data.fd = 0;
    if(epoll_ctl(fd, EPOLL_CTL_ADD, sock, &ev) < 0) {
        close(fd);
        return -1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = -1;
    if(epoll_ctl(fd, EPOLL_CTL_ADD, tfd, &ev) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

int main(void)
{
    int efd, tfd, len;
    struct epoll_event ev;

    addr_t addr;
    socklen_t addrlen;
    uint8_t buf[65536];

    sock = udp_init(1337);
    if (sock < 0)
        return 1;

    tfd = timer_init();
    if( tfd < 0)
        goto EXIT_CLOSE_SOCK;

    efd = epoll_init(sock, tfd);
    if (efd < 0)
        goto EXIT_CLOSE_TFD;

    if (!on_init())
        goto EXIT_ALL;


    addrlen = sizeof(addr);

    do {
        len = epoll_wait(efd, &ev, 1, -1);
        if(len < 0) {
            printf("epoll error %u\n", errno);
            continue;
        }

        if(ev.data.fd < 0) { /* timer event */
            uint8_t tmp[8];
            len = read(tfd, tmp, 8);

            on_frame();
            continue;
        }

        len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &addrlen);
        if(len < 0) {
            printf("recvfrom error %u\n", errno);
            continue;
        }

        on_recv(buf, len, addr);
    } while(1);

EXIT_ALL:
    close(efd);
EXIT_CLOSE_TFD:
    close(tfd);
EXIT_CLOSE_SOCK:
    close(sock);
    return 1;
}
