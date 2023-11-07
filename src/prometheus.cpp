/**
 * @file prometheus.cpp
 * @author Newsworthy39 <newsworthy39@github.com>
 * @brief Exports prometheus metrics from libvirt vms.
 * @version 0.2.5
 * @date Thu, 16 Sep 2023 15:32:37 +0000
 *
 * @copyright Copyright (c) 2023 Newsworthy39 <newsworthy39@github.com>
 *
 */

#include <iostream>
#include <algorithm>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <format.hpp>
#include <serializers.hpp>
#include <libvirt/libvirt.h>

#define MAX_EVENTS 10
#define BACKLOG 10

/**
 * @brief setnonblocking
 * sets a socket into nonblocking.
 *
 * @param sock
 * @return int
 */
int setnonblocking(int sock)
{
    int result;
    int flags;

    flags = ::fcntl(sock, F_GETFL, 0);

    if (flags == -1)
    {
        return -1; // error
    }

    flags |= O_NONBLOCK;

    result = fcntl(sock, F_SETFL, flags);
    return result;
}

/**
 * @brief setflags,
 * Sets different flags on a socket.
 *
 * @param sock
 */
void setflags(int sock)
{
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

/**
 * @brief stream_server
 * This is an event-based, stack-based stream-server. It basically,
 * just spews any data onto a callable, with a FD-on it.
 * @param port the port to listen on
 * @param fnc the callback
 * @return int
 */

int stream_server(int port, std::function<void(int)> fnc)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int sockfd, epollfd, n;
    struct sockaddr_in serv_addr; /* my address information */

    /* Code to set up listening socket, 'listen_sock',
       (socket(), bind(), listen()) omitted. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;         /* host byte order */
    serv_addr.sin_port = htons(port);       /* short, network byte order */
    serv_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
    bzero(&(serv_addr.sin_zero), 8);        /* zero the rest of the struct */

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    setflags(sockfd);

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    /* epoll*/
    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == sockfd)
            {
                socklen_t addrlen;
                struct sockaddr_in client_addr; /* my address information */

                int conn_sock = accept(events[n].data.fd,
                                       (struct sockaddr *)&client_addr, &addrlen);
                if (conn_sock == -1)
                {
                    fprintf(stderr, "accept: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                setnonblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,
                              &ev) == -1)
                {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                // We have data, to read -
                // send messages, if any. We use a proxy to avoid refencing local/stack-variables and
                // to avoid the heap.
                fnc(events[n].data.fd);
            }
        }
    }
}

/**
 * @brief Main entry point
 *
 * @param argc Number of arguments
 * @param argv the actual arguments
 * @return int
 */
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "syntax: %s: http-port libvirt-uri\n", argv[0]);
        return 1;
    }

    int port = std::stoi(std::string(argv[1]));

    printf("using port: %d\n", port);
    printf("using system: %s\n", argv[2]);

    virConnectPtr conn = virConnectOpenReadOnly(argv[2]);
    if (conn == NULL)
    {
        fprintf(stderr, "Failed to connect to %s\n", argv[2]);
        return (EXIT_FAILURE);
    }

    // Setup a stream_server, and use the lambda below
    // for data-processing.
    stream_server(port, [&conn](int fd) -> void {
        // read from socket, consume data.
        char buffer[4096] = {0};
        ssize_t bytes = recv(fd, buffer, 4096, 0);

        // Deal with different reading-scenarios.
        if (bytes > 0) {

            // Generate output
            std::string body;
            virDomainPtr *doms = { 0 };
            body.append("# prometheus data\n");

            // List, domains and take stats, but only if something is there.
            unsigned int flags = VIR_CONNECT_LIST_DOMAINS_RUNNING;
            size_t ret = virConnectListAllDomains(conn, &doms, flags);
            if (ret <= 0) {
                std::string output = custom::generate_prometheus(body);
                send(fd, output.c_str(), output.length(), 0);
                return;
            }

            // cpu-stats.
            virDomainStatsRecordPtr *statscpu;
            size_t rc = virDomainListGetStats(doms,
                                            virDomainStatsTypes::VIR_DOMAIN_STATS_VCPU,
                                            &statscpu, VIR_CONNECT_GET_ALL_DOMAINS_STATS_NOWAIT);
                                            
            if (rc > 0)
            {
                serializer::vcpu_metrics(body, rc, statscpu);
            }
            // Free structures
            virDomainStatsRecordListFree(statscpu);             

            // Network stats
            virDomainStatsRecordPtr *statsnet;
            size_t nrc = virDomainListGetStats(doms,
                                            virDomainStatsTypes::VIR_DOMAIN_STATS_INTERFACE,
                                            &statsnet, VIR_CONNECT_GET_ALL_DOMAINS_STATS_NOWAIT);
            if (nrc > 0)
            {
                serializer::network_metrics(body, nrc, statsnet);                
            }
            // Free structures
            virDomainStatsRecordListFree(statsnet);

             // block stats
            virDomainStatsRecordPtr *statsblocks;
            size_t nrb = virDomainListGetStats(doms,
                                            virDomainStatsTypes::VIR_DOMAIN_STATS_BLOCK,
                                            &statsblocks, VIR_CONNECT_GET_ALL_DOMAINS_STATS_NOWAIT);
            if (nrb > 0)
            {
                serializer::block_metrics(body, nrc, statsblocks);                
            }
            // Free structures
            virDomainStatsRecordListFree(statsblocks);

            // allways free here.            
            free(doms);

            std::string output = custom::generate_prometheus(body);
            send(fd, output.c_str(), output.length(), 0);
        }

        if (bytes <= 0)
            close(fd); 
    });

    if (conn != NULL)
        virConnectClose(conn);

    return 0;
}