#include <authum/net.h>
#include <string.h>

authum_bool_t
authum_net_address_host(const char *address, char *out, authum_usize_t capacity)
{
    const char *begin = address, *end, *port = NULL, *p;
    unsigned colons = 0, number = 0;
    size_t length;
    if (!out || !capacity)
    {
        return authum_bool_false;
    }

    if (!address || !address[0])
    {
        goto invalid;
    }
    end = address + strlen(address);
    if (*begin == '[')
    {
        ++begin;
        end = strchr(begin, ']');
        if (!end || end == begin || (end[1] && end[1] != ':'))
        {
            goto invalid;
        }
        if (end[1])
        {
            port = end + 2;
        }
    }
    else
    {
        for (p = begin; p < end; ++p)
        {
            if (*p == ':')
            {
                ++colons;
            }
        }
        if (colons == 1)
        {
            end = strchr(begin, ':');
            port = end + 1;
        }
    }
    if (port)
    {
        if (!*port)
        {
            goto invalid;
        }
        for (p = port; *p; ++p)
        {
            if (*p < '0' || *p > '9')
            {
                goto invalid;
            }
            number = number * 10 + (unsigned)(*p - '0');
            if (number > 65535)
            {
                goto invalid;
            }
        }
    }
    for (p = begin; p < end; ++p)
    {
        if ((unsigned char)*p <= ' ' || *p == '[' || *p == ']')
        {
            goto invalid;
        }
    }
    length = (size_t)(end - begin);
    if (!length || length >= capacity)
    {
        goto invalid;
    }
    memmove(out, begin, length);
    out[length] = '\0';
    return authum_bool_true;
invalid:
    out[0] = '\0';
    return authum_bool_false;
}
