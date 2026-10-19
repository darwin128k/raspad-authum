#include <authum/error/code.h>
#include <authum/host.h>
#include <authum/module.h>
#include <authum/module/api.h>
#include <authum/version.h>
#include <lh/fs/path.h>
#include <lh/fs/path/style.h>
#include <lh/list.h>
#include <lh/null.h>
#include <lh/os.h>
#include <lh/os/error/code.h>
#include <lh/os/loader.h>
#include <lh/runtime/allocator.h>
#include <lh/str.h>
#include <lh/util/addr.h>
#include <lh/util/ptr.h>

#include <string.h>

typedef struct authum_child_data
{
    const authum_module_info_t *info;
    const authum_module_api_t *api;
    authum_host_t host;
    char *module_dir;
} authum_child_data_t;

const authum_module_info_t *
authum_child_peek_info(const void *data)
{
    if (lh_null_eq(data)) {
        return lh_null;
    }
    return lh_ptr_ccast(authum_child_data_t, data)->info;
}

static char *
authum_child_dup_cstr(const char *src)
{
    size_t n;
    char *dst;

    if (lh_null_eq(src)) {
        return lh_null;
    }
    n = strlen(src) + 1u;
    dst = lh_ptr_cast(char, lh_runtime_allocator_alloc(n));
    if (lh_null_eq(dst)) {
        return lh_null;
    }
    memcpy(dst, src, n);
    return dst;
}

static void
authum_child_fill_module_dir(authum_module_t *child, authum_child_data_t *data)
{
    const lh_fs_path_t *image;
    lh_fs_path_t parent;
    lh_str_t scratch;
    lh_str_ptr dir;

    data->module_dir = lh_null;
    data->host.module_dir = lh_null;

    image = lh_os_module_get_path_as_const(child);
    if (lh_fs_path_is_empty(image)) {
        return;
    }

    lh_fs_path_init(lh_addr_of(parent));
    if (!lh_fs_path_parent(lh_addr_of(parent), image)) {
        lh_fs_path_deinit(lh_addr_of(parent));
        return;
    }

    dir = lh_fs_path_to_cstr(lh_addr_of(parent), lh_fs_path_style_posix, lh_addr_of(scratch));
    if (!lh_null_eq(dir)) {
        data->module_dir = authum_child_dup_cstr(dir);
        data->host.module_dir = data->module_dir;
    }
    lh_str_deinit(lh_addr_of(scratch));
    lh_fs_path_deinit(lh_addr_of(parent));
}

static void
authum_child_fill_host(authum_module_t *child, authum_child_data_t *data)
{
    authum_module_t *parent;
    authum_host_t *out = &data->host;

    out->log = lh_null;
    out->engine_funcs = lh_null;
    out->globals = lh_null;
    out->module_dir = lh_null;

    parent = lh_os_module_get_parent(child);
    if (!lh_null_eq(parent)) {
        out->log = (authum_host_log_fn)lh_os_module_get_sym(parent, "authum_host_log");
        out->engine_funcs =
            (authum_host_get_ptr_fn)lh_os_module_get_sym(parent, "authum_engine_funcs");
        out->globals = (authum_host_get_ptr_fn)lh_os_module_get_sym(parent, "authum_globals");
    }

    authum_child_fill_module_dir(child, data);
}

static lh_bool_t
authum_child_start(authum_module_t *self)
{
    const authum_version_t required = AUTHUM_MODULE_ABI;
    authum_child_data_t *data;
    lh_ptr sym_info;
    lh_ptr sym_api;

    sym_info = lh_os_module_get_sym(self, AUTHUM_MODULE_INFO_SYM);
    sym_api = lh_os_module_get_sym(self, AUTHUM_MODULE_API_SYM);
    if (lh_null_eq(sym_info) || lh_null_eq(sym_api)) {
        lh_os_set_last_error_lit(authum_error_code_missing_info, "missing authum_module_info/api");
        return lh_bool_false;
    }

    data = lh_ptr_cast(authum_child_data_t, lh_runtime_allocator_alloc(sizeof(*data)));
    if (lh_null_eq(data)) {
        lh_os_set_last_error_lit(lh_os_error_code_out_of_memory, "child data");
        return lh_bool_false;
    }
    memset(data, 0, sizeof(*data));

    data->info = lh_ptr_ccast(authum_module_info_t, sym_info);
    data->api = lh_ptr_ccast(authum_module_api_t, sym_api);
    if (lh_null_eq(data->info->name) || lh_null_eq(data->api->start)) {
        lh_runtime_allocator_free(data);
        lh_os_set_last_error_lit(authum_error_code_invalid_info, "invalid module info/api");
        return lh_bool_false;
    }
    if (!authum_version_is_compatible(&data->info->abi, &required)) {
        lh_runtime_allocator_free(data);
        lh_os_set_last_error_lit(authum_error_code_incompatible_abi, "incompatible module abi");
        return lh_bool_false;
    }

    authum_child_fill_host(self, data);
    lh_os_module_set_data(self, data);

    if (!data->api->start(&data->host)) {
        lh_os_module_set_data(self, lh_null);
        if (!lh_null_eq(data->module_dir)) {
            lh_runtime_allocator_free(data->module_dir);
        }
        lh_runtime_allocator_free(data);
        return lh_bool_false;
    }
    return lh_bool_true;
}

static void
authum_child_stop(authum_module_t *self)
{
    authum_child_data_t *data;

    data = lh_ptr_cast(authum_child_data_t, lh_os_module_get_data(self));
    if (lh_null_eq(data)) {
        return;
    }
    if (!lh_null_eq(data->api) && !lh_null_eq(data->api->stop)) {
        data->api->stop();
    }
    lh_os_module_set_data(self, lh_null);
    if (!lh_null_eq(data->module_dir)) {
        lh_runtime_allocator_free(data->module_dir);
    }
    lh_runtime_allocator_free(data);
}

static const lh_os_module_ops_t authum_child_ops = {
    .start = authum_child_start,
    .stop = authum_child_stop,
};

lh_os_module_t *
authum_child_load(lh_os_loader_t *loader, const lh_fs_path_t *path)
{
    lh_os_module_t *child;
    const lh_os_module_ops_t *ops;

    child = lh_os_module_create();
    if (lh_null_eq(child)) {
        return lh_null;
    }

    lh_os_module_set_owner(child, loader);
    if (!lh_os_module_open(child, path)) {
        lh_os_module_destroy(child);
        return lh_null;
    }

    if (!lh_null_eq(lh_os_module_get_sym(child, AUTHUM_MODULE_API_SYM))) {
        lh_os_module_set_ops(child, &authum_child_ops);
    } else {
        ops = lh_ptr_cast(const lh_os_module_ops_t,
                          lh_os_module_get_sym(child, AUTHUM_MODULE_OPS_SYM));
        if (lh_null_eq(ops)) {
            lh_os_module_destroy(child);
            return lh_null;
        }
        lh_os_module_set_ops(child, ops);
    }

    if (!lh_os_module_start(child)) {
        lh_os_module_destroy(child);
        return lh_null;
    }

    lh_list_push_back(lh_os_loader_get_modules(loader), lh_os_module_get_node(child));
    return child;
}
