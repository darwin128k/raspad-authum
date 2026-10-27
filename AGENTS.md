# Contributions to LH

Before changing `lib/lh`, read the nearest existing headers and implementations
for that subsystem, together with `lib/lh/.clang-format`.

- Follow LH types, pointer/cast helpers, field macros and callback `fn`/`cb` layout.
- Use LH memory, allocator, error and runtime facilities in portable code.
  Native APIs and native types belong in the matching OS backend.
- Document public functions, ownership, failure results and lifecycle constraints
  in the same form as neighboring LH headers.
- Extend an existing LH abstraction before introducing a parallel one.
- Run the formatter using LH's own configuration, inspect the diff manually,
  and test the affected primitives and their Authum integration.
  Formatting alone does not establish conformity with LH conventions.
