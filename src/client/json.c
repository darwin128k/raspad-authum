#include <authum/json.h>

#include <authum/cstr.h>

#include <stdlib.h>
#include <string.h>

static const authum_char_t *
authum_json_skip_ws(const authum_char_t *p)
{
	while (!authum_null_eq(p) && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
		++p;
	return p;
}

/**
 * Find `"key"` then `:` in @p json. Returns pointer at the value start, or null.
 */
static const authum_char_t *
authum_json_object_find_value(const authum_char_t *json, const authum_char_t *key)
{
	authum_char_t needle[96];
	authum_usize_t key_len;
	const authum_char_t *p;
	const authum_char_t *end;

	if (authum_null_eq(json) || authum_null_eq(key) || key[0] == '\0')
		return authum_null;

	key_len = (authum_usize_t)strlen(key);
	if (key_len + 3u >= sizeof needle)
		return authum_null;

	needle[0] = '"';
	memcpy(needle + 1, key, key_len);
	needle[key_len + 1u] = '"';
	needle[key_len + 2u] = '\0';

	p = json;
	for (;;) {
		p = strstr(p, needle);
		if (authum_null_eq(p))
			return authum_null;
		/* avoid matching a longer key that ends with our key */
		if (p > json) {
			authum_char_t before = p[-1];
			if (before != '{' && before != ',' && before != ' ' && before != '\t' &&
			    before != '\n' && before != '\r') {
				p += key_len + 2u;
				continue;
			}
		}
		end = p + key_len + 2u;
		end = authum_json_skip_ws(end);
		if (*end != ':') {
			p += key_len + 2u;
			continue;
		}
		return authum_json_skip_ws(end + 1);
	}
}

authum_bool_t
authum_json_object_get_cstr(const authum_char_t *json, const authum_char_t *key, authum_char_t *out,
			    authum_usize_t out_size)
{
	const authum_char_t *p;
	authum_usize_t n = 0u;

	if (authum_null_eq(out) || out_size == 0u)
		return authum_bool_false;
	out[0] = '\0';

	p = authum_json_object_find_value(json, key);
	if (authum_null_eq(p) || *p != '"')
		return authum_bool_false;
	++p;

	while (*p && *p != '"') {
		authum_char_t c = *p++;
		if (c == '\\' && *p) {
			c = *p++;
			if (c == 'n')
				c = '\n';
			else if (c == 't')
				c = '\t';
			else if (c == 'r')
				c = '\r';
			/* \" \\ \/ kept as the escaped char */
		}
		if (n + 1u < out_size)
			out[n++] = c;
	}
	out[n] = '\0';
	return (*p == '"') ? authum_bool_true : authum_bool_false;
}

authum_bool_t
authum_json_object_get_i32(const authum_char_t *json, const authum_char_t *key, authum_i32_t *out)
{
	const authum_char_t *p;
	authum_char_t *end = authum_null;
	long v;

	if (authum_null_eq(out))
		return authum_bool_false;

	p = authum_json_object_find_value(json, key);
	if (authum_null_eq(p))
		return authum_bool_false;
	if (*p != '-' && (*p < '0' || *p > '9'))
		return authum_bool_false;

	v = strtol(p, &end, 10);
	if (authum_null_eq(end) || end == p)
		return authum_bool_false;
	*out = (authum_i32_t)v;
	return authum_bool_true;
}

authum_i32_t
authum_json_object_get_i32_or(const authum_char_t *json, const authum_char_t *key,
			      authum_i32_t fallback)
{
	authum_i32_t v;

	if (authum_json_object_get_i32(json, key, &v))
		return v;
	return fallback;
}
