#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#define SERVER "server"
#define CLIENT "client"

int
client(const char *url, const char *name)
{
        int sock;

        if ((sock = nn_socket(AF_SP, NN_SUB)) < 0) {
                 fprintf(stderr,"nn_socket");
        }

        // subscribe to everything ("" means all topics)
        if (nn_setsockopt(sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) < 0) {
                 fprintf(stderr,"nn_setsockopt");
        }
        if (nn_connect(sock, url) < 0) {
                fprintf(stderr, "nn_connet");
        }
        for (;;) {
                char *buf = NULL;
                int bytes = nn_recv(sock, &buf, NN_MSG, 0);
                if (bytes < 0) {
                         fprintf(stderr,"nn_recv");
                }
                printf("CLIENT (%s): RECEIVED %s\n", name, buf); 
                nn_freemsg(buf);
        }
}

int
main(const int argc, const char **argv)
{
          if ((argc >= 3) && (strcmp(CLIENT, argv[1]) == 0))
                return (client (argv[2], argv[3]));

        fprintf(stderr, "Usage: pubsub %s|%s <URL> <ARG> ...\n",
            SERVER, CLIENT);
        return 1;
}