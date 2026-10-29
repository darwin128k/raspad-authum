#include <authum/host/events/internal.h>

static const authum_events_ops_t authum_events_host_ops_table = {
    sizeof(authum_events_ops_t),       authum_events_host_subscribe,
    authum_events_host_unsubscribe,    authum_events_host_publish_as,
    authum_events_host_resolve,        authum_events_host_subscribe_id,
    authum_events_host_unsubscribe_id, authum_events_host_publish_id_as};

const authum_events_ops_t *
authum_events_host_ops(void)
{
    return lh_addr_of(authum_events_host_ops_table);
}
