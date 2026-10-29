#include <authum/host/events/internal.h>

static const authum_events_ops_t authum_events_host_ops_table = {
    .subscribe = authum_events_host_subscribe,
    .unsubscribe = authum_events_host_unsubscribe,
    .publish = authum_events_host_publish_as,
    .resolve = authum_events_host_resolve,
    .subscribe_id = authum_events_host_subscribe_id,
    .unsubscribe_id = authum_events_host_unsubscribe_id,
    .publish_id = authum_events_host_publish_id_as,
};

const authum_events_ops_t *
authum_events_host_ops(void)
{
    return lh_addr_of(authum_events_host_ops_table);
}
