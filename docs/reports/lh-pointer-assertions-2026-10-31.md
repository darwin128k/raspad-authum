# LH pointer assertion audit — 2026-10-31

Scope: pointer assertion duplication throughout LH, not a general correctness audit.
Scanned 356 C sources under `lib/lh/src` and searched 845 public headers under `lib/lh/include`; 282 of these files contain assertion calls. Tests, examples, benchmarks, third-party dependencies and generated build trees are not production-library candidates.

## Inventory

Existing checker: **948 call-site candidates**, in **85 files**, from **316 caller functions**. These are not 948 independently removable assertions. Several calls can share one caller assertion.

| Subsystem | Call-site candidates |
| --- | ---: |
| ui | 576 |
| os | 129 |
| net | 57 |
| crypto | 53 |
| math | 44 |
| sql | 21 |
| timer | 17 |
| str | 16 |
| logger | 10 |
| spsc | 9 |
| numeric | 8 |
| io | 6 |
| memory | 2 |

## Reviewed patterns

- `memory/view.c`: `get_data` delegates to `get_begin`; no extra entry assertion is needed. `get_end` calls both `get_begin` and `get_size`; the accessor checks stay because each accessor is independently callable. This is not a reason to bypass accessors.
- `memory/allocator.c`: `set_alloc_cb` and `set_dealloc_cb` write fields before calling `set_realloc_cb`. Their entry checks must stay in the current implementation. The old checker misses these because its definition recognizer expects the opening brace immediately after the line containing the function name; `set_realloc_cb` has a multiline signature. Its separate variable-name heuristic also lacks a parameter-position model.
- `ui/view/pointer.c`: `reset`, `init`, `start`, `grab` and `is_active` have an entry `self` assertion and immediately delegate to a checked public setter, reset or getter. Their entry `self` assertions are redundant; keep the checks in the public callees. `copy_*` checks `out` separately: checking `self` does not validate `out`.
- `str/list.c`: `init`, `clear` and `is_empty` already delegate through checked accessors without entry `self` assertions. `get_span` writes through `out`, so its explicit `out` assertion protects a separate contract.
- `runtime/assert.h`: assertions are debug-only by default, but `LH_RUNTIME_ASSERT_ENABLED` can keep them in Release. `lh_runtime_check_*` is always-on failure handling and is outside this deletion pass.

- `net/socket/addr.c`: `init_ip4` can delegate the `self` check to `set_family`, but `addr` is directly dereferenced and needs its own check. In `format`, the `str` check cannot simply move into the IPv4 formatter: the default branch returns without calling that formatter. In `get_ip4`, the family assertion is stronger than non-null and must remain.
- `math/rect.c`: `set_origin` and `set_size` check `self` before calling their checked mutable accessors; these entry checks duplicate accessor protection. `init_origin_size` in turn delegates to these setters.

## Limitations and required review

The existing checker resolves only same-file names and compares variable spelling, not parameter positions. It does not establish order, branches, state changes, aliases, callbacks, macro expansion or assertion dominance. Headers were searched, not preprocessed for every backend. No claim is made that every candidate has been manually proved safe to change.

A supplemental source-wide first-parameter lookup found **50 distinct cross-file caller/argument/callee tuples** with explicit assertions on both sides. It handles differing first-parameter names but remains textual, ignores calls in other argument positions and may join mutually exclusive backend definitions. These tuples are not added to the old 948 as a proven defect count.

## Same-file inventory

| File | Candidates |
| --- | ---: |
| `lib/lh/src/lh/os/window.c` | 81 |
| `lib/lh/src/lh/ui/input.c` | 63 |
| `lib/lh/src/lh/ui/canvas.c` | 57 |
| `lib/lh/src/lh/net/modbus/link.c` | 54 |
| `lib/lh/src/lh/crypto/sha.c` | 49 |
| `lib/lh/src/lh/ui/view.c` | 48 |
| `lib/lh/src/lh/ui/entity.c` | 43 |
| `lib/lh/src/lh/ui/frame/stats.c` | 29 |
| `lib/lh/src/lh/ui/rects.c` | 23 |
| `lib/lh/src/lh/ui/blur/window.c` | 22 |
| `lib/lh/src/lh/ui/label.c` | 19 |
| `lib/lh/src/lh/timer.c` | 17 |
| `lib/lh/src/lh/ui/style.c` | 17 |
| `lib/lh/src/lh/ui/button.c` | 16 |
| `lib/lh/src/lh/ui/transition.c` | 15 |
| `lib/lh/src/lh/sql/mysql/api.c` | 13 |
| `lib/lh/src/lh/ui/toggle.c` | 13 |
| `lib/lh/src/lh/ui/view/pointer.c` | 13 |
| `lib/lh/src/lh/ui/gradient.c` | 12 |
| `lib/lh/src/lh/ui/layout.c` | 12 |
| `lib/lh/src/lh/ui/radii.c` | 12 |
| `lib/lh/src/lh/ui/gradient/sampler/lerp.c` | 12 |
| `lib/lh/src/lh/math/frect.c` | 11 |
| `lib/lh/src/lh/math/rect.c` | 11 |
| `lib/lh/src/lh/ui/scrollbar.c` | 11 |
| `lib/lh/src/lh/logger.c` | 10 |
| `lib/lh/src/lh/ui/insets.c` | 10 |
| `lib/lh/src/lh/ui/font/range.c` | 10 |
| `lib/lh/src/lh/spsc.c` | 9 |
| `lib/lh/src/lh/os/wake.c` | 9 |
| `lib/lh/src/lh/ui/mask.c` | 9 |
| `lib/lh/src/lh/numeric/big.c` | 8 |
| `lib/lh/src/lh/os/input/queue.c` | 8 |
| `lib/lh/src/lh/sql/driver.c` | 8 |
| `lib/lh/src/lh/str/scanf/spec.c` | 8 |
| `lib/lh/src/lh/ui/font.c` | 8 |
| `lib/lh/src/lh/ui/shadow.c` | 8 |
| `lib/lh/src/lh/ui/surface.c` | 8 |
| `lib/lh/src/lh/os/fs/file.c` | 7 |
| `lib/lh/src/lh/ui/color.c` | 7 |
| `lib/lh/src/lh/ui/text/line.c` | 7 |
| `lib/lh/src/lh/os/app.c` | 6 |
| `lib/lh/src/lh/ui/image.c` | 6 |
| `lib/lh/src/lh/ui/key.c` | 6 |
| `lib/lh/src/lh/ui/pixmap.c` | 6 |
| `lib/lh/src/lh/ui/layout/place.c` | 6 |
| `lib/lh/src/lh/io/stream.c` | 5 |
| `lib/lh/src/lh/ui/container.c` | 5 |
| `lib/lh/src/lh/ui/paint.c` | 5 |
| `lib/lh/src/lh/ui/renderer3d.c` | 5 |
| `lib/lh/src/lh/ui/canvas/state.c` | 5 |
| `lib/lh/src/lh/crypto/rijndael.c` | 4 |
| `lib/lh/src/lh/math/quat.c` | 4 |
| `lib/lh/src/lh/math/vec4.c` | 4 |
| `lib/lh/src/lh/os/render/backend/gdi.c` | 4 |
| `lib/lh/src/lh/str/scanf/next.c` | 4 |
| `lib/lh/src/lh/ui/pen.c` | 4 |
| `lib/lh/src/lh/ui/renderer3d/camera.c` | 4 |
| `lib/lh/src/lh/ui/shadow/shape.c` | 4 |
| `lib/lh/src/lh/math/size.c` | 3 |
| `lib/lh/src/lh/math/vec3.c` | 3 |
| `lib/lh/src/lh/os/serial.c` | 3 |
| `lib/lh/src/lh/os/worker.c` | 3 |
| `lib/lh/src/lh/os/net/socket.c` | 3 |
| `lib/lh/src/lh/ui/gradient/sampler.c` | 3 |
| `lib/lh/src/lh/ui/shadow/ramp.c` | 3 |
| `lib/lh/src/lh/ui/text/block.c` | 3 |
| `lib/lh/src/lh/math/fpoint.c` | 2 |
| `lib/lh/src/lh/math/fsize.c` | 2 |
| `lib/lh/src/lh/math/point.c` | 2 |
| `lib/lh/src/lh/math/vec2.c` | 2 |
| `lib/lh/src/lh/memory/std.c` | 2 |
| `lib/lh/src/lh/net/ip.c` | 2 |
| `lib/lh/src/lh/os/fs/dir.c` | 2 |
| `lib/lh/src/lh/os/input/event.c` | 2 |
| `lib/lh/src/lh/str/parse/uint.c` | 2 |
| `lib/lh/src/lh/ui/canvas/sw.c` | 2 |
| `lib/lh/src/lh/ui/entity/event.c` | 2 |
| `lib/lh/src/lh/ui/entity/transform.c` | 2 |
| `lib/lh/src/lh/io/dgram.c` | 1 |
| `lib/lh/src/lh/net/socket/addr.c` | 1 |
| `lib/lh/src/lh/os/input/contacts.c` | 1 |
| `lib/lh/src/lh/str/format/text.c` | 1 |
| `lib/lh/src/lh/str/parse/hex.c` | 1 |
| `lib/lh/src/lh/ui/canvas/clip/round.c` | 1 |

## Cross-file candidates

| Caller | Argument | Callee |
| --- | --- | --- |
| `lib/lh/src/lh/timestamp.c::lh_timestamp_from_datetime` | `self` | `lib/lh/src/lh/datetime.c::lh_datetime_get_date` |
| `lib/lh/src/lh/timestamp.c::lh_timestamp_from_datetime` | `self` | `lib/lh/src/lh/datetime.c::lh_datetime_get_time` |
| `lib/lh/src/lh/net/ip.c::lh_net_ip4_format` | `str` | `lib/lh/src/lh/memory/std.c::lh_memory_std_copy` |
| `lib/lh/src/lh/os/app.c::lh_os_app_get_next_window` | `window` | `lib/lh/src/lh/os/window.c::lh_os_window_get_link` |
| `lib/lh/src/lh/ui/layout.c::lh_ui_layout_apply` | `parent` | `lib/lh/src/lh/ui/entity.c::lh_ui_entity_get_padding` |
| `lib/lh/src/lh/ui/layout.c::lh_ui_layout_apply` | `parent` | `lib/lh/src/lh/ui/entity.c::lh_ui_entity_copy_rect` |
| `lib/lh/src/lh/ui/layout.c::lh_ui_layout_apply` | `parent` | `lib/lh/src/lh/ui/entity.c::lh_ui_entity_get_first_child` |
| `lib/lh/src/lh/ui/style.c::lh_ui_style_set_radii_v` | `radii` | `lib/lh/src/lh/ui/radii.c::lh_ui_radii_get_min` |
| `lib/lh/src/lh/ui/style.c::lh_ui_style_set_padding_v` | `padding` | `lib/lh/src/lh/ui/insets.c::lh_ui_insets_get_top` |
| `lib/lh/src/lh/ui/style.c::lh_ui_style_set_padding_v` | `padding` | `lib/lh/src/lh/ui/insets.c::lh_ui_insets_get_right` |
| `lib/lh/src/lh/ui/style.c::lh_ui_style_set_padding_v` | `padding` | `lib/lh/src/lh/ui/insets.c::lh_ui_insets_get_bottom` |
| `lib/lh/src/lh/ui/view.c::lh_ui_view_start_pointer` | `pointer` | `lib/lh/src/lh/ui/view/pointer.c::lh_ui_view_pointer_start` |
| `lib/lh/src/lh/ui/view.c::lh_ui_view_start_transition` | `entity` | `lib/lh/src/lh/ui/entity.c::lh_ui_entity_get_style` |
| `lib/lh/src/lh/ui/view.c::lh_ui_view_start_transition` | `entity` | `lib/lh/src/lh/ui/entity.c::lh_ui_entity_set_style_at_once` |
| `lib/lh/src/lh/ui/view.c::lh_ui_view_input` | `event` | `lib/lh/src/lh/os/input/event.c::lh_os_input_event_get_key` |
| `lib/lh/src/lh/ui/view.c::lh_ui_view_input` | `event` | `lib/lh/src/lh/os/input/event.c::lh_os_input_event_get_kind` |
| `lib/lh/src/lh/ui/view.c::lh_ui_view_input` | `event` | `lib/lh/src/lh/os/input/event.c::lh_os_input_event_get_code` |
| `lib/lh/src/lh/ui/blur/window.c::lh_ui_blur_window_take` | `pixel` | `lib/lh/src/lh/ui/color.c::lh_ui_color_get_a` |
| `lib/lh/src/lh/ui/blur/window.c::lh_ui_blur_window_take` | `pixel` | `lib/lh/src/lh/ui/color.c::lh_ui_color_get_r` |
| `lib/lh/src/lh/ui/blur/window.c::lh_ui_blur_window_take` | `pixel` | `lib/lh/src/lh/ui/color.c::lh_ui_color_get_g` |
| `lib/lh/src/lh/ui/blur/window.c::lh_ui_blur_window_take` | `pixel` | `lib/lh/src/lh/ui/color.c::lh_ui_color_get_b` |
| `lib/lh/src/lh/ui/blur/window.c::lh_ui_blur_window_drop` | `pixel` | `lib/lh/src/lh/ui/color.c::lh_ui_color_get_a` |
| `lib/lh/src/lh/ui/blur/window.c::lh_ui_blur_window_drop` | `pixel` | `lib/lh/src/lh/ui/color.c::lh_ui_color_get_r` |
| `lib/lh/src/lh/ui/blur/window.c::lh_ui_blur_window_drop` | `pixel` | `lib/lh/src/lh/ui/color.c::lh_ui_color_get_g` |
| `lib/lh/src/lh/ui/blur/window.c::lh_ui_blur_window_drop` | `pixel` | `lib/lh/src/lh/ui/color.c::lh_ui_color_get_b` |
| `lib/lh/src/lh/ui/canvas/sw.c::lh_ui_canvas_sw_use_pixmap` | `pixmap` | `lib/lh/src/lh/ui/pixmap.c::lh_ui_pixmap_get_bounds` |
| `lib/lh/src/lh/ui/frame/stats.c::lh_ui_frame_stats_record` | `frame` | `lib/lh/src/lh/ui/frame.c::lh_ui_frame_get_us` |
| `lib/lh/src/lh/ui/frame/stats.c::lh_ui_frame_stats_record` | `frame` | `lib/lh/src/lh/ui/frame.c::lh_ui_frame_is_asked_known` |
| `lib/lh/src/lh/ui/frame/stats.c::lh_ui_frame_stats_record` | `frame` | `lib/lh/src/lh/ui/frame.c::lh_ui_frame_get_asked_px` |
| `lib/lh/src/lh/ui/frame/stats.c::lh_ui_frame_stats_record` | `frame` | `lib/lh/src/lh/ui/frame.c::lh_ui_frame_get_rects` |
| `lib/lh/src/lh/ui/shadow/ramp.c::lh_ui_shadow_ramp_init` | `shape` | `lib/lh/src/lh/ui/shadow/shape.c::lh_ui_shadow_shape_get_peak` |
| `lib/lh/src/lh/ui/shadow/ramp.c::lh_ui_shadow_ramp_init` | `shape` | `lib/lh/src/lh/ui/shadow/shape.c::lh_ui_shadow_shape_get_start` |
| `lib/lh/src/lh/ui/shadow/ramp.c::lh_ui_shadow_ramp_init` | `shape` | `lib/lh/src/lh/ui/shadow/shape.c::lh_ui_shadow_shape_get_step` |
| `lib/lh/src/lh/ui/shadow/ramp.c::lh_ui_shadow_ramp_init` | `shape` | `lib/lh/src/lh/ui/shadow/shape.c::lh_ui_shadow_shape_get_spread` |
| `lib/lh/src/lh/ui/shadow/ramp.c::lh_ui_shadow_ramp_init` | `shape` | `lib/lh/src/lh/ui/shadow/shape.c::lh_ui_shadow_shape_get_r` |
| `lib/lh/src/lh/str/format/put.c::lh_str_ptr_format_put` | `str` | `lib/lh/src/lh/memory/std.c::lh_memory_std_copy` |
| `lib/lh/src/lh/str/format/real.c::lh_str_ptr_format_real` | `str` | `lib/lh/src/lh/str/format/put.c::lh_str_ptr_format_put` |
| `lib/lh/src/lh/str/format/uint.c::lh_str_ptr_format_uint` | `str` | `lib/lh/src/lh/memory/std.c::lh_memory_std_copy_rev` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next_literal` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_kind` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next_literal` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_literal` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next_literal` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_literal_size` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next_flags` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_zero_pad` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next_flags` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_left_justify` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_literal` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_conversion` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_width` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_have_precision` |
| `lib/lh/src/lh/str/scanf/next.c::lh_str_ptr_scanf_next` | `out` | `lib/lh/src/lh/str/scanf/spec.c::lh_str_scanf_spec_set_precision` |
| `lib/lh/src/lh/os/system/str.c::lh_os_system_str_ptr_to_utf8` | `text` | `lib/lh/src/lh/os/system/posix/str.c::lh_os_system_str_to_utf8` |
| `lib/lh/src/lh/os/system/str.c::lh_os_system_str_ptr_to_utf8` | `text` | `lib/lh/src/lh/os/system/win/str.c::lh_os_system_str_to_utf8` |

## Same-file call-site candidates

```text
lib/lh\src\lh\logger.c: lh_logger_get_emit_cb checks self and calls lh_logger_get_emergency_cb(self), which checks it again
lib/lh\src\lh\logger.c: lh_logger_get_emit_cb checks self and calls lh_logger_get_alert_cb(self), which checks it again
lib/lh\src\lh\logger.c: lh_logger_get_emit_cb checks self and calls lh_logger_get_critical_cb(self), which checks it again
lib/lh\src\lh\logger.c: lh_logger_get_emit_cb checks self and calls lh_logger_get_error_cb(self), which checks it again
lib/lh\src\lh\logger.c: lh_logger_get_emit_cb checks self and calls lh_logger_get_warning_cb(self), which checks it again
lib/lh\src\lh\logger.c: lh_logger_get_emit_cb checks self and calls lh_logger_get_notice_cb(self), which checks it again
lib/lh\src\lh\logger.c: lh_logger_get_emit_cb checks self and calls lh_logger_get_info_cb(self), which checks it again
lib/lh\src\lh\logger.c: lh_logger_get_emit_cb checks self and calls lh_logger_get_debug_cb(self), which checks it again
lib/lh\src\lh\logger.c: lh_logger_log_v checks self and calls lh_logger_get_flags(self), which checks it again
lib/lh\src\lh\logger.c: lh_logger_log_v checks self and calls lh_logger_get_emit_cb(self), which checks it again
lib/lh\src\lh\spsc.c: lh_spsc_push checks self and calls lh_spsc_get_written(self), which checks it again
lib/lh\src\lh\spsc.c: lh_spsc_push checks self and calls lh_spsc_get_read(self), which checks it again
lib/lh\src\lh\spsc.c: lh_spsc_push checks self and calls lh_spsc_get_capacity(self), which checks it again
lib/lh\src\lh\spsc.c: lh_spsc_push checks self and calls lh_spsc_get_item_size(self), which checks it again
lib/lh\src\lh\spsc.c: lh_spsc_push checks self and calls lh_spsc_set_written(self), which checks it again
lib/lh\src\lh\spsc.c: lh_spsc_peek checks self and calls lh_spsc_get_read(self), which checks it again
lib/lh\src\lh\spsc.c: lh_spsc_peek checks self and calls lh_spsc_get_written(self), which checks it again
lib/lh\src\lh\spsc.c: lh_spsc_release checks self and calls lh_spsc_set_read(self), which checks it again
lib/lh\src\lh\spsc.c: lh_spsc_release checks self and calls lh_spsc_get_read(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_stop checks self and calls lh_timer_get_node_mut(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_stop checks self and calls lh_timer_set_paused(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_init checks self and calls lh_timer_set_period(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_init checks self and calls lh_timer_set_last(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_init checks self and calls lh_timer_set_priority(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_init checks self and calls lh_timer_set_repeat(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_init checks self and calls lh_timer_set_paused(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_init checks self and calls lh_timer_set_cb(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_init checks self and calls lh_timer_set_context(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_deinit checks self and calls lh_timer_stop(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_deinit checks self and calls lh_timer_set_cb(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_deinit checks self and calls lh_timer_set_context(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_group_set_priority checks self and calls lh_timer_get_node_mut(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_group_set_priority checks self and calls lh_timer_set_priority(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_pause checks self and calls lh_timer_set_paused(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_resume checks self and calls lh_timer_set_paused(self), which checks it again
lib/lh\src\lh\timer.c: lh_timer_resume checks self and calls lh_timer_set_last(self), which checks it again
lib/lh\src\lh\crypto\rijndael.c: lh_crypto_rijndael_encrypt_block checks self and calls lh_crypto_rijndael_get_chain(self), which checks it again
lib/lh\src\lh\crypto\rijndael.c: lh_crypto_rijndael_encrypt_block checks self and calls lh_crypto_rijndael_get_chain(self), which checks it again
lib/lh\src\lh\crypto\rijndael.c: lh_crypto_rijndael_decrypt_block checks self and calls lh_crypto_rijndael_get_chain(self), which checks it again
lib/lh\src\lh\crypto\rijndael.c: lh_crypto_rijndael_decrypt_block checks self and calls lh_crypto_rijndael_get_chain(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_kind(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_finished(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_bit_count_lo(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_bit_count_hi(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_block_fill(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_init checks self and calls lh_crypto_sha_set_state_word(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_update checks self and calls lh_crypto_sha_get_block_fill(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_update checks self and calls lh_crypto_sha_get_block_mut(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_update checks self and calls lh_crypto_sha_set_block_fill(self), which checks it again
lib/lh\src\lh\crypto\sha.c: lh_crypto_sha_finish checks self and calls lh_crypto_sha_set_finished(self), which checks it again
lib/lh\src\lh\io\dgram.c: lh_io_dgram_recv checks self and calls lh_io_dgram_get_reader(self), which checks it again
lib/lh\src\lh\io\stream.c: lh_io_stream_set checks self and calls lh_io_stream_set_reader(self), which checks it again
lib/lh\src\lh\io\stream.c: lh_io_stream_set checks self and calls lh_io_stream_set_writer(self), which checks it again
lib/lh\src\lh\io\stream.c: lh_io_stream_assign checks self and calls lh_io_stream_set(self), which checks it again
lib/lh\src\lh\io\stream.c: lh_io_stream_read checks self and calls lh_io_stream_get_reader(self), which checks it again
lib/lh\src\lh\io\stream.c: lh_io_stream_write checks self and calls lh_io_stream_get_writer(self), which checks it again
lib/lh\src\lh\math\fpoint.c: lh_math_fpoint_is_at_least checks self and calls lh_math_fpoint_get_x(self), which checks it again
lib/lh\src\lh\math\fpoint.c: lh_math_fpoint_is_at_least checks self and calls lh_math_fpoint_get_y(self), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_init_origin_size checks self and calls lh_math_frect_set_origin(self), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_init_origin_size checks self and calls lh_math_frect_set_size(self), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_far checks self and calls lh_math_frect_get_size(self), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_contains_point checks self and calls lh_math_frect_far(self), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_contains_point checks self and calls lh_math_frect_get_origin(self), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_is_at_least checks self and calls lh_math_frect_get_origin(self), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_intersection checks a and calls lh_math_frect_origin_max(a), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_intersection checks a and calls lh_math_frect_far_min(a), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_intersects checks a and calls lh_math_frect_intersection(a), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_union checks a and calls lh_math_frect_origin_min(a), which checks it again
lib/lh\src\lh\math\frect.c: lh_math_frect_union checks a and calls lh_math_frect_far_max(a), which checks it again
lib/lh\src\lh\math\fsize.c: lh_math_fsize_is_at_least checks self and calls lh_math_fsize_get_width(self), which checks it again
lib/lh\src\lh\math\fsize.c: lh_math_fsize_is_at_least checks self and calls lh_math_fsize_get_height(self), which checks it again
lib/lh\src\lh\math\point.c: lh_math_point_is_at_least checks self and calls lh_math_point_get_x(self), which checks it again
lib/lh\src\lh\math\point.c: lh_math_point_is_at_least checks self and calls lh_math_point_get_y(self), which checks it again
lib/lh\src\lh\math\quat.c: lh_math_quat_init checks self and calls lh_math_quat_set_x(self), which checks it again
lib/lh\src\lh\math\quat.c: lh_math_quat_init checks self and calls lh_math_quat_set_y(self), which checks it again
lib/lh\src\lh\math\quat.c: lh_math_quat_init checks self and calls lh_math_quat_set_z(self), which checks it again
lib/lh\src\lh\math\quat.c: lh_math_quat_init checks self and calls lh_math_quat_set_w(self), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_init_origin_size checks self and calls lh_math_rect_set_origin(self), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_init_origin_size checks self and calls lh_math_rect_set_size(self), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_far checks self and calls lh_math_rect_get_size(self), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_contains_point checks self and calls lh_math_rect_far(self), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_contains_point checks self and calls lh_math_rect_get_origin(self), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_is_at_least checks self and calls lh_math_rect_get_origin(self), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_intersection checks a and calls lh_math_rect_origin_max(a), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_intersection checks a and calls lh_math_rect_far_min(a), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_intersects checks a and calls lh_math_rect_intersection(a), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_union checks a and calls lh_math_rect_origin_min(a), which checks it again
lib/lh\src\lh\math\rect.c: lh_math_rect_union checks a and calls lh_math_rect_far_max(a), which checks it again
lib/lh\src\lh\math\size.c: lh_math_size_is_at_least checks self and calls lh_math_size_get_width(self), which checks it again
lib/lh\src\lh\math\size.c: lh_math_size_is_at_least checks self and calls lh_math_size_get_height(self), which checks it again
lib/lh\src\lh\math\size.c: lh_math_size_is_empty checks self and calls lh_math_size_get_height(self), which checks it again
lib/lh\src\lh\math\vec2.c: lh_math_vec2_init checks self and calls lh_math_vec2_set_x(self), which checks it again
lib/lh\src\lh\math\vec2.c: lh_math_vec2_init checks self and calls lh_math_vec2_set_y(self), which checks it again
lib/lh\src\lh\math\vec3.c: lh_math_vec3_init checks self and calls lh_math_vec3_set_x(self), which checks it again
lib/lh\src\lh\math\vec3.c: lh_math_vec3_init checks self and calls lh_math_vec3_set_y(self), which checks it again
lib/lh\src\lh\math\vec3.c: lh_math_vec3_init checks self and calls lh_math_vec3_set_z(self), which checks it again
lib/lh\src\lh\math\vec4.c: lh_math_vec4_init checks self and calls lh_math_vec4_set_x(self), which checks it again
lib/lh\src\lh\math\vec4.c: lh_math_vec4_init checks self and calls lh_math_vec4_set_y(self), which checks it again
lib/lh\src\lh\math\vec4.c: lh_math_vec4_init checks self and calls lh_math_vec4_set_z(self), which checks it again
lib/lh\src\lh\math\vec4.c: lh_math_vec4_init checks self and calls lh_math_vec4_set_w(self), which checks it again
lib/lh\src\lh\memory\std.c: lh_memory_std_move checks dst and calls lh_memory_std_rcopy(dst), which checks it again
lib/lh\src\lh\memory\std.c: lh_memory_std_move checks dst and calls lh_memory_std_copy(dst), which checks it again
lib/lh\src\lh\net\ip.c: lh_net_ip4_format checks self and calls lh_net_ip4_get_octet(self), which checks it again
lib/lh\src\lh\net\ip.c: lh_net_ip4_is_at_least checks self and calls lh_net_ip4_get_octet(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_set_stream checks self and calls lh_mb_link_set_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_set_stream checks self and calls lh_mb_link_set_state(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_set_stream checks self and calls lh_mb_link_set_reason(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_take checks self and calls lh_mb_link_get_got(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_take checks self and calls lh_mb_link_get_values(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_release checks self and calls lh_mb_link_set_state(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_release checks self and calls lh_mb_link_set_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_state(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_frame_mut(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_frame(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_asked_fc(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_framing(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_state(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_reason(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_now_us(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_now_us(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_clock(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_since_us(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_held(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_state(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_reason(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_state(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_unit(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_frame(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_framing(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_frame(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_asked_tx(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_got(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_address(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_expected(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_got(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_address(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_written(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_asked_fc(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_expected(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_values_mut(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_got(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_reason(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_state(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_now_us(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_clock(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_since_us(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_set_state(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_poll checks self and calls lh_mb_link_get_state(self), which checks it again
lib/lh\src\lh\net\modbus\link.c: lh_mb_link_write_one checks self and calls lh_mb_link_set_written(self), which checks it again
lib/lh\src\lh\net\socket\addr.c: lh_net_socket_addr_init_ip4 checks self and calls lh_net_socket_addr_set_family(self), which checks it again
lib/lh\src\lh\numeric\big.c: lh_numeric_big_set_u32 checks self and calls lh_numeric_big_set_word(self), which checks it again
lib/lh\src\lh\numeric\big.c: lh_numeric_big_set_u32 checks self and calls lh_numeric_big_set_word(self), which checks it again
lib/lh\src\lh\numeric\big.c: lh_numeric_big_mul_u32 checks self and calls lh_numeric_big_get_word(self), which checks it again
lib/lh\src\lh\numeric\big.c: lh_numeric_big_mul_u32 checks self and calls lh_numeric_big_set_word(self), which checks it again
lib/lh\src\lh\numeric\big.c: lh_numeric_big_mul_u32 checks self and calls lh_numeric_big_set_word(self), which checks it again
lib/lh\src\lh\numeric\big.c: lh_numeric_big_mul_u32 checks self and calls lh_numeric_big_set_word(self), which checks it again
lib/lh\src\lh\numeric\big.c: lh_numeric_big_mul_u32 checks self and calls lh_numeric_big_set_word(self), which checks it again
lib/lh\src\lh\numeric\big.c: lh_numeric_big_shl checks self and calls lh_numeric_big_set_word(self), which checks it again
lib/lh\src\lh\os\app.c: lh_os_app_init checks self and calls lh_os_app_set_quit(self), which checks it again
lib/lh\src\lh\os\app.c: lh_os_app_quit checks self and calls lh_os_app_set_quit(self), which checks it again
lib/lh\src\lh\os\app.c: lh_os_app_deinit checks self and calls lh_os_app_get_first_window(self), which checks it again
lib/lh\src\lh\os\app.c: lh_os_app_deinit checks self and calls lh_os_app_set_quit(self), which checks it again
lib/lh\src\lh\os\app.c: lh_os_app_run checks self and calls lh_os_app_is_quit(self), which checks it again
lib/lh\src\lh\os\app.c: lh_os_app_run checks self and calls lh_os_app_set_quit(self), which checks it again
lib/lh\src\lh\os\serial.c: lh_os_serial_init checks self and calls lh_os_serial_set_handle(self), which checks it again
lib/lh\src\lh\os\serial.c: lh_os_serial_is_open checks self and calls lh_os_serial_get_handle(self), which checks it again
lib/lh\src\lh\os\serial.c: lh_os_serial_close checks self and calls lh_os_serial_set_handle(self), which checks it again
lib/lh\src\lh\os\wake.c: lh_os_wake_init checks self and calls lh_os_wake_set_reader(self), which checks it again
lib/lh\src\lh\os\wake.c: lh_os_wake_init checks self and calls lh_os_wake_set_writer(self), which checks it again
lib/lh\src\lh\os\wake.c: lh_os_wake_open checks self and calls lh_os_wake_get_reader(self), which checks it again
lib/lh\src\lh\os\wake.c: lh_os_wake_open checks self and calls lh_os_wake_set_reader(self), which checks it again
lib/lh\src\lh\os\wake.c: lh_os_wake_open checks self and calls lh_os_wake_set_writer(self), which checks it again
lib/lh\src\lh\os\wake.c: lh_os_wake_get_handle checks self and calls lh_os_wake_get_reader(self), which checks it again
lib/lh\src\lh\os\wake.c: lh_os_wake_close checks self and calls lh_os_wake_get_reader(self), which checks it again
lib/lh\src\lh\os\wake.c: lh_os_wake_close checks self and calls lh_os_wake_get_writer(self), which checks it again
lib/lh\src\lh\os\wake.c: lh_os_wake_close checks self and calls lh_os_wake_init(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_app(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_parent(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_modal(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_closing(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_frame(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_corner(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_placement(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_maximized(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_zone_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_zone_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_resize_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_resize_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_paint_dc(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_paint_left(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_paint_top(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_paint_right(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_paint_bottom(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_tick_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_tick_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_tick_ms(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_paint_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_paint_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_frame_stats(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_input_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_input_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_controller(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_controller_armed(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_pressed_buttons(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_close_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_init checks self and calls lh_os_window_set_on_close_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_on_close checks self and calls lh_os_window_set_on_close_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_on_close checks self and calls lh_os_window_set_on_close_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_on_paint checks self and calls lh_os_window_set_on_paint_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_on_paint checks self and calls lh_os_window_set_on_paint_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_on_input checks self and calls lh_os_window_set_on_input_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_on_input checks self and calls lh_os_window_set_on_input_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_tick checks self and calls lh_os_window_get_on_tick_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_tick checks self and calls lh_os_window_get_on_tick_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_invalidate checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_invalidate_rect checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_frame_v checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_frame_v checks self and calls lh_os_window_set_frame(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_corner_radius checks self and calls lh_os_window_set_corner(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_corner_radius checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_corner_radius checks self and calls lh_os_window_get_corner(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_placement_v checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_placement_v checks self and calls lh_os_window_set_placement(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_on_zone checks self and calls lh_os_window_set_on_zone_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_set_on_zone checks self and calls lh_os_window_set_on_zone_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_zone_at checks self and calls lh_os_window_get_on_zone_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_zone_at checks self and calls lh_os_window_get_on_zone_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_zone_at checks self and calls lh_os_window_get_on_zone_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_resize checks self and calls lh_os_window_set_maximized(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_resize checks self and calls lh_os_window_get_on_resize_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_resize checks self and calls lh_os_window_get_on_resize_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_minimize checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_maximize checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_input checks self and calls lh_os_window_get_on_input_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_input checks self and calls lh_os_window_get_on_input_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_input checks self and calls lh_os_window_get_on_input_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_destroy checks self and calls lh_os_window_get_app(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_destroy checks self and calls lh_os_window_get_parent(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_destroy checks self and calls lh_os_window_is_modal(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_destroy checks self and calls lh_os_window_is_closing(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_destroy checks self and calls lh_os_window_set_closing(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_destroy checks self and calls lh_os_window_set_controller_armed(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_destroy checks self and calls lh_os_window_set_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_destroy checks self and calls lh_os_window_get_on_close_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_on_native_destroy checks self and calls lh_os_window_get_on_close_context(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_close checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_close checks self and calls lh_os_window_set_tick_ms(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_close checks self and calls lh_os_window_set_controller_armed(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_close checks self and calls lh_os_window_get_first_child(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_close checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_close checks self and calls lh_os_window_set_closing(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_close checks self and calls lh_os_window_get_handle(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_close checks self and calls lh_os_window_on_native_destroy(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_deinit checks self and calls lh_os_window_close(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_deinit checks self and calls lh_os_window_set_on_close_fn(self), which checks it again
lib/lh\src\lh\os\window.c: lh_os_window_deinit checks self and calls lh_os_window_set_on_close_context(self), which checks it again
lib/lh\src\lh\os\worker.c: lh_os_worker_take checks self and calls lh_os_worker_set_count(self), which checks it again
lib/lh\src\lh\os\worker.c: lh_os_worker_stop checks self and calls lh_os_worker_set_stop(self), which checks it again
lib/lh\src\lh\os\worker.c: lh_os_worker_stop checks self and calls lh_os_worker_set_loop(self), which checks it again
lib/lh\src\lh\os\fs\dir.c: lh_os_fs_dir_init checks self and calls lh_os_fs_dir_set_handle(self), which checks it again
lib/lh\src\lh\os\fs\dir.c: lh_os_fs_dir_open checks self and calls lh_os_fs_dir_set_handle(self), which checks it again
lib/lh\src\lh\os\fs\file.c: lh_os_fs_file_init checks self and calls lh_os_fs_file_set_mode(self), which checks it again
lib/lh\src\lh\os\fs\file.c: lh_os_fs_file_close checks self and calls lh_os_fs_file_set_mode(self), which checks it again
lib/lh\src\lh\os\fs\file.c: lh_os_fs_file_close checks self and calls lh_os_fs_file_get_handle(self), which checks it again
lib/lh\src\lh\os\fs\file.c: lh_os_fs_file_close checks self and calls lh_os_fs_file_set_mode(self), which checks it again
lib/lh\src\lh\os\fs\file.c: lh_os_fs_file_deinit checks self and calls lh_os_fs_file_close(self), which checks it again
lib/lh\src\lh\os\fs\file.c: lh_os_fs_file_open checks self and calls lh_os_fs_file_close(self), which checks it again
lib/lh\src\lh\os\fs\file.c: lh_os_fs_file_open checks self and calls lh_os_fs_file_set_mode(self), which checks it again
lib/lh\src\lh\os\input\contacts.c: lh_os_input_contacts_get_count checks self and calls lh_os_input_contacts_is_taken(self), which checks it again
lib/lh\src\lh\os\input\event.c: lh_os_input_event_set_delta checks self and calls lh_os_input_event_set_dx(self), which checks it again
lib/lh\src\lh\os\input\event.c: lh_os_input_event_set_delta checks self and calls lh_os_input_event_set_dy(self), which checks it again
lib/lh\src\lh\os\input\queue.c: lh_os_input_queue_init checks self and calls lh_os_input_queue_set_storage(self), which checks it again
lib/lh\src\lh\os\input\queue.c: lh_os_input_queue_init checks self and calls lh_os_input_queue_set_capacity(self), which checks it again
lib/lh\src\lh\os\input\queue.c: lh_os_input_queue_init checks self and calls lh_os_input_queue_set_head(self), which checks it again
lib/lh\src\lh\os\input\queue.c: lh_os_input_queue_init checks self and calls lh_os_input_queue_set_tail(self), which checks it again
lib/lh\src\lh\os\input\queue.c: lh_os_input_queue_init checks self and calls lh_os_input_queue_set_dropped(self), which checks it again
lib/lh\src\lh\os\input\queue.c: lh_os_input_queue_get_count checks self and calls lh_os_input_queue_get_head(self), which checks it again
lib/lh\src\lh\os\input\queue.c: lh_os_input_queue_get_count checks self and calls lh_os_input_queue_get_tail(self), which checks it again
lib/lh\src\lh\os\input\queue.c: lh_os_input_queue_get_count checks self and calls lh_os_input_queue_get_capacity(self), which checks it again
lib/lh\src\lh\os\net\socket.c: lh_os_net_socket_init checks self and calls lh_os_net_socket_set_handle(self), which checks it again
lib/lh\src\lh\os\net\socket.c: lh_os_net_socket_open checks self and calls lh_os_net_socket_set_handle(self), which checks it again
lib/lh\src\lh\os\net\socket.c: lh_os_net_socket_open_nonblocking checks self and calls lh_os_net_socket_set_handle(self), which checks it again
lib/lh\src\lh\os\render\backend\gdi.c: lh_os_render_backend_gdi_context_init checks self and calls lh_os_render_backend_gdi_context_set_hdc(self), which checks it again
lib/lh\src\lh\os\render\backend\gdi.c: lh_os_render_backend_gdi_context_init checks self and calls lh_os_render_backend_gdi_reset_counters(self), which checks it again
lib/lh\src\lh\os\render\backend\gdi.c: lh_os_render_backend_gdi_context_deinit checks self and calls lh_os_render_backend_gdi_context_init(self), which checks it again
lib/lh\src\lh\os\render\backend\gdi.c: lh_os_render_backend_gdi_context_get_frame_us checks self and calls lh_os_render_backend_gdi_context_get_frame_start_us(self), which checks it again
lib/lh\src\lh\sql\driver.c: lh_sql_driver_close checks self and calls lh_sql_driver_get_close_fn(self), which checks it again
lib/lh\src\lh\sql\driver.c: lh_sql_driver_exec checks self and calls lh_sql_driver_get_exec_fn(self), which checks it again
lib/lh\src\lh\sql\driver.c: lh_sql_driver_get_last_insert_id checks self and calls lh_sql_driver_get_last_insert_id_fn(self), which checks it again
lib/lh\src\lh\sql\driver.c: lh_sql_driver_finalize checks self and calls lh_sql_driver_get_finalize_fn(self), which checks it again
lib/lh\src\lh\sql\driver.c: lh_sql_driver_bind_null checks self and calls lh_sql_driver_get_bind_null_fn(self), which checks it again
lib/lh\src\lh\sql\driver.c: lh_sql_driver_step checks self and calls lh_sql_driver_get_step_fn(self), which checks it again
lib/lh\src\lh\sql\driver.c: lh_sql_driver_reset checks self and calls lh_sql_driver_get_reset_fn(self), which checks it again
lib/lh\src\lh\sql\driver.c: lh_sql_driver_get_column_count checks self and calls lh_sql_driver_get_column_count_fn(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_init_native checks self and calls lh_sql_mysql_api_get_init(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_close checks self and calls lh_sql_mysql_api_get_close(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_error checks self and calls lh_sql_mysql_api_get_error(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_error_code checks self and calls lh_sql_mysql_api_get_error_code(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_store_result checks self and calls lh_sql_mysql_api_get_store_result(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_free_result checks self and calls lh_sql_mysql_api_get_free_result(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_fetch_row checks self and calls lh_sql_mysql_api_get_fetch_row(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_fetch_lengths checks self and calls lh_sql_mysql_api_get_fetch_lengths(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_num_fields checks self and calls lh_sql_mysql_api_get_num_fields(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_field_count checks self and calls lh_sql_mysql_api_get_field_count(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_insert_id checks self and calls lh_sql_mysql_api_get_insert_id(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_thread_end checks self and calls lh_sql_mysql_api_get_thread_end(self), which checks it again
lib/lh\src\lh\sql\mysql\api.c: lh_sql_mysql_api_thread_end checks self and calls lh_sql_mysql_api_get_thread_end(self), which checks it again
lib/lh\src\lh\str\format\text.c: lh_str_ptr_format_cstr_v checks str and calls lh_str_ptr_format_text_v(str), which checks it again
lib/lh\src\lh\str\parse\hex.c: lh_str_ptr_parse_hex_prefix checks str and calls lh_str_ptr_parse_hex(str), which checks it again
lib/lh\src\lh\str\parse\uint.c: lh_str_ptr_parse_uint checks str and calls lh_str_ptr_parse_uint_digits(str), which checks it again
lib/lh\src\lh\str\parse\uint.c: lh_str_ptr_parse_uint_prefix checks str and calls lh_str_ptr_parse_uint(str), which checks it again
lib/lh\src\lh\str\scanf\next.c: lh_str_ptr_scanf_next checks fmt and calls lh_str_ptr_scanf_next_literal(fmt), which checks it again
lib/lh\src\lh\str\scanf\next.c: lh_str_ptr_scanf_next checks fmt and calls lh_str_ptr_scanf_next_flags(fmt), which checks it again
lib/lh\src\lh\str\scanf\next.c: lh_str_ptr_scanf_next checks fmt and calls lh_str_ptr_scanf_next_number(fmt), which checks it again
lib/lh\src\lh\str\scanf\next.c: lh_str_ptr_scanf_next checks fmt and calls lh_str_ptr_scanf_next_number(fmt), which checks it again
lib/lh\src\lh\str\scanf\spec.c: lh_str_scanf_spec_set_conversion checks self and calls lh_str_scanf_spec_set_kind(self), which checks it again
lib/lh\src\lh\str\scanf\spec.c: lh_str_scanf_spec_set_conversion checks self and calls lh_str_scanf_spec_set_kind(self), which checks it again
lib/lh\src\lh\str\scanf\spec.c: lh_str_scanf_spec_set_conversion checks self and calls lh_str_scanf_spec_set_kind(self), which checks it again
lib/lh\src\lh\str\scanf\spec.c: lh_str_scanf_spec_set_conversion checks self and calls lh_str_scanf_spec_set_kind(self), which checks it again
lib/lh\src\lh\str\scanf\spec.c: lh_str_scanf_spec_set_conversion checks self and calls lh_str_scanf_spec_set_kind(self), which checks it again
lib/lh\src\lh\str\scanf\spec.c: lh_str_scanf_spec_set_conversion checks self and calls lh_str_scanf_spec_set_kind(self), which checks it again
lib/lh\src\lh\str\scanf\spec.c: lh_str_scanf_spec_set_conversion checks self and calls lh_str_scanf_spec_set_kind(self), which checks it again
lib/lh\src\lh\str\scanf\spec.c: lh_str_scanf_spec_set_conversion checks self and calls lh_str_scanf_spec_set_uppercase(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_init checks self and calls lh_ui_button_get_container_mut(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_init checks self and calls lh_ui_button_set_rest_style(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_init checks self and calls lh_ui_button_set_hot_style(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_init checks self and calls lh_ui_button_set_hot(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_init checks self and calls lh_ui_button_set_on_click_fn(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_init checks self and calls lh_ui_button_set_click_context(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_set_style checks self and calls lh_ui_button_set_rest_style(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_set_style checks self and calls lh_ui_button_is_hot(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_get_style_now checks self and calls lh_ui_button_get_hot_style(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_get_style_now checks self and calls lh_ui_button_get_rest_style(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_set_hot_style_v checks self and calls lh_ui_button_set_hot_style(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_set_hot_style_v checks self and calls lh_ui_button_get_style_now(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_set_hot_v checks self and calls lh_ui_button_set_hot(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_set_hot_v checks self and calls lh_ui_button_get_style_now(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_set_on_click checks self and calls lh_ui_button_set_on_click_fn(self), which checks it again
lib/lh\src\lh\ui\button.c: lh_ui_button_set_on_click checks self and calls lh_ui_button_set_click_context(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_init checks self and calls lh_ui_canvas_set_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_init checks self and calls lh_ui_canvas_set_context(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_init checks self and calls lh_ui_canvas_set_depth(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_init checks self and calls lh_ui_canvas_set_has_damage(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_init checks self and calls lh_ui_canvas_set_scratch(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_init checks self and calls lh_ui_canvas_set_scratch_bytes(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_use_scratch checks self and calls lh_ui_canvas_set_scratch(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_use_scratch checks self and calls lh_ui_canvas_set_scratch_bytes(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_add_damage checks self and calls lh_ui_canvas_is_has_damage(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_add_damage checks self and calls lh_ui_canvas_set_damage(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_add_damage checks self and calls lh_ui_canvas_set_has_damage(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_add_damage checks self and calls lh_ui_canvas_get_damage_mut(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_reset_damage checks self and calls lh_ui_canvas_set_has_damage(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_get_damage_v checks self and calls lh_ui_canvas_is_has_damage(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_get_damage_v checks self and calls lh_ui_canvas_get_damage(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_is_cutting checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_describe_clip checks self and calls lh_ui_canvas_get_rounds(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_save checks self and calls lh_ui_canvas_get_saved_mut(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_save checks self and calls lh_ui_canvas_get_depth(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_save checks self and calls lh_ui_canvas_set_depth(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_save checks self and calls lh_ui_canvas_get_depth(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_get_offset checks self and calls lh_ui_canvas_get_state(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_send_clip checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_send_clip checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_send_clip checks self and calls lh_ui_canvas_get_clip(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_send_clip checks self and calls lh_ui_canvas_get_context(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_send_clip checks self and calls lh_ui_canvas_describe_clip(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_sync_clip checks self and calls lh_ui_canvas_get_state_mut(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_sync_clip checks self and calls lh_ui_canvas_send_clip(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_pop checks self and calls lh_ui_canvas_set_depth(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_pop checks self and calls lh_ui_canvas_get_depth(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_pop checks self and calls lh_ui_canvas_set_state(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_pop checks self and calls lh_ui_canvas_get_saved(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_pop checks self and calls lh_ui_canvas_get_depth(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_pop checks self and calls lh_ui_canvas_sync_clip(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_begin checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_begin checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_begin checks self and calls lh_ui_canvas_get_context(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_begin_area checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_begin_area checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_begin_area checks self and calls lh_ui_canvas_begin(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_begin_area checks self and calls lh_ui_canvas_set_frame_at(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_begin_area checks self and calls lh_ui_canvas_get_context(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_end checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_end checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_end checks self and calls lh_ui_canvas_get_context(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_end checks self and calls lh_ui_canvas_get_drawn_mut(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_clear checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_clear checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_clear checks self and calls lh_ui_canvas_get_context(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_clear checks self and calls lh_ui_canvas_add_damage(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_blur checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_blur checks self and calls lh_ui_canvas_get_backend(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_blur checks self and calls lh_ui_canvas_add_damage(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_blur checks self and calls lh_ui_canvas_get_context(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_blur checks self and calls lh_ui_canvas_get_scratch(self), which checks it again
lib/lh\src\lh\ui\canvas.c: lh_ui_canvas_blur checks self and calls lh_ui_canvas_get_scratch_bytes(self), which checks it again
lib/lh\src\lh\ui\color.c: lh_ui_color_get_argb checks self and calls lh_ui_color_get_a(self), which checks it again
lib/lh\src\lh\ui\color.c: lh_ui_color_get_argb checks self and calls lh_ui_color_get_r(self), which checks it again
lib/lh\src\lh\ui\color.c: lh_ui_color_get_argb checks self and calls lh_ui_color_get_g(self), which checks it again
lib/lh\src\lh\ui\color.c: lh_ui_color_get_argb checks self and calls lh_ui_color_get_b(self), which checks it again
lib/lh\src\lh\ui\color.c: lh_ui_color_get_rgb565 checks self and calls lh_ui_color_get_r(self), which checks it again
lib/lh\src\lh\ui\color.c: lh_ui_color_get_rgb565 checks self and calls lh_ui_color_get_g(self), which checks it again
lib/lh\src\lh\ui\color.c: lh_ui_color_get_rgb565 checks self and calls lh_ui_color_get_b(self), which checks it again
lib/lh\src\lh\ui\container.c: lh_ui_container_init checks self and calls lh_ui_container_set_layout(self), which checks it again
lib/lh\src\lh\ui\container.c: lh_ui_container_init checks self and calls lh_ui_container_set_placing(self), which checks it again
lib/lh\src\lh\ui\container.c: lh_ui_container_get_content_size checks self and calls lh_ui_container_get_entity(self), which checks it again
lib/lh\src\lh\ui\container.c: lh_ui_container_get_content_size checks self and calls lh_ui_container_get_content_far(self), which checks it again
lib/lh\src\lh\ui\container.c: lh_ui_container_set_scroll_v checks self and calls lh_ui_container_get_scroll_mut(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_init checks self and calls lh_ui_entity_set_rect(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_init checks self and calls lh_ui_entity_set_style(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_init checks self and calls lh_ui_entity_set_class(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_init checks self and calls lh_ui_entity_set_hidden(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_init checks self and calls lh_ui_entity_set_pressed(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_init checks self and calls lh_ui_entity_set_parent(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_init checks self and calls lh_ui_entity_set_view(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_set_place_v checks self and calls lh_ui_entity_set_place(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_style_now checks self and calls lh_ui_entity_get_style(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_style_now checks self and calls lh_ui_entity_is_pressed(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_fill_color checks self and calls lh_ui_entity_get_style_now(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_radius_now checks self and calls lh_ui_entity_get_style_now(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_padding checks self and calls lh_ui_entity_get_style(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_set_class_v checks self and calls lh_ui_entity_set_class(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_walk checks self and calls lh_ui_entity_walk_children(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_walk_children checks self and calls lh_ui_entity_get_first_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_walk_children checks self and calls lh_ui_entity_get_next_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_is_shown checks self and calls lh_ui_entity_send(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_extend_bounds checks self and calls lh_ui_entity_get_rect(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_children_bounds checks self and calls lh_ui_entity_get_first_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_children_bounds checks self and calls lh_ui_entity_get_next_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_baseline checks self and calls lh_ui_entity_send(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_measure_bounds checks self and calls lh_ui_entity_send(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_root_offset checks self and calls lh_ui_entity_get_parent(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_get_root_offset checks self and calls lh_ui_entity_get_parent(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_is_hit checks self and calls lh_ui_entity_is_shown(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_searches_children_at checks self and calls lh_ui_entity_get_radius_now(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_searches_children_at checks self and calls lh_ui_entity_get_first_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_find_child_at checks self and calls lh_ui_entity_searches_children_at(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_find_child_at checks self and calls lh_ui_entity_get_last_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_find_child_at checks self and calls lh_ui_entity_get_last_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_find_child_at checks self and calls lh_ui_entity_get_prev_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_set_rect_v checks self and calls lh_ui_entity_get_rect_mut(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_set_rect_v checks self and calls lh_ui_entity_set_rect(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_set_style_at_once checks self and calls lh_ui_entity_set_style(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_set_style_v checks self and calls lh_ui_entity_get_style(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_set_style_v checks self and calls lh_ui_entity_set_style_at_once(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_move_by checks self and calls lh_ui_entity_get_rect_mut(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_move_by checks self and calls lh_ui_entity_get_first_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_move_by checks self and calls lh_ui_entity_get_next_child(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_set_hidden_v checks self and calls lh_ui_entity_set_hidden(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_set_hidden_v checks self and calls lh_ui_entity_set_hidden(self), which checks it again
lib/lh\src\lh\ui\entity.c: lh_ui_entity_shows_on checks self and calls lh_ui_entity_get_measure_bounds(self), which checks it again
lib/lh\src\lh\ui\font.c: lh_ui_font_get_range checks self and calls lh_ui_font_get_range_count(self), which checks it again
lib/lh\src\lh\ui\font.c: lh_ui_font_get_first checks self and calls lh_ui_font_get_range_count(self), which checks it again
lib/lh\src\lh\ui\font.c: lh_ui_font_get_first checks self and calls lh_ui_font_get_ranges(self), which checks it again
lib/lh\src\lh\ui\font.c: lh_ui_font_get_glyph_count checks self and calls lh_ui_font_get_range_count(self), which checks it again
lib/lh\src\lh\ui\font.c: lh_ui_font_get_glyph_count checks self and calls lh_ui_font_get_ranges(self), which checks it again
lib/lh\src\lh\ui\font.c: lh_ui_font_get_advance checks self and calls lh_ui_font_get_advances(self), which checks it again
lib/lh\src\lh\ui\font.c: lh_ui_font_get_top checks self and calls lh_ui_font_get_tops(self), which checks it again
lib/lh\src\lh\ui\font.c: lh_ui_font_get_glyph checks self and calls lh_ui_font_get_glyphs(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_stop_init checks self and calls lh_ui_gradient_stop_set_color_v(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_stop_init checks self and calls lh_ui_gradient_stop_set_frac(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_init checks self and calls lh_ui_gradient_set_stops_count(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_init checks self and calls lh_ui_gradient_set_shape(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_init checks self and calls lh_ui_gradient_set_angle(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_get_stop checks self and calls lh_ui_gradient_get_stops_count(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_get_stop_mut checks self and calls lh_ui_gradient_get_stops_count(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_set_linear checks self and calls lh_ui_gradient_set_shape(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_set_linear checks self and calls lh_ui_gradient_set_angle(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_color_at checks self and calls lh_ui_gradient_get_stops(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_color_at checks self and calls lh_ui_gradient_get_stops_count(self), which checks it again
lib/lh\src\lh\ui\gradient.c: lh_ui_gradient_color_at checks self and calls lh_ui_gradient_get_stops_count(self), which checks it again
lib/lh\src\lh\ui\image.c: lh_ui_image_init checks self and calls lh_ui_image_set_mask(self), which checks it again
lib/lh\src\lh\ui\image.c: lh_ui_image_init checks self and calls lh_ui_image_set_baseline(self), which checks it again
lib/lh\src\lh\ui\image.c: lh_ui_image_set_tint_v checks self and calls lh_ui_image_set_tint(self), which checks it again
lib/lh\src\lh\ui\image.c: lh_ui_image_get_size checks self and calls lh_ui_image_get_mask(self), which checks it again
lib/lh\src\lh\ui\image.c: lh_ui_image_get_size checks self and calls lh_ui_image_get_mask(self), which checks it again
lib/lh\src\lh\ui\image.c: lh_ui_image_get_size checks self and calls lh_ui_image_get_mask(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_init checks self and calls lh_ui_input_set_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_init checks self and calls lh_ui_input_set_capacity(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_init checks self and calls lh_ui_input_set_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_init checks self and calls lh_ui_input_set_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_init checks self and calls lh_ui_input_set_focused(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_init checks self and calls lh_ui_input_set_caret_color(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_init checks self and calls lh_ui_input_set_caret_shape(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_set_text checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_set_text checks self and calls lh_ui_input_get_capacity(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_set_text checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_set_text checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_set_text checks self and calls lh_ui_input_set_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_set_text checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_set_text checks self and calls lh_ui_input_set_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_clear checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_clear checks self and calls lh_ui_input_set_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_clear checks self and calls lh_ui_input_set_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_clear checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_capacity(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_capacity(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_set_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_set_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_insert checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_get_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_set_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_get_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_set_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_backspace checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_set_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_delete_forward checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_set_caret_v checks self and calls lh_ui_input_get_buffer(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_set_caret_v checks self and calls lh_ui_input_set_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_end checks self and calls lh_ui_input_set_caret(self), which checks it again
lib/lh\src\lh\ui\input.c: lh_ui_input_end checks self and calls lh_ui_input_get_used(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_is_zero checks self and calls lh_ui_insets_get_left(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_is_zero checks self and calls lh_ui_insets_get_top(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_is_zero checks self and calls lh_ui_insets_get_right(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_is_zero checks self and calls lh_ui_insets_get_bottom(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_shrink checks self and calls lh_ui_insets_get_left(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_shrink checks self and calls lh_ui_insets_get_top(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_shrink checks self and calls lh_ui_insets_get_left(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_shrink checks self and calls lh_ui_insets_get_right(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_shrink checks self and calls lh_ui_insets_get_top(self), which checks it again
lib/lh\src\lh\ui\insets.c: lh_ui_insets_shrink checks self and calls lh_ui_insets_get_bottom(self), which checks it again
lib/lh\src\lh\ui\key.c: lh_ui_key_input_init_key checks self and calls lh_ui_key_input_set_key(self), which checks it again
lib/lh\src\lh\ui\key.c: lh_ui_key_input_init_key checks self and calls lh_ui_key_input_set_pressed(self), which checks it again
lib/lh\src\lh\ui\key.c: lh_ui_key_input_init_key checks self and calls lh_ui_key_input_set_code(self), which checks it again
lib/lh\src\lh\ui\key.c: lh_ui_key_input_is_down checks self and calls lh_ui_key_input_get_key(self), which checks it again
lib/lh\src\lh\ui\key.c: lh_ui_key_input_is_down checks self and calls lh_ui_key_input_is_pressed(self), which checks it again
lib/lh\src\lh\ui\key.c: lh_ui_key_input_is_down checks self and calls lh_ui_key_input_get_code(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_init checks self and calls lh_ui_label_set_text(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_init checks self and calls lh_ui_label_set_metrics_ready(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_init checks self and calls lh_ui_label_set_text_hash(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_init checks self and calls lh_ui_label_set_text_hashed(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_drop_metrics checks self and calls lh_ui_label_set_metrics_ready(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_drop_metrics checks self and calls lh_ui_label_set_text_hashed(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_get_text_hash(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_get_text_length(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_set_text(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_as_entity(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_get_text_noted_mut(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_get_text_noted_mut(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_get_text_noted_mut(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_set_text(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_set_text_hashed(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_set_text_hash(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_set_text_length(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_set_metrics_ready(self), which checks it again
lib/lh\src\lh\ui\label.c: lh_ui_label_set_text_v checks self and calls lh_ui_label_get_text_noted_mut(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_init checks self and calls lh_ui_layout_set_axis(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_init checks self and calls lh_ui_layout_set_gap(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_init checks self and calls lh_ui_layout_set_justify(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_init checks self and calls lh_ui_layout_set_wrap(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_init checks self and calls lh_ui_layout_set_line_gap(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_set_wrapping checks self and calls lh_ui_layout_set_wrap(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_set_wrapping checks self and calls lh_ui_layout_set_line_gap(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_apply checks self and calls lh_ui_layout_get_axis(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_apply checks self and calls lh_ui_layout_is_wrap(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_apply checks self and calls lh_ui_layout_get_line_gap(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_apply checks self and calls lh_ui_layout_get_line_gap(self), which checks it again
lib/lh\src\lh\ui\layout.c: lh_ui_layout_apply checks self and calls lh_ui_layout_get_line_gap(self), which checks it again
lib/lh\src\lh\ui\mask.c: lh_ui_mask_get_row checks self and calls lh_ui_mask_get_bits(self), which checks it again
lib/lh\src\lh\ui\mask.c: lh_ui_mask_get_row checks self and calls lh_ui_mask_get_row_bytes(self), which checks it again
lib/lh\src\lh\ui\mask.c: lh_ui_mask_has_pixel checks self and calls lh_ui_mask_get_width(self), which checks it again
lib/lh\src\lh\ui\mask.c: lh_ui_mask_has_pixel checks self and calls lh_ui_mask_get_height(self), which checks it again
lib/lh\src\lh\ui\mask.c: lh_ui_mask_get_coverage_8 checks self and calls lh_ui_mask_get_height(self), which checks it again
lib/lh\src\lh\ui\mask.c: lh_ui_mask_get_coverage_8 checks self and calls lh_ui_mask_get_width(self), which checks it again
lib/lh\src\lh\ui\mask.c: lh_ui_mask_get_coverage_8 checks self and calls lh_ui_mask_get_row(self), which checks it again
lib/lh\src\lh\ui\mask.c: lh_ui_mask_get_coverage_8 checks self and calls lh_ui_mask_get_row_bytes(self), which checks it again
lib/lh\src\lh\ui\mask.c: lh_ui_mask_get_coverage_8 checks self and calls lh_ui_mask_get_width(self), which checks it again
lib/lh\src\lh\ui\paint.c: lh_ui_paint_init checks self and calls lh_ui_paint_set_kind(self), which checks it again
lib/lh\src\lh\ui\paint.c: lh_ui_paint_init checks self and calls lh_ui_paint_set_gradient(self), which checks it again
lib/lh\src\lh\ui\paint.c: lh_ui_paint_get_color_v checks self and calls lh_ui_paint_get_color(self), which checks it again
lib/lh\src\lh\ui\paint.c: lh_ui_paint_get_gradient_v checks self and calls lh_ui_paint_get_kind(self), which checks it again
lib/lh\src\lh\ui\paint.c: lh_ui_paint_get_gradient_v checks self and calls lh_ui_paint_get_gradient(self), which checks it again
lib/lh\src\lh\ui\pen.c: lh_ui_pen_set_paint_v checks self and calls lh_ui_pen_set_paint(self), which checks it again
lib/lh\src\lh\ui\pen.c: lh_ui_pen_set_width_v checks self and calls lh_ui_pen_set_width(self), which checks it again
lib/lh\src\lh\ui\pen.c: lh_ui_pen_init_paint checks self and calls lh_ui_pen_set_paint_v(self), which checks it again
lib/lh\src\lh\ui\pen.c: lh_ui_pen_init_paint checks self and calls lh_ui_pen_set_width_v(self), which checks it again
lib/lh\src\lh\ui\pixmap.c: lh_ui_pixmap_get_bounds checks self and calls lh_ui_pixmap_get_width(self), which checks it again
lib/lh\src\lh\ui\pixmap.c: lh_ui_pixmap_get_bounds checks self and calls lh_ui_pixmap_get_height(self), which checks it again
lib/lh\src\lh\ui\pixmap.c: lh_ui_pixmap_get_row checks self and calls lh_ui_pixmap_get_bits(self), which checks it again
lib/lh\src\lh\ui\pixmap.c: lh_ui_pixmap_get_row checks self and calls lh_ui_pixmap_get_stride(self), which checks it again
lib/lh\src\lh\ui\pixmap.c: lh_ui_pixmap_clear checks self and calls lh_ui_pixmap_get_width(self), which checks it again
lib/lh\src\lh\ui\pixmap.c: lh_ui_pixmap_clear checks self and calls lh_ui_pixmap_get_height(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_is_uniform checks self and calls lh_ui_radii_get_top_right(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_is_uniform checks self and calls lh_ui_radii_get_top_right(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_is_uniform checks self and calls lh_ui_radii_get_bottom_right(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_is_uniform checks self and calls lh_ui_radii_get_bottom_right(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_is_uniform checks self and calls lh_ui_radii_get_bottom_left(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_get_min checks self and calls lh_ui_radii_get_top_left(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_get_min checks self and calls lh_ui_radii_get_top_right(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_get_min checks self and calls lh_ui_radii_get_bottom_left(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_get_at checks self and calls lh_ui_radii_get_top_left(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_get_at checks self and calls lh_ui_radii_get_top_right(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_get_at checks self and calls lh_ui_radii_get_bottom_left(self), which checks it again
lib/lh\src\lh\ui\radii.c: lh_ui_radii_get_at checks self and calls lh_ui_radii_get_bottom_right(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_is_empty checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_get checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_drop checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_drop checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_drop checks self and calls lh_ui_rects_get_rects_mut(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_drop checks self and calls lh_ui_rects_get_rects(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_drop checks self and calls lh_ui_rects_set_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_drop checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_join_once checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_join_once checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_join_once checks self and calls lh_ui_rects_get_rects(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_join_once checks self and calls lh_ui_rects_get_rects(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_join_once checks self and calls lh_ui_rects_drop(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_squeeze checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_squeeze checks self and calls lh_ui_rects_get_rects(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_squeeze checks self and calls lh_ui_rects_get_rects_mut(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_squeeze checks self and calls lh_ui_rects_close(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_add checks self and calls lh_ui_rects_squeeze(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_add checks self and calls lh_ui_rects_get_rects_mut(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_add checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_add checks self and calls lh_ui_rects_set_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_add checks self and calls lh_ui_rects_get_count(self), which checks it again
lib/lh\src\lh\ui\rects.c: lh_ui_rects_add checks self and calls lh_ui_rects_close(self), which checks it again
lib/lh\src\lh\ui\renderer3d.c: lh_ui_renderer3d_clear checks self and calls lh_ui_renderer3d_get_height(self), which checks it again
lib/lh\src\lh\ui\renderer3d.c: lh_ui_renderer3d_clear checks self and calls lh_ui_renderer3d_set_target(self), which checks it again
lib/lh\src\lh\ui\renderer3d.c: lh_ui_renderer3d_clear checks self and calls lh_ui_renderer3d_get_width(self), which checks it again
lib/lh\src\lh\ui\renderer3d.c: lh_ui_renderer3d_clear checks self and calls lh_ui_renderer3d_get_depth(self), which checks it again
lib/lh\src\lh\ui\renderer3d.c: lh_ui_renderer3d_clear checks self and calls lh_ui_renderer3d_get_width(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_is_visible checks self and calls lh_ui_scrollbar_get_mode(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_is_visible checks self and calls lh_ui_scrollbar_get_mode(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_get_track_length checks self and calls lh_ui_scrollbar_get_axis(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_get_thumb_start_at checks self and calls lh_ui_scrollbar_get_entity(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_get_thumb_start_at checks self and calls lh_ui_scrollbar_get_axis(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_get_thumb_start_at checks self and calls lh_ui_scrollbar_get_axis(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_set_thumb_start checks self and calls lh_ui_scrollbar_get_axis(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_set_thumb_start checks self and calls lh_ui_scrollbar_get_track_length(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_set_thumb_start checks self and calls lh_ui_scrollbar_get_axis(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_get_thumb_rect checks self and calls lh_ui_scrollbar_get_axis(self), which checks it again
lib/lh\src\lh\ui\scrollbar.c: lh_ui_scrollbar_contains_thumb checks self and calls lh_ui_scrollbar_get_thumb_rect(self), which checks it again
lib/lh\src\lh\ui\shadow.c: lh_ui_shadow_init checks self and calls lh_ui_shadow_set_spread(self), which checks it again
lib/lh\src\lh\ui\shadow.c: lh_ui_shadow_init checks self and calls lh_ui_shadow_set_offset_x(self), which checks it again
lib/lh\src\lh\ui\shadow.c: lh_ui_shadow_init checks self and calls lh_ui_shadow_set_offset_y(self), which checks it again
lib/lh\src\lh\ui\shadow.c: lh_ui_shadow_is_empty checks self and calls lh_ui_shadow_get_spread(self), which checks it again
lib/lh\src\lh\ui\shadow.c: lh_ui_shadow_set_spread_v checks self and calls lh_ui_shadow_set_spread(self), which checks it again
lib/lh\src\lh\ui\shadow.c: lh_ui_shadow_set_offset checks self and calls lh_ui_shadow_set_offset_x(self), which checks it again
lib/lh\src\lh\ui\shadow.c: lh_ui_shadow_set_offset checks self and calls lh_ui_shadow_set_offset_y(self), which checks it again
lib/lh\src\lh\ui\shadow.c: lh_ui_shadow_get_outset checks self and calls lh_ui_shadow_get_spread(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_radius(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_hit_radius(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_font(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_align_h(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_align_v(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_overflow(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_wrap(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_pressed(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_transition(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_init checks self and calls lh_ui_style_set_easing(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_set_radius_v checks self and calls lh_ui_style_set_radius(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_set_radii_v checks self and calls lh_ui_style_set_radii(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_set_radii_v checks self and calls lh_ui_style_set_radius(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_set_hit_radius_v checks self and calls lh_ui_style_set_hit_radius(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_set_padding_v checks self and calls lh_ui_style_set_padding(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_set_shadow_v checks self and calls lh_ui_style_set_shadow(self), which checks it again
lib/lh\src\lh\ui\style.c: lh_ui_style_set_border_v checks self and calls lh_ui_style_set_border(self), which checks it again
lib/lh\src\lh\ui\surface.c: lh_ui_surface_init checks self and calls lh_ui_surface_set_format(self), which checks it again
lib/lh\src\lh\ui\surface.c: lh_ui_surface_init checks self and calls lh_ui_surface_set_handle(self), which checks it again
lib/lh\src\lh\ui\surface.c: lh_ui_surface_deinit checks self and calls lh_ui_surface_init(self), which checks it again
lib/lh\src\lh\ui\surface.c: lh_ui_surface_set_format_v checks self and calls lh_ui_surface_set_format(self), which checks it again
lib/lh\src\lh\ui\surface.c: lh_ui_surface_set_size_v checks self and calls lh_ui_surface_set_handle(self), which checks it again
lib/lh\src\lh\ui\surface.c: lh_ui_surface_set_size_v checks self and calls lh_ui_surface_set_size(self), which checks it again
lib/lh\src\lh\ui\surface.c: lh_ui_surface_set_size_v checks self and calls lh_ui_surface_set_handle(self), which checks it again
lib/lh\src\lh\ui\surface.c: lh_ui_surface_set_size_v checks self and calls lh_ui_surface_is_valid(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_init checks self and calls lh_ui_toggle_get_button_mut(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_init checks self and calls lh_ui_toggle_set_off_style(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_init checks self and calls lh_ui_toggle_set_off_hot_style(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_init checks self and calls lh_ui_toggle_set_on_style(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_init checks self and calls lh_ui_toggle_set_on_hot_style(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_init checks self and calls lh_ui_toggle_set_checked(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_init checks self and calls lh_ui_toggle_set_on_click_fn(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_init checks self and calls lh_ui_toggle_set_click_context(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_set_on_styles checks self and calls lh_ui_toggle_set_on_style(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_set_on_styles checks self and calls lh_ui_toggle_set_on_hot_style(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_set_checked_v checks self and calls lh_ui_toggle_set_checked(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_set_on_click checks self and calls lh_ui_toggle_set_on_click_fn(self), which checks it again
lib/lh\src\lh\ui\toggle.c: lh_ui_toggle_set_on_click checks self and calls lh_ui_toggle_set_click_context(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_init checks self and calls lh_ui_transition_set_entity(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_init checks self and calls lh_ui_transition_set_to(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_init checks self and calls lh_ui_transition_set_elapsed(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_init checks self and calls lh_ui_transition_set_total(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_get_entity(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_get_elapsed(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_get_total(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_set_elapsed(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_get_total(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_set_look(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_set_elapsed(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_get_total(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_get_from_mut(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_get_to(self), which checks it again
lib/lh\src\lh\ui\transition.c: lh_ui_transition_advance checks self and calls lh_ui_transition_get_to(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_add_damage checks self and calls lh_ui_view_is_has_pending(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_add_damage checks self and calls lh_ui_view_set_pending(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_add_damage checks self and calls lh_ui_view_set_has_pending(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_add_damage checks self and calls lh_ui_view_get_pending_mut(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_take_damage checks self and calls lh_ui_view_is_has_pending(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_take_damage checks self and calls lh_ui_view_set_has_pending(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_add_whole_area checks self and calls lh_ui_view_get_whole_mut(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_add_whole_area checks self and calls lh_ui_view_get_whole_count(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_add_whole_area checks self and calls lh_ui_view_set_whole_count(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_add_whole_area checks self and calls lh_ui_view_get_whole_count(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_draw checks self and calls lh_ui_view_get_canvas(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_hit_test checks self and calls lh_ui_view_get_root(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_stop_throw checks self and calls lh_ui_view_get_throwing(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_stop_throw checks self and calls lh_ui_view_set_throwing(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_reset_pointer checks self and calls lh_ui_view_get_pointers_mut(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_set_canvas(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_set_root(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_set_clear(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_set_strip_height(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_set_scrolling(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_set_throwing(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_set_focus(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_set_whole_count(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_set_has_pending(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_get_pointers_mut(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_init checks self and calls lh_ui_view_get_transitions_mut(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_click checks self and calls lh_ui_view_get_root(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_set_pressed checks self and calls lh_ui_view_damage_shape(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_start_pointer checks self and calls lh_ui_view_stop_throw(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_release_pointer checks self and calls lh_ui_view_click(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_set_focus_v checks self and calls lh_ui_view_get_focus(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_set_focus_v checks self and calls lh_ui_view_set_focus(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_press_pointer checks self and calls lh_ui_view_start_pointer(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_key checks self and calls lh_ui_view_set_focus_v(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_cancel checks self and calls lh_ui_view_cancel_pointer(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_input checks self and calls lh_ui_view_key(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_input checks self and calls lh_ui_view_text(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_input checks self and calls lh_ui_view_cancel(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_set_root_v checks self and calls lh_ui_view_finish_transitions(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_set_root_v checks self and calls lh_ui_view_get_root(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_set_root_v checks self and calls lh_ui_view_set_root(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_tick checks self and calls lh_ui_view_advance(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_tick checks self and calls lh_ui_view_get_throwing(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_tick checks self and calls lh_ui_view_get_velocity(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_tick checks self and calls lh_ui_view_get_velocity_mut(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_tick checks self and calls lh_ui_view_get_throwing(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_tick checks self and calls lh_ui_view_get_velocity(self), which checks it again
lib/lh\src\lh\ui\view.c: lh_ui_view_tick checks self and calls lh_ui_view_stop_throw(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_take checks self and calls lh_ui_blur_window_set_r(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_take checks self and calls lh_ui_blur_window_get_r(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_take checks self and calls lh_ui_blur_window_set_g(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_take checks self and calls lh_ui_blur_window_get_g(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_take checks self and calls lh_ui_blur_window_set_b(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_take checks self and calls lh_ui_blur_window_get_b(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_take checks self and calls lh_ui_blur_window_set_a(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_take checks self and calls lh_ui_blur_window_get_a(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_drop checks self and calls lh_ui_blur_window_set_r(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_drop checks self and calls lh_ui_blur_window_get_r(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_drop checks self and calls lh_ui_blur_window_set_g(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_drop checks self and calls lh_ui_blur_window_get_g(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_drop checks self and calls lh_ui_blur_window_set_b(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_drop checks self and calls lh_ui_blur_window_get_b(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_drop checks self and calls lh_ui_blur_window_set_a(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_drop checks self and calls lh_ui_blur_window_get_a(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_average checks self and calls lh_ui_blur_window_get_a(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_average checks self and calls lh_ui_blur_window_get_r(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_average checks self and calls lh_ui_blur_window_get_g(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_average checks self and calls lh_ui_blur_window_get_b(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_average checks self and calls lh_ui_blur_window_get_a(self), which checks it again
lib/lh\src\lh\ui\blur\window.c: lh_ui_blur_window_average checks self and calls lh_ui_blur_window_get_a(self), which checks it again
lib/lh\src\lh\ui\canvas\state.c: lh_ui_canvas_state_init checks self and calls lh_ui_canvas_state_set_clipped(self), which checks it again
lib/lh\src\lh\ui\canvas\state.c: lh_ui_canvas_state_init checks self and calls lh_ui_canvas_state_set_round_count(self), which checks it again
lib/lh\src\lh\ui\canvas\state.c: lh_ui_canvas_state_get_clip_v checks self and calls lh_ui_canvas_state_is_clipped(self), which checks it again
lib/lh\src\lh\ui\canvas\state.c: lh_ui_canvas_state_get_clip_v checks self and calls lh_ui_canvas_state_get_clip(self), which checks it again
lib/lh\src\lh\ui\canvas\state.c: lh_ui_canvas_state_move checks self and calls lh_ui_canvas_state_get_offset_mut(self), which checks it again
lib/lh\src\lh\ui\canvas\sw.c: lh_ui_canvas_sw_use_pixmap checks self and calls lh_ui_canvas_sw_set_pixmap(self), which checks it again
lib/lh\src\lh\ui\canvas\sw.c: lh_ui_canvas_sw_use_pixmap checks self and calls lh_ui_canvas_sw_get_limit_mut(self), which checks it again
lib/lh\src\lh\ui\canvas\clip\round.c: lh_ui_canvas_clip_round_coverage checks self and calls lh_ui_canvas_clip_round_get_radius(self), which checks it again
lib/lh\src\lh\ui\entity\event.c: lh_ui_entity_event_init checks self and calls lh_ui_entity_event_set_code(self), which checks it again
lib/lh\src\lh\ui\entity\event.c: lh_ui_entity_event_init checks self and calls lh_ui_entity_event_set_context(self), which checks it again
lib/lh\src\lh\ui\entity\transform.c: lh_ui_entity_transform_init checks self and calls lh_ui_entity_transform_set_clip(self), which checks it again
lib/lh\src\lh\ui\entity\transform.c: lh_ui_entity_transform_is_identity checks self and calls lh_ui_entity_transform_is_clip(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_init checks self and calls lh_ui_font_range_set_first(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_init checks self and calls lh_ui_font_range_set_length(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_init checks self and calls lh_ui_font_range_set_base(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_get_last checks self and calls lh_ui_font_range_get_length(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_get_last checks self and calls lh_ui_font_range_get_first(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_get_last checks self and calls lh_ui_font_range_get_length(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_get_last checks self and calls lh_ui_font_range_get_first(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_has checks self and calls lh_ui_font_range_get_first(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_has checks self and calls lh_ui_font_range_get_first(self), which checks it again
lib/lh\src\lh\ui\font\range.c: lh_ui_font_range_has checks self and calls lh_ui_font_range_get_length(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_frames(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_unknown(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_wasteful(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_drawn_px(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_asked_px(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_rects(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_us(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_worst_us(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_on_frame_fn(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_init checks self and calls lh_ui_frame_stats_set_on_frame_context(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_set_frames(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_frames(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_set_us(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_us(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_worst_us(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_set_worst_us(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_set_unknown(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_unknown(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_set_drawn_px(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_drawn_px(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_set_asked_px(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_asked_px(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_set_rects(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_rects(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_set_wasteful(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_wasteful(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_on_frame_fn(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_on_frame_fn(self), which checks it again
lib/lh\src\lh\ui\frame\stats.c: lh_ui_frame_stats_record checks self and calls lh_ui_frame_stats_get_on_frame_context(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler.c: lh_ui_gradient_sampler_is_flat checks self and calls lh_ui_gradient_sampler_get_height(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler.c: lh_ui_gradient_sampler_is_flat checks self and calls lh_ui_gradient_sampler_get_shape(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler.c: lh_ui_gradient_sampler_is_flat checks self and calls lh_ui_gradient_sampler_get_length(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_next checks self and calls lh_ui_gradient_sampler_lerp_get_quotient_mut(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_next checks self and calls lh_ui_gradient_sampler_lerp_get_step(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_next checks self and calls lh_ui_gradient_sampler_lerp_get_rest_mut(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_next checks self and calls lh_ui_gradient_sampler_lerp_get_step_rest(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_next checks self and calls lh_ui_gradient_sampler_lerp_get_span(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_next checks self and calls lh_ui_gradient_sampler_lerp_get_rest_mut(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_next checks self and calls lh_ui_gradient_sampler_lerp_get_span(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_next checks self and calls lh_ui_gradient_sampler_lerp_get_quotient_mut(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_color checks self and calls lh_ui_gradient_sampler_lerp_get_quotient(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_color checks self and calls lh_ui_gradient_sampler_lerp_get_quotient(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_color checks self and calls lh_ui_gradient_sampler_lerp_get_quotient(self), which checks it again
lib/lh\src\lh\ui\gradient\sampler\lerp.c: lh_ui_gradient_sampler_lerp_color checks self and calls lh_ui_gradient_sampler_lerp_get_quotient(self), which checks it again
lib/lh\src\lh\ui\layout\place.c: lh_ui_place_init checks self and calls lh_ui_place_set_size_mode(self), which checks it again
lib/lh\src\lh\ui\layout\place.c: lh_ui_place_init checks self and calls lh_ui_place_set_size(self), which checks it again
lib/lh\src\lh\ui\layout\place.c: lh_ui_place_init checks self and calls lh_ui_place_set_align(self), which checks it again
lib/lh\src\lh\ui\layout\place.c: lh_ui_place_set_sizing checks self and calls lh_ui_place_set_size_mode(self), which checks it again
lib/lh\src\lh\ui\layout\place.c: lh_ui_place_set_sizing checks self and calls lh_ui_place_set_size(self), which checks it again
lib/lh\src\lh\ui\layout\place.c: lh_ui_place_assign_margin checks self and calls lh_ui_place_set_margin(self), which checks it again
lib/lh\src\lh\ui\renderer3d\camera.c: lh_ui_renderer3d_camera_is_usable checks self and calls lh_ui_renderer3d_camera_get_tangent(self), which checks it again
lib/lh\src\lh\ui\renderer3d\camera.c: lh_ui_renderer3d_camera_is_usable checks self and calls lh_ui_renderer3d_camera_get_near_plane(self), which checks it again
lib/lh\src\lh\ui\renderer3d\camera.c: lh_ui_renderer3d_camera_is_usable checks self and calls lh_ui_renderer3d_camera_get_far_plane(self), which checks it again
lib/lh\src\lh\ui\renderer3d\camera.c: lh_ui_renderer3d_camera_is_usable checks self and calls lh_ui_renderer3d_camera_get_near_plane(self), which checks it again
lib/lh\src\lh\ui\shadow\ramp.c: lh_ui_shadow_ramp_init checks self and calls lh_ui_shadow_ramp_set_count(self), which checks it again
lib/lh\src\lh\ui\shadow\ramp.c: lh_ui_shadow_ramp_init checks self and calls lh_ui_shadow_ramp_get_count(self), which checks it again
lib/lh\src\lh\ui\shadow\ramp.c: lh_ui_shadow_ramp_init checks self and calls lh_ui_shadow_ramp_get_square_mut(self), which checks it again
lib/lh\src\lh\ui\shadow\shape.c: lh_ui_shadow_shape_row checks self and calls lh_ui_shadow_shape_get_cy(self), which checks it again
lib/lh\src\lh\ui\shadow\shape.c: lh_ui_shadow_shape_row checks self and calls lh_ui_shadow_shape_get_cy(self), which checks it again
lib/lh\src\lh\ui\shadow\shape.c: lh_ui_shadow_shape_row checks self and calls lh_ui_shadow_shape_get_cy(self), which checks it again
lib/lh\src\lh\ui\shadow\shape.c: lh_ui_shadow_shape_row checks self and calls lh_ui_shadow_shape_get_hy(self), which checks it again
lib/lh\src\lh\ui\text\block.c: lh_ui_text_block_init checks self and calls lh_ui_text_block_set_width(self), which checks it again
lib/lh\src\lh\ui\text\block.c: lh_ui_text_block_init checks self and calls lh_ui_text_block_set_ink_top(self), which checks it again
lib/lh\src\lh\ui\text\block.c: lh_ui_text_block_init checks self and calls lh_ui_text_block_set_lines(self), which checks it again
lib/lh\src\lh\ui\text\line.c: lh_ui_text_line_init checks self and calls lh_ui_text_line_set_begin(self), which checks it again
lib/lh\src\lh\ui\text\line.c: lh_ui_text_line_init checks self and calls lh_ui_text_line_set_end(self), which checks it again
lib/lh\src\lh\ui\text\line.c: lh_ui_text_line_init checks self and calls lh_ui_text_line_set_next(self), which checks it again
lib/lh\src\lh\ui\text\line.c: lh_ui_text_line_init checks self and calls lh_ui_text_line_set_width(self), which checks it again
lib/lh\src\lh\ui\text\line.c: lh_ui_text_line_init checks self and calls lh_ui_text_line_set_ink_top(self), which checks it again
lib/lh\src\lh\ui\text\line.c: lh_ui_text_line_init checks self and calls lh_ui_text_line_set_ink_bottom(self), which checks it again
lib/lh\src\lh\ui\text\line.c: lh_ui_text_line_init checks self and calls lh_ui_text_line_set_inked(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_reset checks self and calls lh_ui_view_pointer_set_target(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_reset checks self and calls lh_ui_view_pointer_set_drag(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_reset checks self and calls lh_ui_view_pointer_set_grab(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_reset checks self and calls lh_ui_view_pointer_set_grab_offset(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_reset checks self and calls lh_ui_view_pointer_set_pressed(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_reset checks self and calls lh_ui_view_pointer_set_dragged(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_init checks self and calls lh_ui_view_pointer_reset(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_start checks self and calls lh_ui_view_pointer_reset(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_start checks self and calls lh_ui_view_pointer_set_pressed(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_start checks self and calls lh_ui_view_pointer_set_press_point(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_start checks self and calls lh_ui_view_pointer_set_last_point(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_is_active checks self and calls lh_ui_view_pointer_is_pressed(self), which checks it again
lib/lh\src\lh\ui\view\pointer.c: lh_ui_view_pointer_track checks self and calls lh_ui_view_pointer_set_last_point(self), which checks it again
948 double check(s).
```

## First implementation pass

Removed six assertions: `lh_{str,wstr}_as_view(out)`,
`lh_{str,wstr}_truncate(self)` and `lh_{str,wstr}_list_get(out)`.
The view output reaches `memory_view_init -> set_v -> assign_v -> assign ->
set -> set_begin` before any dereference of that output; truncate checks
through `get_size -> array_get_size`; list_get forwards its output through
`view_from_offset` to the view setters. Narrow and wide forms follow the
same contracts. Added GoogleTest death coverage for all six functions.

Retained the existing same-file candidates in memory and strings:

- `memory_std_move`: tiny, SIMD and overlapping forward paths do not all
  delegate to the public copy bricks.
- `parse_uint`, `parse_uint_prefix`, `parse_hex_prefix`: read the string
  before delegating, with early returns as well.
- `scanf_next`: reads `fmt` and `pos`, and writes `out`, before helper calls.
- `scanf_spec_set_conversion`: its default branch makes no setter call.
- `format_cstr_v`: its size assertion is between the pointer assertion and
  delegation. Kept in this pass to preserve the selected failure when both
  arguments violate the contract.
- Cross-file `format_put`, `format_uint`, `format_real` and
  `scanf_next_flags`: there are early-return/no-setter paths.

The original 948 count remains a historical candidate inventory, not a
remaining-work counter. These six removals are outside its limited matching.
A missing entry guard in `scanf_spec_clear` was recorded separately in the
cleanup backlog; it is not a redundant assertion removal.

Validation so far: Windows Debug `lh_test_memory` (297 tests) and
`lh_test_str` (288 tests) passed. Changed-line git-clang-format reports no
format changes; layout, includes, mirror, extern-C, native and foundation
convention checks pass. `check-decls` reports 13 existing string declaration
piles, recorded in cleanup rather than mixed into this assertion change.
Authum host and example modules build. Of 14 integration suites excluding
logs, 12 initially passed; rebuilding the examples test removed its stale
ABI failure. The cfg suite still rejects null `src` in `memory_std_copy`,
reproduced with all four changed LH source files restored to HEAD. Full
Authum build is impeded by a locked `authum_test_logs.exe`; that process was
not terminated. The cfg failure is recorded in cleanup.

Linux Debug (Ubuntu 22.04 / WSL) also passed both suites:
`lh_test_memory` 198.80 s, `lh_test_str` 11.74 s. Final source files retain
the six removals; the baseline comparison was temporary and restored in a
`finally` block. No commits were made.

## Config failure resolved

The cfg failure came from `lh_str_view_split_next_line` discarding the
source address for an empty line. The pointer splitter returned a valid
source address with size zero, but its view wrapper replaced that with an
uninitialized/null-backed empty view. `lh_str_view_split_next_of` had the
same conversion. Both now always initialize the result from the pointer
splitter's address and size, preserving empty slices without allocation.
Public contracts and the LH changelog document the borrowed source address.
No Authum source or memory-copy contract was changed.

Two regression tests for empty line/field source addresses failed before
the fix and pass after it. Windows Debug `lh_test_str` and `authum_client`
pass, including the existing cfg blank-line integration case. The cfg debt
item was removed from cleanup.

Final fix validation: all 290 string tests passed on Windows Debug and
Linux Debug (Ubuntu 22.04 / WSL); all 81 Authum client tests passed on
Windows Debug. Formatting and repository convention checks passed.
