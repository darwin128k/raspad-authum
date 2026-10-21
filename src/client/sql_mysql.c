/**
 * MySQL/MariaDB backend — loaded at runtime (dlopen / LoadLibrary).
 * No link dependency on libmysqlclient; the host must ship a client .so/.dll.
 */

#include "sql_internal.h"

#include <authum/cstr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

typedef struct st_mysql MYSQL;
typedef struct st_mysql_res MYSQL_RES;
typedef char **MYSQL_ROW;

typedef MYSQL *(*authum_mysql_init_fn)(MYSQL *);
typedef MYSQL *(*authum_mysql_real_connect_fn)(MYSQL *, const char *, const char *, const char *,
					      const char *, unsigned int, const char *,
					      unsigned long);
typedef void (*authum_mysql_close_fn)(MYSQL *);
typedef const char *(*authum_mysql_error_fn)(MYSQL *);
typedef int (*authum_mysql_query_fn)(MYSQL *, const char *);
typedef MYSQL_RES *(*authum_mysql_store_result_fn)(MYSQL *);
typedef void (*authum_mysql_free_result_fn)(MYSQL_RES *);
typedef MYSQL_ROW (*authum_mysql_fetch_row_fn)(MYSQL_RES *);
typedef unsigned long long (*authum_mysql_insert_id_fn)(MYSQL *);
typedef unsigned long (*authum_mysql_real_escape_string_fn)(MYSQL *, char *, const char *,
							   unsigned long);

struct authum_sql_mysql_api
{
	void *lib;
	authum_mysql_init_fn init;
	authum_mysql_real_connect_fn real_connect;
	authum_mysql_close_fn close;
	authum_mysql_error_fn error;
	authum_mysql_query_fn query;
	authum_mysql_store_result_fn store_result;
	authum_mysql_free_result_fn free_result;
	authum_mysql_fetch_row_fn fetch_row;
	authum_mysql_insert_id_fn insert_id;
	authum_mysql_real_escape_string_fn real_escape_string;
};

enum
{
	authum_sql_mysql_bind_max = 32,
	authum_sql_mysql_text_max = 256,
	authum_sql_mysql_sql_max = 2048
};

enum authum_sql_mysql_bind_kind
{
	authum_sql_mysql_bind_none = 0,
	authum_sql_mysql_bind_i32 = 1,
	authum_sql_mysql_bind_cstr = 2
};

struct authum_sql_mysql_bind
{
	enum authum_sql_mysql_bind_kind kind;
	authum_i32_t i32;
	authum_char_t text[authum_sql_mysql_text_max];
};

struct authum_sql_mysql_stmt
{
	authum_char_t sql[authum_sql_mysql_sql_max];
	struct authum_sql_mysql_bind binds[authum_sql_mysql_bind_max];
	MYSQL_RES *res;
	MYSQL_ROW row;
	authum_bool_t executed;
};

static struct authum_sql_mysql_api g_mysql_api;
static authum_bool_t g_mysql_tried;
static authum_bool_t g_mysql_ok;

static void *
authum_sql_mysql_dlsym(void *lib, const authum_char_t *name)
{
#if defined(_WIN32)
	return (void *)GetProcAddress((HMODULE)lib, name);
#else
	return dlsym(lib, name);
#endif
}

static void *
authum_sql_mysql_dlopen(const authum_char_t *path)
{
#if defined(_WIN32)
	return (void *)LoadLibraryA(path);
#else
	return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

static authum_bool_t
authum_sql_mysql_load(void)
{
	static const authum_char_t *names[] = {
#if defined(_WIN32)
		"libmariadb.dll",
		"libmysql.dll",
		"libmysqlclient.dll",
#else
		"libmariadb.so.3",
		"libmariadb.so",
		"libmysqlclient.so.21",
		"libmysqlclient.so.20",
		"libmysqlclient.so.18",
		"libmysqlclient.so",
#endif
		authum_null
	};
	authum_usize_t i;

	if (g_mysql_tried)
	{
		return g_mysql_ok;
	}
	g_mysql_tried = authum_bool_true;

	for (i = 0; names[i] != authum_null; ++i)
	{
		void *lib = authum_sql_mysql_dlopen(names[i]);
		if (authum_null_eq(lib))
		{
			continue;
		}
		g_mysql_api.lib = lib;
		g_mysql_api.init = (authum_mysql_init_fn)authum_sql_mysql_dlsym(lib, "mysql_init");
		g_mysql_api.real_connect =
			(authum_mysql_real_connect_fn)authum_sql_mysql_dlsym(lib, "mysql_real_connect");
		g_mysql_api.close = (authum_mysql_close_fn)authum_sql_mysql_dlsym(lib, "mysql_close");
		g_mysql_api.error = (authum_mysql_error_fn)authum_sql_mysql_dlsym(lib, "mysql_error");
		g_mysql_api.query = (authum_mysql_query_fn)authum_sql_mysql_dlsym(lib, "mysql_query");
		g_mysql_api.store_result =
			(authum_mysql_store_result_fn)authum_sql_mysql_dlsym(lib, "mysql_store_result");
		g_mysql_api.free_result =
			(authum_mysql_free_result_fn)authum_sql_mysql_dlsym(lib, "mysql_free_result");
		g_mysql_api.fetch_row =
			(authum_mysql_fetch_row_fn)authum_sql_mysql_dlsym(lib, "mysql_fetch_row");
		g_mysql_api.insert_id =
			(authum_mysql_insert_id_fn)authum_sql_mysql_dlsym(lib, "mysql_insert_id");
		g_mysql_api.real_escape_string = (authum_mysql_real_escape_string_fn)authum_sql_mysql_dlsym(
			lib, "mysql_real_escape_string");
		if (authum_null_eq(g_mysql_api.init) || authum_null_eq(g_mysql_api.real_connect) ||
		    authum_null_eq(g_mysql_api.close) || authum_null_eq(g_mysql_api.error) ||
		    authum_null_eq(g_mysql_api.query) || authum_null_eq(g_mysql_api.store_result) ||
		    authum_null_eq(g_mysql_api.free_result) || authum_null_eq(g_mysql_api.fetch_row) ||
		    authum_null_eq(g_mysql_api.insert_id) ||
		    authum_null_eq(g_mysql_api.real_escape_string))
		{
			g_mysql_api.lib = authum_null;
			continue;
		}
		g_mysql_ok = authum_bool_true;
		return authum_bool_true;
	}
	return authum_bool_false;
}

authum_bool_t
authum_sql_mysql_available(void)
{
	return authum_sql_mysql_load();
}

authum_bool_t
authum_sql_open_mysql(authum_sql_t *self, const authum_char_t *host, const authum_char_t *user,
		      const authum_char_t *pass, const authum_char_t *db, authum_u32_t port)
{
	MYSQL *mysql;

	if (authum_null_eq(self) || authum_null_eq(host) || authum_null_eq(user) ||
	    authum_null_eq(db))
	{
		return authum_bool_false;
	}

	authum_sql_close(self);
	authum_sql_init(self);
	self->driver = authum_sql_driver_mysql;

	if (!authum_sql_mysql_load())
	{
		authum_sql_set_errmsg(self, "MySQL client library not found (dlopen)");
		return authum_bool_false;
	}

	mysql = g_mysql_api.init(authum_null);
	if (authum_null_eq(mysql))
	{
		authum_sql_set_errmsg(self, "mysql_init failed");
		return authum_bool_false;
	}

	if (port == 0u)
	{
		port = 3306u;
	}

	if (authum_null_eq(g_mysql_api.real_connect(mysql, host, user,
						    authum_null_eq(pass) ? "" : pass, db,
						    (unsigned int)port, authum_null, 0ul)))
	{
		authum_sql_set_errmsg(self, g_mysql_api.error(mysql));
		g_mysql_api.close(mysql);
		return authum_bool_false;
	}

	self->db = mysql;
	self->errmsg[0] = '\0';
	return authum_bool_true;
}

void
authum_sql_mysql_close(authum_sql_t *self)
{
	if (authum_null_eq(self) || authum_null_eq(self->db) || !g_mysql_ok)
	{
		return;
	}
	g_mysql_api.close((MYSQL *)self->db);
	self->db = authum_null;
}

authum_bool_t
authum_sql_mysql_exec(authum_sql_t *self, const authum_char_t *sql)
{
	if (!authum_sql_is_open(self) || authum_null_eq(sql) || !g_mysql_ok)
	{
		return authum_bool_false;
	}
	if (g_mysql_api.query((MYSQL *)self->db, sql) != 0)
	{
		authum_sql_set_errmsg(self, g_mysql_api.error((MYSQL *)self->db));
		return authum_bool_false;
	}
	self->errmsg[0] = '\0';
	return authum_bool_true;
}

authum_i64_t
authum_sql_mysql_last_insert_id(authum_sql_t *self)
{
	if (!authum_sql_is_open(self) || !g_mysql_ok)
	{
		return 0;
	}
	return (authum_i64_t)g_mysql_api.insert_id((MYSQL *)self->db);
}

static authum_bool_t
authum_sql_mysql_build_query(authum_sql_t *self, struct authum_sql_mysql_stmt *ms,
			     authum_char_t *out, authum_usize_t out_size)
{
	const authum_char_t *p = ms->sql;
	authum_usize_t o = 0u;
	authum_i32_t bi = 0;

	while (*p)
	{
		if (*p != '?')
		{
			if (o + 1u >= out_size)
			{
				return authum_bool_false;
			}
			out[o++] = *p++;
			continue;
		}
		++p;
		if (bi >= authum_sql_mysql_bind_max)
		{
			return authum_bool_false;
		}
		if (ms->binds[bi].kind == authum_sql_mysql_bind_i32)
		{
			authum_char_t num[16];
			authum_usize_t n;
			snprintf(num, sizeof num, "%d", (int)ms->binds[bi].i32);
			n = (authum_usize_t)strlen(num);
			if (o + n >= out_size)
			{
				return authum_bool_false;
			}
			memcpy(out + o, num, n);
			o += n;
		}
		else if (ms->binds[bi].kind == authum_sql_mysql_bind_cstr)
		{
			authum_char_t esc[authum_sql_mysql_text_max * 2u + 1u];
			authum_usize_t in_len = (authum_usize_t)strlen(ms->binds[bi].text);
			unsigned long esc_len;
			if (o + 2u >= out_size)
			{
				return authum_bool_false;
			}
			out[o++] = '\'';
			esc_len = g_mysql_api.real_escape_string((MYSQL *)self->db, esc,
								ms->binds[bi].text,
								(unsigned long)in_len);
			if (o + (authum_usize_t)esc_len + 1u >= out_size)
			{
				return authum_bool_false;
			}
			memcpy(out + o, esc, (authum_usize_t)esc_len);
			o += (authum_usize_t)esc_len;
			out[o++] = '\'';
		}
		else
		{
			authum_sql_set_errmsg(self, "mysql: unbound ?");
			return authum_bool_false;
		}
		++bi;
	}
	out[o] = '\0';
	return authum_bool_true;
}

authum_bool_t
authum_sql_mysql_prepare(authum_sql_t *self, authum_sql_stmt_t *stmt, const authum_char_t *sql)
{
	struct authum_sql_mysql_stmt *ms;

	if (authum_null_eq(stmt) || authum_null_eq(sql) || !authum_sql_is_open(self) || !g_mysql_ok)
	{
		return authum_bool_false;
	}

	ms = (struct authum_sql_mysql_stmt *)calloc(1, sizeof *ms);
	if (authum_null_eq(ms))
	{
		return authum_bool_false;
	}
	if (!authum_cstr_copy(ms->sql, sizeof ms->sql, sql))
	{
		free(ms);
		return authum_bool_false;
	}
	stmt->sql = self;
	stmt->stmt = ms;
	return authum_bool_true;
}

void
authum_sql_mysql_stmt_finalize(authum_sql_stmt_t *stmt)
{
	struct authum_sql_mysql_stmt *ms;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt))
	{
		return;
	}
	ms = (struct authum_sql_mysql_stmt *)stmt->stmt;
	if (!authum_null_eq(ms->res) && g_mysql_ok)
	{
		g_mysql_api.free_result(ms->res);
	}
	free(ms);
	stmt->stmt = authum_null;
	stmt->sql = authum_null;
}

authum_bool_t
authum_sql_mysql_stmt_bind_i32(authum_sql_stmt_t *stmt, authum_i32_t index, authum_i32_t value)
{
	struct authum_sql_mysql_stmt *ms;
	authum_i32_t i;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || index < 1 ||
	    index > authum_sql_mysql_bind_max)
	{
		return authum_bool_false;
	}
	ms = (struct authum_sql_mysql_stmt *)stmt->stmt;
	i = index - 1;
	ms->binds[i].kind = authum_sql_mysql_bind_i32;
	ms->binds[i].i32 = value;
	return authum_bool_true;
}

authum_bool_t
authum_sql_mysql_stmt_bind_cstr(authum_sql_stmt_t *stmt, authum_i32_t index,
				const authum_char_t *text)
{
	struct authum_sql_mysql_stmt *ms;
	authum_i32_t i;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || index < 1 ||
	    index > authum_sql_mysql_bind_max)
	{
		return authum_bool_false;
	}
	ms = (struct authum_sql_mysql_stmt *)stmt->stmt;
	i = index - 1;
	ms->binds[i].kind = authum_sql_mysql_bind_cstr;
	authum_cstr_copy(ms->binds[i].text, sizeof ms->binds[i].text,
			 authum_null_eq(text) ? "" : text);
	return authum_bool_true;
}

static authum_bool_t
authum_sql_mysql_stmt_execute(authum_sql_stmt_t *stmt, authum_bool_t want_result)
{
	struct authum_sql_mysql_stmt *ms;
	authum_char_t query[authum_sql_mysql_sql_max * 2u];
	authum_sql_t *self;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || authum_null_eq(stmt->sql) ||
	    !g_mysql_ok)
	{
		return authum_bool_false;
	}
	ms = (struct authum_sql_mysql_stmt *)stmt->stmt;
	self = stmt->sql;

	if (!ms->executed)
	{
		if (!authum_sql_mysql_build_query(self, ms, query, sizeof query))
		{
			return authum_bool_false;
		}
		if (g_mysql_api.query((MYSQL *)self->db, query) != 0)
		{
			authum_sql_set_errmsg(self, g_mysql_api.error((MYSQL *)self->db));
			return authum_bool_false;
		}
		if (want_result)
		{
			ms->res = g_mysql_api.store_result((MYSQL *)self->db);
		}
		ms->executed = authum_bool_true;
		self->errmsg[0] = '\0';
	}
	return authum_bool_true;
}

authum_bool_t
authum_sql_mysql_stmt_step_row(authum_sql_stmt_t *stmt)
{
	struct authum_sql_mysql_stmt *ms;

	if (!authum_sql_mysql_stmt_execute(stmt, authum_bool_true))
	{
		return authum_bool_false;
	}
	ms = (struct authum_sql_mysql_stmt *)stmt->stmt;
	if (authum_null_eq(ms->res))
	{
		return authum_bool_false;
	}
	ms->row = g_mysql_api.fetch_row(ms->res);
	return authum_null_eq(ms->row) ? authum_bool_false : authum_bool_true;
}

authum_bool_t
authum_sql_mysql_stmt_step_done(authum_sql_stmt_t *stmt)
{
	return authum_sql_mysql_stmt_execute(stmt, authum_bool_false);
}

authum_i32_t
authum_sql_mysql_stmt_column_i32(authum_sql_stmt_t *stmt, authum_i32_t index)
{
	struct authum_sql_mysql_stmt *ms;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || index < 0)
	{
		return 0;
	}
	ms = (struct authum_sql_mysql_stmt *)stmt->stmt;
	if (authum_null_eq(ms->row) || authum_null_eq(ms->row[index]))
	{
		return 0;
	}
	return (authum_i32_t)atoi(ms->row[index]);
}

const authum_char_t *
authum_sql_mysql_stmt_column_cstr(authum_sql_stmt_t *stmt, authum_i32_t index)
{
	struct authum_sql_mysql_stmt *ms;

	if (authum_null_eq(stmt) || authum_null_eq(stmt->stmt) || index < 0)
	{
		return "";
	}
	ms = (struct authum_sql_mysql_stmt *)stmt->stmt;
	if (authum_null_eq(ms->row) || authum_null_eq(ms->row[index]))
	{
		return "";
	}
	return ms->row[index];
}
