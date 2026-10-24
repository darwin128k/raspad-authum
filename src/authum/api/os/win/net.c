#include "../os.h"

#include <winsock2.h>
#include <iphlpapi.h>
#include <stdlib.h>

enum
{
    authum_os_ipv4_tries = 3
};

void
authum_os_ipv4_each(authum_os_ipv4_fn fn, authum_ptr user)
{
    const ULONG flags =
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    IP_ADAPTER_ADDRESSES *list = authum_null;
    IP_ADAPTER_ADDRESSES *it;
    ULONG size = 16u * 1024u;
    ULONG result = ERROR_BUFFER_OVERFLOW;
    int i;

    /* The adapter list can grow between the size query and the call: retry. */
    for (i = 0; i < authum_os_ipv4_tries && result == ERROR_BUFFER_OVERFLOW; i++)
    {
        free(list);
        list = (IP_ADAPTER_ADDRESSES *)malloc(size);
        if (!list)
        {
            return;
        }
        result = GetAdaptersAddresses(AF_INET, flags, NULL, list, &size);
    }
    for (it = (result == NO_ERROR) ? list : authum_null; it; it = it->Next)
    {
        IP_ADAPTER_UNICAST_ADDRESS *address;

        for (address = it->FirstUnicastAddress; address; address = address->Next)
        {
            const struct sockaddr_in *in =
                (const struct sockaddr_in *)address->Address.lpSockaddr;
            const unsigned char *b;

            if (!in || in->sin_family != AF_INET)
            {
                continue;
            }
            b = (const unsigned char *)&in->sin_addr;
            fn(((authum_u32_t)b[0] << 24) | ((authum_u32_t)b[1] << 16) |
                   ((authum_u32_t)b[2] << 8) | (authum_u32_t)b[3],
               user);
        }
    }
    free(list);
}
