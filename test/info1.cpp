/**
 * section: Information
 * synopsis: Extract information about Xen domain 0
 * purpose: Demonstrate the basic use of the library to connect to the
 *          hypervisor and extract domain information.
 * usage: info1
 * test: info1
 * copy: see Copyright for the status of this software.
 */

#include <iostream>
#include <algorithm>
#include <thread>
#include <stdio.h>
#include <chrono>
#include <libvirt/libvirt.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/nn.h>
#include <string.h>

static const std::vector<std::string>
getDomains(const virConnectPtr &conn)
{
    virDomainPtr *domains;
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_RUNNING | VIR_CONNECT_LIST_DOMAINS_PAUSED;
    size_t i;
    int ret;
    std::vector<std::string> str_domains;

    ret = virConnectListAllDomains(conn, &domains, flags);
    if (ret < 0)
        fprintf(stderr, "No domains found.");

    for (i = 0; i < ret; i++)
    {
        /* Get the information, guaranted to have a name */

        const char *name = virDomainGetName(domains[i]);
        str_domains.push_back(name);

        // here or in a separate loop if needed
        virDomainFree(domains[i]);
    }

    if (domains != NULL)
        free(domains);

    return str_domains;
}

template <typename T>
std::vector<T> vectorDifference(const std::vector<T> &v1, const std::vector<T> &v2)
{
    // make the result initially to v1, but only if its not empty
    std::vector<T> result = v1;

    // remove the elements in v2 from result
    for (const T &element : v2)
    {
        const auto it = std::find_if(result.begin(), result.end(), [element](const T &node)
                                     { return node == element; });

        if (it != result.end())
        {
            result.erase(it);
        }
    }
    return result;
}

/**
 * @brief This server function, serves as a guard, initializing and proxying the nn_send-function.
 *        This function blocks indefinitely.
 * 
 * @param url the url to bind to
 * @param fnc lambda, to send back to local lambda
 * @return int 0
 */
int server(const char *url, std::function<void(std::function<void(const char *msg, int length)>)> fnc)
{
    // Allways populate at the beginning, to avoid unnecessary
    // state change messages.
    int sock = 0;
    if ((sock = nn_socket(AF_SP, NN_PUB)) < 0)
    {
        fprintf(stderr, "nn_socket");
    }
    if (nn_bind(sock, url) < 0)
    {
        fprintf(stderr, "nn_bind");
    }

    for (;;)
    {
        // send messages, if any. We use a proxy to avoid refencing local/stack-variables and 
        // to avoid the heap.
        auto snd = [&sock](const char *msg, int length)
        {
            int bytes = nn_send(sock, msg, length, 0);
            if (bytes < 0)
            {
                fprintf(stderr, "nn_send");
            }
        };

        fnc(snd); // dumpster fires are fun.

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "syntax: %s: ipc-uri libvirt-uri", argv[0]);
        return 1;
    }

    virConnectPtr conn = virConnectOpenReadOnly(argv[2]);
    if (conn == NULL)
    {
        fprintf(stderr, "Failed to connect to hypervisor\n");
    }

    // We use lambdas to fix scoping issues and avoid having to work with far-memory.
    // We send a send-proxy alongside, to avoid copying/referencing memory outside
    // of the segment.
    std::vector<std::string> str_domains = getDomains(conn);
    server(argv[1], [&conn, &str_domains](std::function<void(const char *msg, int length)> send) -> void
           {
               std::vector<std::string> tmp = getDomains(conn);
               std::vector<std::string> diff;
               int on = 1;

               if (tmp.size() <= str_domains.size())
               {
                   // machines are leaving
                   diff = vectorDifference(str_domains, tmp);
                   on = 0;
               }
               else
               {
                   diff = vectorDifference(tmp, str_domains);
               }

               for (auto i : diff)
               {
                    char buffer[4096] = { 0 };
                    size_t sz_d = snprintf(buffer, 4096, "{ \"%s\": \"state\": %d }", i.c_str(), on);
                    send(buffer,sz_d );                   
               }

               str_domains = tmp;
           });

    if (conn != NULL)
        virConnectClose(conn);

    return 0;
}
