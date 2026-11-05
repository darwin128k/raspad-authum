#include <authum/runtime.h>
#include <lh/core/char/map.h>
#include <lh/core/compiler/constructor.h>
#include <lh/core/runtime/allocator.h>
#include <lh/core/runtime/logger.h>
#include <lh/core/runtime/terminate.h>
#include <lh/core/str/format/text.h>
#include <lh/core/util/str/ptr.h>
#include <lh/os/rail/base.h>
#include <lh/rail/runtime/base.h>

#include <stdio.h>
#include <stdlib.h>

/* The C functions take no context, so they are wrapped rather than cast to the
   callback types: calling a function through a pointer of another type is
   undefined behaviour. */
static lh_ptr
authum_runtime_alloc(lh_self_ptr self, lh_usize_t size)
{
    (void)self;
    return malloc(size);
}

static lh_void
authum_runtime_dealloc(lh_self_ptr self, lh_ptr ptr)
{
    (void)self;
    free(ptr);
}

static lh_ptr
authum_runtime_realloc(lh_self_ptr self, lh_ptr ptr, lh_usize_t old_size, lh_usize_t new_size)
{
    (void)self;
    (void)old_size;
    return realloc(ptr, new_size);
}

static lh_void
authum_runtime_abort(lh_self_ptr self)
{
    (void)self;
    abort();
}

/* One record, one line on stderr; a record too long for the line is written as
   its format mask, so it still says something. */
static lh_ssize_t
authum_runtime_log_stderr(lh_self_ptr self, lh_logger_level_t level, lh_str_cptr fmt, va_list args)
{
    char line[1024];
    lh_usize_t n = lh_str_ptr_format_text_v(line, sizeof(line) - 1u, fmt, args);
    const char *text = line;
    (void)self;
    (void)level;
    if (n == 0u)
    {
        text = fmt;
        n = lh_str_ptr_len(fmt);
    }
    if (fwrite(text, 1u, n, stderr) != n || fputc(lh_char_map_lf, stderr) == EOF)
    {
        return -1;
    }
    return lh_cast_static(lh_ssize_t, n + 1u);
}

void
authum_runtime_use_stderr_logger(void)
{
    lh_logger_init(lh_runtime_logger_get_mut(), lh_logger_level_flags_all,
                   authum_runtime_log_stderr, lh_null);
}

void
authum_runtime_install(void)
{
    lh_memory_allocator_init(lh_runtime_allocator_get_mut(), authum_runtime_alloc,
                             authum_runtime_dealloc);
    lh_memory_allocator_set_realloc_cb(lh_runtime_allocator_get_mut(), authum_runtime_realloc);
    lh_runtime_terminate_handler_init(lh_runtime_terminate_get_mut(), authum_runtime_abort,
                                      lh_null);
    /* rail (threads, loops) runs on the OS here. */
    lh_rail_base_assign(lh_rail_runtime_base_get_mut(), lh_os_rail_base_get());
    authum_runtime_use_stderr_logger();
}

/* Every image that carries this object fills its own LH copy's slots as it
   loads, before any of its code runs. */
lh_compiler_constructor(authum_runtime_at_load)
{
    authum_runtime_install();
}
