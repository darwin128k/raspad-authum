#ifndef AUTHUM_HOST_LOGS_FILES_H
#define AUTHUM_HOST_LOGS_FILES_H
#include <authum/host/logs/writer.h>
void
authum_logs_files_set_root(const authum_path_t *dir);
void
authum_logs_files_dropped(authum_u32_t count);
void
authum_logs_files_emit(const authum_logs_record_t *record);
void
authum_logs_files_flush(void);
void
authum_logs_files_close(void);
#endif
