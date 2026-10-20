# JSON (`<authum/json.h>`)

Small **object field** helpers for flat JSON (one level), e.g. HTTP API bodies.
Not a full parser/DOM — modules that need trees should bring their own later or
extend this brick.

```c
authum_char_t code[8];
authum_i32_t asn;

authum_json_object_get_cstr(body, "countryCode", code, sizeof code);
asn = authum_json_object_get_i32_or(body, "asn", -1);
```

String escapes handled: `\"` `\\` `\/` `\n` `\t` `\r`.
