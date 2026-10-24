#include "../os.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>

void
authum_os_ipv4_each(authum_os_ipv4_fn fn, authum_ptr user)
{
    struct ifaddrs *list;
    struct ifaddrs *it;

    if (getifaddrs(&list) != 0)
    {
        return;
    }
    for (it = list; it; it = it->ifa_next)
    {
        if (!it->ifa_addr || it->ifa_addr->sa_family != AF_INET)
        {
            continue;
        }
        fn((authum_u32_t)ntohl(((const struct sockaddr_in *)it->ifa_addr)->sin_addr.s_addr), user);
    }
    freeifaddrs(list);
}
