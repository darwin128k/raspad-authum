# LH Release benchmarks — 2026-10-31

Intel Core i7-3770K (4 cores / 8 threads), Windows, x86, MSYS2 GCC 16.2.0.
LH revision `dbf07062`; benchmark-only compatibility repairs are uncommitted.
Release `-O3 -DNDEBUG`, maximum Release optimization enabled, FAT LTO, OWN memory backend, double real, UI/OS enabled.
Google Benchmark 1.8.5. Full run: 442 cases, 3 repeats, 0.05 s minimum per repetition, no errors.
Focused run: 21 cases, 5 repeats, 0.2 s minimum, random interleaving, no errors.
Values below are wall-time medians; GiB/s is bytes divided by median wall time (2^30 bytes/GiB).
These are repeated same-buffer microbenchmarks, so small working sets are cache-resident; this is not a cold-memory bandwidth measurement.

## Copy comparison (focused run)

| Bytes | LH time, ns | CRT memcpy time, ns | LH GiB/s | CRT GiB/s | CRT time / LH time |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 64 | 8.23 | 14.06 | 7.24 | 4.24 | 1.71 |
| 4096 | 100.80 | 319.64 | 37.84 | 11.93 | 3.17 |
| 65536 | 2453.58 | 6204.88 | 24.88 | 9.84 | 2.53 |
| 1048576 | 50926.52 | 61842.25 | 19.18 | 15.79 | 1.21 |
| 16777216 | 2008674.50 | 2615633.33 | 7.78 | 5.97 | 1.30 |

## Other focused measurements

| Benchmark | Wall median |
| --- | ---: |
| `BM_memory_std_set/1048576` | 38355.349 ns |
| `BM_ui_view_paint/1/0` | 294.896 us |
| `BM_os_net_tcp_roundtrip/64` | 23.300 us |
| `BM_memory_std_set/4096` | 79.416 ns |
| `BM_crypto_sha_update_finish/2/4096` | 21988.839 ns |
| `BM_str_format_text_mixed_conversions` | 83.241 ns |
| `BM_ui_view_paint/0/0` | 242.772 us |
| `BM_net_ip4_parse` | 33.636 ns |
| `BM_ui_view_paint/2/0` | 318.237 us |
| `BM_os_tick_us` | 52.285 ns |
| `BM_str_parse_uint_10_digits` | 6.010 ns |

UI screen size is 480 × 320; paint variants 0/0, 1/0, 2/0 mean cards, flex and text in ARGB8888.
The 10-digit parser benchmark observes the success flag; do not treat it as a guarantee for every consumer/output usage.

## Full median inventory

| Benchmark | Label | Wall median |
| --- | --- | ---: |
| `BM_os_tick_us` |  | 55.269 ns |
| `BM_os_tick_ms` |  | 62.221 ns |
| `BM_os_timestamp_now` |  | 15.523 ns |
| `BM_os_fs_file_write/4096` |  | 302.690 us |
| `BM_os_fs_file_write/1048576` |  | 4973.072 us |
| `BM_os_fs_file_read/4096` |  | 59.499 us |
| `BM_os_fs_file_read/1048576` |  | 151.771 us |
| `BM_os_fs_stat` |  | 10193.871 ns |
| `BM_os_fs_dir_list` |  | 71.377 us |
| `BM_os_net_tcp_roundtrip/64` |  | 22.838 us |
| `BM_os_net_tcp_roundtrip/4096` |  | 25.119 us |
| `BM_os_net_tcp_roundtrip/65536` |  | 77.105 us |
| `BM_os_net_udp_send_recv/64` |  | 10.596 us |
| `BM_os_net_udp_send_recv/1400` |  | 11.267 us |
| `BM_ui_web_frame/0` | cards | 103.603 us |
| `BM_ui_web_frame/1` | flex | 141.411 us |
| `BM_ui_web_frame/2` | text | 315.390 us |
| `BM_ui_web_frame_glyphs_cached/0` | cards | 102.181 us |
| `BM_ui_web_frame_glyphs_cached/2` | text | 317.698 us |
| `BM_ui_web_ws_write/64` |  | 17.287 ns |
| `BM_ui_web_ws_write/4096` |  | 313.720 ns |
| `BM_ui_web_ws_write/65536` |  | 7514.290 ns |
| `BM_ui_web_ws_read_unmask/16` |  | 19.168 ns |
| `BM_ui_web_ws_read_unmask/1024` |  | 889.540 ns |
| `BM_ui_web_ws_read_unmask/16384` |  | 13647.902 ns |
| `BM_ui_web_page_build` |  | 15244.578 ns |
| `BM_ui_view_paint/0/0` | cards/argb8888 | 243.797 us |
| `BM_ui_view_paint/1/0` | flex/argb8888 | 291.591 us |
| `BM_ui_view_paint/2/0` | text/argb8888 | 351.387 us |
| `BM_ui_view_paint/0/1` | cards/rgb565 | 237.778 us |
| `BM_ui_view_paint/1/1` | flex/rgb565 | 276.258 us |
| `BM_ui_view_paint/2/1` | text/rgb565 | 267.819 us |
| `BM_ui_view_draw_partial/0/0` | cards/argb8888 | 10.584 us |
| `BM_ui_view_draw_partial/1/0` | flex/argb8888 | 9.614 us |
| `BM_ui_view_draw_partial/2/0` | text/argb8888 | 74.236 us |
| `BM_ui_view_draw_partial/0/1` | cards/rgb565 | 10.363 us |
| `BM_ui_view_draw_partial/1/1` | flex/rgb565 | 9.304 us |
| `BM_ui_view_draw_partial/2/1` | text/rgb565 | 66.897 us |
| `BM_ui_layout_apply_flex` |  | 2178.674 ns |
| `BM_ui_view_hit_test/0` | cards | 515.313 ns |
| `BM_ui_view_hit_test/1` | flex | 2039.682 ns |
| `BM_ui_view_press_release` |  | 4714.101 ns |
| `BM_ui_view_tick_idle` |  | 9.563 ns |
| `BM_ui_canvas_clear/0` | argb8888 | 26013.214 ns |
| `BM_ui_canvas_clear/1` | rgb565 | 14798.214 ns |
| `BM_ui_canvas_fill_rect/0/16` | argb8888 | 105.740 ns |
| `BM_ui_canvas_fill_rect/1/16` | rgb565 | 108.669 ns |
| `BM_ui_canvas_fill_rect/0/72` | argb8888 | 679.024 ns |
| `BM_ui_canvas_fill_rect/1/72` | rgb565 | 642.788 ns |
| `BM_ui_canvas_fill_rect/0/320` | argb8888 | 17727.656 ns |
| `BM_ui_canvas_fill_rect/1/320` | rgb565 | 9316.941 ns |
| `BM_ui_canvas_fill_rect_translucent/0` | argb8888 | 6576.220 ns |
| `BM_ui_canvas_fill_rect_translucent/1` | rgb565 | 6537.210 ns |
| `BM_ui_canvas_fill_round_rect/0/0` | argb8888 | 652.907 ns |
| `BM_ui_canvas_fill_round_rect/1/0` | rgb565 | 617.925 ns |
| `BM_ui_canvas_fill_round_rect/0/8` | argb8888 | 4375.886 ns |
| `BM_ui_canvas_fill_round_rect/1/8` | rgb565 | 5164.930 ns |
| `BM_ui_canvas_fill_round_rect/0/24` | argb8888 | 16127.366 ns |
| `BM_ui_canvas_fill_round_rect/1/24` | rgb565 | 15673.906 ns |
| `BM_ui_canvas_fill_box_gradient/0/0` | argb8888 | 10622.392 ns |
| `BM_ui_canvas_fill_box_gradient/1/0` | rgb565 | 13260.424 ns |
| `BM_ui_canvas_fill_box_gradient/0/1` | argb8888 | 41962.631 ns |
| `BM_ui_canvas_fill_box_gradient/1/1` | rgb565 | 46332.016 ns |
| `BM_ui_canvas_fill_box_gradient/0/2` | argb8888 | 60898.008 ns |
| `BM_ui_canvas_fill_box_gradient/1/2` | rgb565 | 65244.500 ns |
| `BM_ui_canvas_shadow/0/4` | argb8888 | 25112.857 ns |
| `BM_ui_canvas_shadow/1/4` | rgb565 | 25189.643 ns |
| `BM_ui_canvas_shadow/0/12` | argb8888 | 45164.970 ns |
| `BM_ui_canvas_shadow/1/12` | rgb565 | 44620.964 ns |
| `BM_ui_canvas_shadow/0/24` | argb8888 | 84728.486 ns |
| `BM_ui_canvas_shadow/1/24` | rgb565 | 94197.500 ns |
| `BM_ui_canvas_blur/0/2` | argb8888 | 1185444.000 ns |
| `BM_ui_canvas_blur/1/2` | rgb565 | 1037865.672 ns |
| `BM_ui_canvas_blur/0/8` | argb8888 | 1003047.761 ns |
| `BM_ui_canvas_blur/1/8` | rgb565 | 1109635.821 ns |
| `BM_ui_canvas_glass/0` | argb8888 | 1321516.000 ns |
| `BM_ui_canvas_glass/1` | rgb565 | 1443386.667 ns |
| `BM_ui_canvas_push_pop` |  | 53.364 ns |
| `BM_ui_text_draw/0/0` | argb8888 | 3168.632 ns |
| `BM_ui_text_draw/1/0` | rgb565 | 2995.187 ns |
| `BM_ui_text_draw/0/1` | argb8888 | 16709.196 ns |
| `BM_ui_text_draw/1/1` | rgb565 | 15344.598 ns |
| `BM_ui_text_draw_cyrillic` |  | 5617.210 ns |
| `BM_ui_canvas_fill_glyph/0/0` | argb8888 | 435.757 ns |
| `BM_ui_canvas_fill_glyph/1/0` | rgb565 | 384.714 ns |
| `BM_ui_canvas_fill_glyph/0/1` | argb8888 | 473.681 ns |
| `BM_ui_canvas_fill_glyph/1/1` | rgb565 | 430.689 ns |
| `BM_ui_text_draw_wrapped/0` | argb8888 | 83425.100 ns |
| `BM_ui_text_draw_wrapped/1` | rgb565 | 77765.700 ns |
| `BM_ui_text_get_size` |  | 452.076 ns |
| `BM_ui_text_get_size_wrapped` |  | 2283.806 ns |
| `BM_ui_text_line_measure` |  | 164.718 ns |
| `BM_ui_text_get_line_width` |  | 165.996 ns |
| `BM_ui_font_get_glyph` |  | 1.702 ns |
| `BM_ui_text_next_code` |  | 22.726 ns |
| `BM_ui_pixmap_fill_span/0` | argb8888 | 82.160 ns |
| `BM_ui_pixmap_fill_span/1` | rgb565 | 41.109 ns |
| `BM_ui_pixmap_blend_span/0` | argb8888 | 555.231 ns |
| `BM_ui_pixmap_blend_span/1` | rgb565 | 550.964 ns |
| `BM_ui_pixmap_blend_alpha_span/0/0` | argb8888/opaque | 77.895 ns |
| `BM_ui_pixmap_blend_alpha_span/0/1` | argb8888/transparent | 132.790 ns |
| `BM_ui_pixmap_blend_alpha_span/1/0` | rgb565/opaque | 73.495 ns |
| `BM_ui_color_over` |  | 6.394 ns |
| `BM_ui_color_rgb565_roundtrip` |  | 5.976 ns |
| `BM_timer_start_stop` |  | 2.917 ns |
| `BM_timer_group_handler/1` |  | 4.260 ns |
| `BM_timer_group_handler/16` |  | 36.422 ns |
| `BM_timer_group_handler/256` |  | 392.206 ns |
| `BM_timer_group_until_next` |  | 151.307 ns |
| `BM_timestamp_to_datetime` |  | 113.315 ns |
| `BM_timestamp_from_datetime` |  | 7.276 ns |
| `BM_timestamp_get_date` |  | 111.192 ns |
| `BM_timestamp_get_fields` |  | 340.639 ns |
| `BM_timestamp_add_months` |  | 137.358 ns |
| `BM_timestamp_add_years` |  | 145.871 ns |
| `BM_logger_log_enabled` |  | 52.271 ns |
| `BM_logger_log_filtered` |  | 2.947 ns |
| `BM_logger_log_null_sink` |  | 3.244 ns |
| `BM_fs_path_set` |  | 121.426 ns |
| `BM_fs_path_join` |  | 45.534 ns |
| `BM_fs_path_parent` |  | 37.700 ns |
| `BM_fs_path_get_segment` |  | 27.281 ns |
| `BM_math_rect_contains_point` |  | 2.930 ns |
| `BM_math_rect_intersects` |  | 2.949 ns |
| `BM_math_rect_intersection` |  | 2.844 ns |
| `BM_math_rect_union` |  | 2.972 ns |
| `BM_bit_pack_unpack_be32` |  | 0.790 ns |
| `BM_bit_pack_unpack_le32` |  | 0.650 ns |
| `BM_bit_pack_unpack_be64` |  | 3.702 ns |
| `BM_bit_pack_unpack_le16` |  | 0.664 ns |
| `BM_bit_rotate_left_u32` |  | 0.790 ns |
| `BM_bit_rotate_right_u32` |  | 0.791 ns |
| `BM_bit_rotate_left_u64` |  | 3.420 ns |
| `BM_bit_rotate_right_u64` |  | 3.458 ns |
| `BM_bit_bswap_u16` |  | 0.645 ns |
| `BM_bit_bswap_u32` |  | 0.549 ns |
| `BM_bit_bswap_u64` |  | 1.618 ns |
| `BM_bit_scan_forward_u32` |  | 0.806 ns |
| `BM_bit_scan_reverse_u32` |  | 1.136 ns |
| `BM_bit_scan_forward_u64` |  | 2.118 ns |
| `BM_bit_scan_reverse_u64` |  | 2.387 ns |
| `BM_bit_scan_forward_u16` |  | 0.887 ns |
| `BM_bit_scan_reverse_u8` |  | 1.067 ns |
| `BM_memory_view_get_size` |  | 0.602 ns |
| `BM_memory_view_get_ptr_from_begin` |  | 0.562 ns |
| `BM_memory_view_compare_4KB_equal` |  | 163.500 ns |
| `BM_memory_view_find_byte` |  | 821.820 ns |
| `BM_memory_typed_allocated_resize_1000_ints` |  | 179.788 ns |
| `BM_memory_typed_get_size` |  | 3.426 ns |
| `BM_memory_typed_get_ptr_from_begin` |  | 0.792 ns |
| `BM_memory_typed_is_valid_index` |  | 3.883 ns |
| `BM_memory_typed_set_value` |  | 3.281 ns |
| `BM_memory_typed_set_values_64` |  | 13.114 ns |
| `BM_memory_bounds_allocated_resize_4KB` |  | 176.918 ns |
| `BM_memory_bounds_allocated_exchange` |  | 161.746 ns |
| `BM_memory_bounds_init_by_size` |  | 0.645 ns |
| `BM_memory_bounds_get_size` |  | 0.608 ns |
| `BM_memory_bounds_get_ptr_from_begin` |  | 0.569 ns |
| `BM_memory_bounds_is_valid` |  | 0.797 ns |
| `BM_memory_bounds_copy_4KB` |  | 91.927 ns |
| `BM_memory_bounds_compare_4KB_equal` |  | 165.374 ns |
| `BM_memory_bounds_find_byte` |  | 826.469 ns |
| `BM_runtime_allocator_alloc_free_64B` |  | 77.466 ns |
| `BM_runtime_allocator_realloc_grow` |  | 291.111 ns |
| `BM_io_reader_read_4KB` |  | 114.388 ns |
| `BM_io_writer_write_4KB` |  | 120.529 ns |
| `BM_io_reader_read_1KB_in_64B_chunks` |  | 299.491 ns |
| `BM_mb_crc16/8` |  | 71.210 ns |
| `BM_mb_crc16/64` |  | 588.569 ns |
| `BM_mb_crc16/256` |  | 2348.142 ns |
| `BM_mb_build_read` |  | 55.135 ns |
| `BM_mb_build_write` |  | 214.223 ns |
| `BM_mb_plan_reads` |  | 15.578 ns |
| `BM_net_ip4_socket_addr_parse` |  | 50.379 ns |
| `BM_net_ip4_socket_addr_format` |  | 36.680 ns |
| `BM_net_socket_addr_format_dispatch` |  | 36.318 ns |
| `BM_net_ip4_socket_addr_equals` |  | 7.114 ns |
| `BM_net_port_parse` |  | 3.079 ns |
| `BM_net_ip4_parse` |  | 33.975 ns |
| `BM_net_ip4_format` |  | 24.558 ns |
| `BM_net_ip4_equals` |  | 7.207 ns |
| `BM_wstr_view_init` |  | 398.829 ns |
| `BM_wstr_view_from_offset` |  | 0.311 ns |
| `BM_wstr_view_tail` |  | 1.616 ns |
| `BM_wstr_view_get_size` |  | 0.305 ns |
| `BM_wstr_ptr_len` |  | 399.122 ns |
| `BM_wstr_ptr_get_size` |  | 0.304 ns |
| `BM_wstr_ptr_find` |  | 40.818 ns |
| `BM_wstr_ptr_equals` |  | 796.536 ns |
| `BM_wstr_ptr_to_lower_45B` |  | 47.221 ns |
| `BM_str_ptr_split_next_slash` |  | 87.029 ns |
| `BM_str_ptr_split_next_of_sep` |  | 54.477 ns |
| `BM_str_view_split_next_of_sep` |  | 57.856 ns |
| `BM_str_ptr_len` |  | 10.702 ns |
| `BM_str_ptr_get_size` |  | 0.303 ns |
| `BM_str_ptr_find` |  | 18.194 ns |
| `BM_str_ptr_equals` |  | 24.640 ns |
| `BM_str_ptr_to_lower_45B` |  | 19.366 ns |
| `BM_str_ptr_trim` |  | 23.130 ns |
| `BM_str_ptr_find_of_char/64` |  | 18.364 ns |
| `BM_str_ptr_find_of_char/256` |  | 66.529 ns |
| `BM_str_ptr_find_of_char/4096` |  | 826.808 ns |
| `BM_memory_find_byte/64` |  | 18.291 ns |
| `BM_memory_find_byte/256` |  | 63.165 ns |
| `BM_memory_find_byte/4096` |  | 826.621 ns |
| `BM_crt_memchr/64` |  | 17.092 ns |
| `BM_crt_memchr/256` |  | 62.668 ns |
| `BM_crt_memchr/4096` |  | 840.956 ns |
| `BM_str_ptr_find_of_chars_sep/64` |  | 56.832 ns |
| `BM_str_ptr_find_of_chars_sep/256` |  | 212.065 ns |
| `BM_str_ptr_find_of_chars_sep/4096` |  | 3275.710 ns |
| `BM_str_view_find_char` |  | 14.083 ns |
| `BM_str_view_find` |  | 18.329 ns |
| `BM_str_view_equals` |  | 8.680 ns |
| `BM_str_view_compare` |  | 7.158 ns |
| `BM_crypto_rijndael_aes128_ecb_encrypt` |  | 11.618 ns |
| `BM_crypto_rijndael_aes128_ecb_decrypt` |  | 12.761 ns |
| `BM_crypto_rijndael_aes256_ecb_encrypt` |  | 18.247 ns |
| `BM_crypto_rijndael_aes128_cbc_encrypt` |  | 761.558 ns |
| `BM_crypto_rijndael_aes128_ecb_init` |  | 95.812 ns |
| `BM_crypto_rijndael_aes256_ecb_init` |  | 117.279 ns |
| `BM_crypto_sha_hash/0/0` | sha1 | 227.714 ns |
| `BM_crypto_sha_hash/1/0` | sha224 | 364.933 ns |
| `BM_crypto_sha_hash/2/0` | sha256 | 368.197 ns |
| `BM_crypto_sha_hash/3/0` | sha384 | 1041.144 ns |
| `BM_crypto_sha_hash/4/0` | sha512 | 1062.920 ns |
| `BM_crypto_sha_hash/0/64` | sha1 | 446.361 ns |
| `BM_crypto_sha_hash/1/64` | sha224 | 706.048 ns |
| `BM_crypto_sha_hash/2/64` | sha256 | 698.643 ns |
| `BM_crypto_sha_hash/3/64` | sha384 | 1032.240 ns |
| `BM_crypto_sha_hash/4/64` | sha512 | 1028.044 ns |
| `BM_crypto_sha_hash/0/1024` | sha1 | 3502.175 ns |
| `BM_crypto_sha_hash/1/1024` | sha224 | 5662.570 ns |
| `BM_crypto_sha_hash/2/1024` | sha256 | 5746.966 ns |
| `BM_crypto_sha_hash/3/1024` | sha384 | 9166.500 ns |
| `BM_crypto_sha_hash/4/1024` | sha512 | 9106.029 ns |
| `BM_crypto_sha_hash/0/4096` | sha1 | 13387.188 ns |
| `BM_crypto_sha_hash/1/4096` | sha224 | 21370.737 ns |
| `BM_crypto_sha_hash/2/4096` | sha256 | 21839.398 ns |
| `BM_crypto_sha_hash/3/4096` | sha384 | 32701.205 ns |
| `BM_crypto_sha_hash/4/4096` | sha512 | 32898.455 ns |
| `BM_crypto_sha_hash/0/65536` | sha1 | 212600.896 ns |
| `BM_crypto_sha_hash/1/65536` | sha224 | 340838.614 ns |
| `BM_crypto_sha_hash/2/65536` | sha256 | 338934.375 ns |
| `BM_crypto_sha_hash/3/65536` | sha384 | 511150.000 ns |
| `BM_crypto_sha_hash/4/65536` | sha512 | 511354.000 ns |
| `BM_crypto_sha_hash/0/1048576` | sha1 | 3361285.000 ns |
| `BM_crypto_sha_hash/1/1048576` | sha224 | 5504750.000 ns |
| `BM_crypto_sha_hash/2/1048576` | sha256 | 5410920.000 ns |
| `BM_crypto_sha_hash/3/1048576` | sha384 | 8278180.000 ns |
| `BM_crypto_sha_hash/4/1048576` | sha512 | 8395244.445 ns |
| `BM_crypto_sha_update_finish/0/64` | sha1 | 438.108 ns |
| `BM_crypto_sha_update_finish/2/64` | sha256 | 712.625 ns |
| `BM_crypto_sha_update_finish/4/64` | sha512 | 1053.355 ns |
| `BM_crypto_sha_update_finish/0/4096` | sha1 | 14046.719 ns |
| `BM_crypto_sha_update_finish/2/4096` | sha256 | 21438.036 ns |
| `BM_crypto_sha_update_finish/4/4096` | sha512 | 32567.892 ns |
| `BM_crypto_sha_update_finish/0/65536` | sha1 | 220855.224 ns |
| `BM_crypto_sha_update_finish/2/65536` | sha256 | 336873.154 ns |
| `BM_crypto_sha_update_finish/4/65536` | sha512 | 505944.000 ns |
| `BM_str_format_bytes_hex_32` |  | 4.808 ns |
| `BM_datetime_parse` |  | 78.910 ns |
| `BM_datetime_format` |  | 154.526 ns |
| `BM_time_parse` |  | 24.467 ns |
| `BM_time_format` |  | 73.804 ns |
| `BM_time_add_second` |  | 18.677 ns |
| `BM_date_parse` |  | 29.456 ns |
| `BM_date_format` |  | 72.102 ns |
| `BM_date_add_day` |  | 84.437 ns |
| `BM_date_add_day_month_roll` |  | 84.309 ns |
| `BM_wstr_ptr_get_size_desc` |  | 0.320 ns |
| `BM_wstr_ptr_len_desc` |  | 171.244 ns |
| `BM_werror_make_lit` |  | 0.597 ns |
| `BM_werror_make_by_code` |  | 0.608 ns |
| `BM_wstr_view_lit` |  | 0.541 ns |
| `BM_raw_ptr_werror_store` |  | 0.668 ns |
| `BM_view_werror_store_lit` |  | 0.793 ns |
| `BM_werror_init_lit` |  | 0.534 ns |
| `BM_werror_make_empty` |  | 0.536 ns |
| `BM_str_ptr_get_size_desc` |  | 0.303 ns |
| `BM_str_ptr_len_desc` |  | 3.501 ns |
| `BM_error_make_lit` |  | 0.532 ns |
| `BM_error_make_by_code` |  | 0.534 ns |
| `BM_runtime_error_make_lit` |  | 0.535 ns |
| `BM_str_view_lit` |  | 0.581 ns |
| `BM_raw_ptr_error_store` |  | 0.630 ns |
| `BM_view_error_store_lit` |  | 0.877 ns |
| `BM_error_init_lit` |  | 0.528 ns |
| `BM_error_make_empty` |  | 0.530 ns |
| `BM_raw_ptr_origin_store` |  | 0.800 ns |
| `BM_str_parse_hex_2_digits` |  | 2.733 ns |
| `BM_str_parse_hex_8_digits` |  | 8.442 ns |
| `BM_str_parse_uint_1_digit` |  | 1.121 ns |
| `BM_str_parse_uint_10_digits` |  | 5.787 ns |
| `BM_str_format_text_literal` |  | 15.255 ns |
| `BM_str_format_text_literal_long` |  | 67.512 ns |
| `BM_str_format_text_mixed_conversions` |  | 94.140 ns |
| `BM_str_format_hex_1_digit` |  | 3.270 ns |
| `BM_str_format_hex_8_digits` |  | 8.022 ns |
| `BM_str_format_sint_1_digit` |  | 2.542 ns |
| `BM_str_format_sint_10_digits` |  | 15.932 ns |
| `BM_str_format_uint_1_digit` |  | 2.433 ns |
| `BM_str_format_uint_10_digits` |  | 15.094 ns |
| `BM_str_append_grow` |  | 9409.680 ns |
| `BM_str_push_back_grow` |  | 8743.260 ns |
| `BM_str_reserve_append` |  | 7242.880 ns |
| `BM_str_clear_and_reuse` |  | 10.568 ns |
| `BM_list_push_back` |  | 2262.432 ns |
| `BM_list_push_front` |  | 2370.980 ns |
| `BM_list_traverse_forward` |  | 1065.018 ns |
| `BM_list_sort/100` |  | 2744.286 ns |
| `BM_list_sort/1000` |  | 26893.244 ns |
| `BM_list_sort/10000` |  | 487120.134 ns |
| `BM_list_insert_sorted/100` |  | 3539.095 ns |
| `BM_list_insert_sorted/1000` |  | 276174.107 ns |
| `BM_list_pop_front_drain` |  | 4432.753 ns |
| `BM_array_push_back_grow` |  | 6287.040 ns |
| `BM_array_push_back_reserved` |  | 5221.154 ns |
| `BM_array_get_ptr_random_access` |  | 1.469 ns |
| `BM_array_insert_front` |  | 66246.215 ns |
| `BM_array_erase_front` |  | 53910.583 ns |
| `BM_array_sort/100` |  | 4163.785 ns |
| `BM_array_sort/1000` |  | 48070.603 ns |
| `BM_array_sort/10000` |  | 635628.713 ns |
| `BM_memory_std_copy/64` |  | 8.230 ns |
| `BM_memory_std_copy/128` |  | 10.541 ns |
| `BM_memory_std_copy/256` |  | 12.299 ns |
| `BM_memory_std_copy/512` |  | 21.212 ns |
| `BM_memory_std_copy/1024` |  | 33.729 ns |
| `BM_memory_std_copy/2048` |  | 46.693 ns |
| `BM_memory_std_copy/4096` |  | 98.013 ns |
| `BM_memory_std_copy/8192` |  | 163.387 ns |
| `BM_memory_std_copy/65536` |  | 2641.183 ns |
| `BM_memory_std_copy/262144` |  | 10963.767 ns |
| `BM_memory_std_copy/1048576` |  | 50072.845 ns |
| `BM_memory_std_copy/2097152` |  | 106631.800 ns |
| `BM_memory_std_copy/4194304` |  | 416141.518 ns |
| `BM_memory_std_copy/8388608` |  | 909371.642 ns |
| `BM_memory_std_copy/16777216` |  | 1995973.333 ns |
| `BM_memory_std_rcopy/64` |  | 7.063 ns |
| `BM_memory_std_rcopy/4096` |  | 110.068 ns |
| `BM_memory_std_rcopy/65536` |  | 3163.674 ns |
| `BM_memory_std_rcopy/1048576` |  | 83513.100 ns |
| `BM_memory_std_copy_rev/64` |  | 7.552 ns |
| `BM_memory_std_copy_rev/4096` |  | 112.874 ns |
| `BM_memory_std_copy_rev/65536` |  | 2451.036 ns |
| `BM_memory_std_copy_rev/1048576` |  | 59146.100 ns |
| `BM_crt_memcpy/64` |  | 13.718 ns |
| `BM_crt_memcpy/128` |  | 18.227 ns |
| `BM_crt_memcpy/256` |  | 28.281 ns |
| `BM_crt_memcpy/512` |  | 30.297 ns |
| `BM_crt_memcpy/1024` |  | 38.128 ns |
| `BM_crt_memcpy/2048` |  | 57.510 ns |
| `BM_crt_memcpy/4096` |  | 305.453 ns |
| `BM_crt_memcpy/8192` |  | 176.925 ns |
| `BM_crt_memcpy/65536` |  | 5977.366 ns |
| `BM_crt_memcpy/262144` |  | 26568.906 ns |
| `BM_crt_memcpy/1048576` |  | 58907.405 ns |
| `BM_crt_memcpy/2097152` |  | 127305.232 ns |
| `BM_crt_memcpy/4194304` |  | 360960.268 ns |
| `BM_crt_memcpy/8388608` |  | 1271386.667 ns |
| `BM_crt_memcpy/16777216` |  | 2707766.667 ns |
| `BM_memory_std_set/64` |  | 8.635 ns |
| `BM_memory_std_set/128` |  | 9.708 ns |
| `BM_memory_std_set/256` |  | 12.188 ns |
| `BM_memory_std_set/512` |  | 16.554 ns |
| `BM_memory_std_set/1024` |  | 25.605 ns |
| `BM_memory_std_set/2048` |  | 45.366 ns |
| `BM_memory_std_set/4096` |  | 78.905 ns |
| `BM_memory_std_set/8192` |  | 147.278 ns |
| `BM_memory_std_set/65536` |  | 1693.618 ns |
| `BM_memory_std_set/262144` |  | 6902.650 ns |
| `BM_memory_std_set/1048576` |  | 38354.170 ns |
| `BM_memory_std_set/2097152` |  | 78658.566 ns |
| `BM_memory_std_set/4194304` |  | 160025.893 ns |
| `BM_memory_std_set/8388608` |  | 456618.792 ns |
| `BM_memory_std_set/16777216` |  | 886670.149 ns |
| `BM_crt_memset/64` |  | 13.462 ns |
| `BM_crt_memset/128` |  | 17.988 ns |
| `BM_crt_memset/256` |  | 26.119 ns |
| `BM_crt_memset/512` |  | 26.694 ns |
| `BM_crt_memset/1024` |  | 38.410 ns |
| `BM_crt_memset/2048` |  | 63.488 ns |
| `BM_crt_memset/4096` |  | 117.117 ns |
| `BM_crt_memset/8192` |  | 217.704 ns |
| `BM_crt_memset/65536` |  | 1710.973 ns |
| `BM_crt_memset/262144` |  | 7063.757 ns |
| `BM_crt_memset/1048576` |  | 30314.150 ns |
| `BM_crt_memset/2097152` |  | 61818.500 ns |
| `BM_crt_memset/4194304` |  | 122650.000 ns |
| `BM_crt_memset/8388608` |  | 256418.750 ns |
| `BM_crt_memset/16777216` |  | 604200.671 ns |
| `BM_memory_std_compare_equal/4096` |  | 164.646 ns |
| `BM_memory_std_compare_equal/65536` |  | 2866.580 ns |
| `BM_memory_std_compare_equal/1048576` |  | 56991.330 ns |
| `BM_crt_memcmp/4096` |  | 564.587 ns |
| `BM_crt_memcmp/65536` |  | 8883.328 ns |
| `BM_crt_memcmp/1048576` |  | 142909.375 ns |
| `BM_memory_std_compare_mismatch_at_start/4096` |  | 4.275 ns |
| `BM_memory_std_rcompare_equal/4096` |  | 157.385 ns |
| `BM_memory_std_rcompare_mismatch_at_end/4096` |  | 4.333 ns |
| `BM_memory_std_find/64` |  | 22.920 ns |
| `BM_memory_std_find/1024` |  | 94.578 ns |
| `BM_memory_std_find/65536` |  | 4454.071 ns |
| `BM_memory_find_substring/64` |  | 22.658 ns |
| `BM_memory_find_substring/1024` |  | 94.490 ns |
| `BM_memory_find_substring/65536` |  | 4421.282 ns |
| `BM_memory_set_pattern/64/2` |  | 13.649 ns |
| `BM_memory_set_pattern/4096/2` |  | 89.277 ns |
| `BM_memory_set_pattern/64/4` |  | 13.402 ns |
| `BM_memory_set_pattern/4096/4` |  | 117.007 ns |
| `BM_memory_set_pattern/64/8` |  | 25.543 ns |
| `BM_memory_set_pattern/4096/8` |  | 161.020 ns |
| `BM_memory_std_move/8/0` |  | 5.411 ns |
| `BM_memory_std_move/16/0` |  | 7.839 ns |
| `BM_memory_std_move/32/0` |  | 18.794 ns |
| `BM_memory_std_move/64/0` |  | 11.262 ns |
| `BM_memory_std_move/128/0` |  | 11.061 ns |
| `BM_memory_std_move/256/0` |  | 12.176 ns |
| `BM_memory_std_move/4096/0` |  | 129.954 ns |
| `BM_memory_std_move/8/1` |  | 5.467 ns |
| `BM_memory_std_move/16/1` |  | 6.217 ns |
| `BM_memory_std_move/32/1` |  | 6.360 ns |
| `BM_memory_std_move/64/1` |  | 7.720 ns |
| `BM_memory_std_move/128/1` |  | 8.879 ns |
| `BM_memory_std_move/256/1` |  | 16.762 ns |
| `BM_memory_std_move/4096/1` |  | 125.143 ns |
| `BM_memory_std_xor/16` |  | 9.142 ns |
| `BM_memory_std_xor/64` |  | 8.639 ns |
| `BM_memory_std_xor/256` |  | 16.663 ns |
| `BM_memory_std_xor/1024` |  | 37.697 ns |
| `BM_memory_std_xor/4096` |  | 177.994 ns |
| `BM_memory_std_xor/65536` |  | 5479.796 ns |
| `BM_memory_std_xor/1048576` |  | 84830.677 ns |
| `BM_memory_std_copy_words/64` |  | 5.676 ns |
| `BM_memory_std_copy_words/4096` |  | 144.550 ns |
| `BM_memory_std_copy_words/65536` |  | 2610.290 ns |
| `BM_memory_std_copy_words/1048576` |  | 59318.479 ns |
| `BM_memory_std_fill_words/64` |  | 14.724 ns |
| `BM_memory_std_fill_words/4096` |  | 121.413 ns |
| `BM_memory_std_fill_words/65536` |  | 1946.859 ns |
| `BM_memory_std_fill_words/1048576` |  | 30000.893 ns |

## Reproduction and build repairs

Build: `cmake -S lib/lh -B lib/lh/build-windows-mingw32 -DLH_BUILD_BENCH=ON` then `cmake --build lib/lh/build-windows-mingw32 --target lh_bench -j 8`.
Full run: `lh_bench.exe --benchmark_min_time=0.05s --benchmark_repetitions=3 --benchmark_report_aggregates_only=true --benchmark_out=release-results.json --benchmark_out_format=json`.
Focused run additionally uses `--benchmark_min_time=0.2s --benchmark_repetitions=5 --benchmark_enable_random_interleaving=true` and a filter selecting the 21 case names listed above.
The pinned Google Benchmark submodule was initialized. Bench sources for wide views, rect, timestamp, filesystem path and text/color were updated to the current out-parameter API; Modbus gets its size-type header; web frame validation reads through the existing buffer accessor instead of removed `web.len`. Library implementation and optimization settings were not changed for these repairs.

Raw JSON, console logs, median CSV and executable remain in `lib/lh/build-windows-mingw32/`.

The layout checker now excludes `lib/lh/bench/lib/` (third-party benchmark source), as it already excludes other vendored dependencies; initializing the pinned benchmark had exposed 9 external formatting findings.
