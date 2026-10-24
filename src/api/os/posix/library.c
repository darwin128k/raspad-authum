#define _GNU_SOURCE

#include "library.h"

#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
authum_os_library_match(struct dl_phdr_info *info, size_t size, void *data)
{
    authum_os_library_t *out = (authum_os_library_t *)data;
    const char *base_name;

    (void)size;

    if (!info->dlpi_name || !info->dlpi_name[0])
    {
        return 0;
    }
    base_name = strrchr(info->dlpi_name, '/');
    base_name = base_name ? base_name + 1 : info->dlpi_name;
    if (strcmp(base_name, out->name) != 0)
    {
        return 0;
    }
    out->base = (uintptr_t)info->dlpi_addr;
    snprintf(out->path, sizeof out->path, "%s", info->dlpi_name);
    return 1; /* stop */
}

authum_bool_t
authum_os_library_read_all(int fd, void *buf, size_t n)
{
    uint8_t *p = (uint8_t *)buf;
    size_t got = 0;

    while (got < n)
    {
        ssize_t r = read(fd, p + got, n - got);
        if (r <= 0)
        {
            return authum_bool_false;
        }
        got += (size_t)r;
    }
    return authum_bool_true;
}

authum_ptr
authum_os_library_find_in_symtab(const authum_os_library_t *library, const authum_char_t *name)
{
    int fd;
    Elf32_Ehdr ehdr;
    Elf32_Shdr *shdrs = authum_null;
    Elf32_Sym *syms = authum_null;
    char *strs = authum_null;
    const Elf32_Shdr *symtab = authum_null;
    const Elf32_Shdr *strtab = authum_null;
    authum_ptr result = authum_null;
    size_t nsym;
    size_t i;

    if (!library->path[0])
    {
        return authum_null;
    }
    fd = open(library->path, O_RDONLY);
    if (fd < 0)
    {
        return authum_null;
    }

    memset(&ehdr, 0, sizeof ehdr);
    if (!authum_os_library_read_all(fd, &ehdr, sizeof ehdr) ||
        memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0 || ehdr.e_ident[EI_CLASS] != ELFCLASS32 ||
        ehdr.e_shentsize != sizeof(Elf32_Shdr) || ehdr.e_shnum == 0 ||
        lseek(fd, (off_t)ehdr.e_shoff, SEEK_SET) < 0)
    {
        goto done;
    }

    shdrs = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr) * ehdr.e_shnum);
    if (!shdrs || !authum_os_library_read_all(fd, shdrs, sizeof(Elf32_Shdr) * ehdr.e_shnum))
    {
        goto done;
    }
    for (i = 0; i < ehdr.e_shnum; i++)
    {
        if (shdrs[i].sh_type == SHT_SYMTAB && shdrs[i].sh_link < ehdr.e_shnum)
        {
            symtab = &shdrs[i];
            strtab = &shdrs[shdrs[i].sh_link];
            break;
        }
    }
    if (!symtab || symtab->sh_entsize != sizeof(Elf32_Sym))
    {
        goto done;
    }

    nsym = symtab->sh_size / sizeof(Elf32_Sym);
    syms = (Elf32_Sym *)malloc(symtab->sh_size);
    strs = (char *)malloc(strtab->sh_size);
    if (!syms || !strs || lseek(fd, (off_t)symtab->sh_offset, SEEK_SET) < 0 ||
        !authum_os_library_read_all(fd, syms, symtab->sh_size) ||
        lseek(fd, (off_t)strtab->sh_offset, SEEK_SET) < 0 ||
        !authum_os_library_read_all(fd, strs, strtab->sh_size))
    {
        goto done;
    }
    for (i = 0; i < nsym; i++)
    {
        if (syms[i].st_name == 0 || syms[i].st_value == 0 || syms[i].st_name >= strtab->sh_size)
        {
            continue;
        }
        if (strcmp(strs + syms[i].st_name, name) == 0)
        {
            /* ET_DYN: st_value is relative to the load bias. */
            result = (authum_ptr)(library->base + syms[i].st_value);
            break;
        }
    }

done:
    free(strs);
    free(syms);
    free(shdrs);
    close(fd);
    return result;
}

authum_ptr
authum_os_library_export(const authum_char_t *library, const authum_char_t *name)
{
    void *handle;
    void *sym;

    handle = dlopen(library, RTLD_NOW | RTLD_NOLOAD);
    if (!handle)
    {
        return authum_null;
    }
    sym = dlsym(handle, name);
    dlclose(handle); /* drops only the reference RTLD_NOLOAD added */
    return (authum_ptr)sym;
}

authum_ptr
authum_os_library_symtab(const authum_char_t *library, const authum_char_t *name)
{
    authum_os_library_t lib;

    memset(&lib, 0, sizeof lib);
    lib.name = library;
    dl_iterate_phdr(authum_os_library_match, &lib);
    if (!lib.base)
    {
        return authum_null;
    }
    return authum_os_library_find_in_symtab(&lib, name);
}
