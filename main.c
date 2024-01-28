/*
 * Copyright (c) 2024-Present, Japroz Singh Saini <japrozsaini@outlook.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach-o/loader.h>

// colors
#define RED_BOLD "\033[1;31m"
#define WHITE_BOLD "\033[1;37m"
#define RESET "\033[0m"

// helpful macros
#define ALLOC(type, count) (type *)malloc(sizeof(type) * (count))

// CONSTANT DEFINITION START
/*
 * After MacOS X 10.1 when a new load command is added that is required to be
 * understood by the dynamic linker for the image to execute properly the
 * LC_REQ_DYLD bit will be or'ed into the load command constant.  If the dynamic
 * linker sees such a load command it it does not understand will issue a
 * "unknown load command required for execution" error and refuse to use the
 * image.  Other load commands without this bit that are not understood will
 * simply be ignored.
 */
#define LC_REQ_DYLD 0x80000000

/* Constants for the cmd field of all load commands, the type */
#define LC_SEGMENT 0x1         /* segment of this file to be mapped */
#define LC_SYMTAB 0x2          /* link-edit stab symbol table info */
#define LC_SYMSEG 0x3          /* link-edit gdb symbol table info (obsolete) */
#define LC_THREAD 0x4          /* thread */
#define LC_UNIXTHREAD 0x5      /* unix thread (includes a stack) */
#define LC_LOADFVMLIB 0x6      /* load a specified fixed VM shared library */
#define LC_IDFVMLIB 0x7        /* fixed VM shared library identification */
#define LC_IDENT 0x8           /* object identification info (obsolete) */
#define LC_FVMFILE 0x9         /* fixed VM file inclusion (internal use) */
#define LC_PREPAGE 0xa         /* prepage command (internal use) */
#define LC_DYSYMTAB 0xb        /* dynamic link-edit symbol table info */
#define LC_LOAD_DYLIB 0xc      /* load a dynamically linked shared library */
#define LC_ID_DYLIB 0xd        /* dynamically linked shared lib ident */
#define LC_LOAD_DYLINKER 0xe   /* load a dynamic linker */
#define LC_ID_DYLINKER 0xf     /* dynamic linker identification */
#define LC_PREBOUND_DYLIB 0x10 /* modules prebound for a dynamically */
                               /*  linked shared library */
#define LC_ROUTINES 0x11       /* image routines */
#define LC_SUB_FRAMEWORK 0x12  /* sub framework */
#define LC_SUB_UMBRELLA 0x13   /* sub umbrella */
#define LC_SUB_CLIENT 0x14     /* sub client */
#define LC_SUB_LIBRARY 0x15    /* sub library */
#define LC_TWOLEVEL_HINTS 0x16 /* two-level namespace lookup hints */
#define LC_PREBIND_CKSUM 0x17  /* prebind checksum */

// END CONSTANTS DEFINITIONS

struct nlist_64_t
{
  uint32_t n_strx;  /* index into the string table */
  uint8_t n_type;   /* type flag, see below */
  uint8_t n_sect;   /* section number or NO_SECT */
  uint16_t n_desc;  /* see <mach-o/stab.h> */
  uint64_t n_value; /* value of this symbol (or stab offset) */
};

struct dysymtab_command_t
{
  uint32_t ilocalsym;      /* index to local symbols */
  uint32_t nlocalsym;      /* number of local symbols */
  uint32_t iextdefsym;     /* index to externally defined symbols */
  uint32_t nextdefsym;     /* number of externally defined symbols */
  uint32_t iundefsym;      /* index to undefined symbols */
  uint32_t nundefsym;      /* number of undefined symbols */
  uint32_t tocoff;         /* file offset to table of contents */
  uint32_t ntoc;           /* number of entries in table of contents */
  uint32_t modtaboff;      /* file offset to module table */
  uint32_t nmodtab;        /* number of module table entries */
  uint32_t extrefsymoff;   /* offset to referenced symbol table */
  uint32_t nextrefsyms;    /* number of referenced symbol table entries */
  uint32_t indirectsymoff; /* file offset to the indirect symbol table */
  uint32_t nindirectsyms;  /* number of indirect symbol table entries */
  uint32_t extreloff;      /* offset to external relocation entries */
  uint32_t nextrel;        /* number of external relocation entries */
  uint32_t locreloff;      /* offset to local relocation entries */
  uint32_t nlocrel;        /* number of local relocation entries */
};

struct symtab_command_t
{
  uint32_t symoff;  /* symbol table offset */
  uint32_t nsyms;   /* number of symbol table entries */
  uint32_t stroff;  /* string table offset */
  uint32_t strsize; /* string table size in bytes */
  char *string_table;
  struct nlist_64_t *symbol_table;
};

struct section_64_t
{                     /* for 64-bit architectures */
  char sectname[16];  /* name of this section */
  char segname[16];   /* segment this section goes in */
  uint64_t addr;      /* memory address of this section */
  uint64_t size;      /* size in bytes of this section */
  uint32_t offset;    /* file offset of this section */
  uint32_t align;     /* section alignment (power of 2) */
  uint32_t reloff;    /* file offset of relocation entries */
  uint32_t nreloc;    /* number of relocation entries */
  uint32_t flags;     /* flags (section type and attributes)*/
  uint32_t reserved1; /* reserved (for offset or index) */
  uint32_t reserved2; /* reserved (for count or sizeof) */
  uint32_t reserved3; /* reserved */
};

struct segment_command_64_t
{
  char segname[16];   /* segment name */
  uint64_t vmaddr;    /* memory address of this segment */
  uint64_t vmsize;    /* memory size of this segment */
  uint64_t fileoff;   /* file offset of this segment */
  uint64_t filesize;  /* amount to map from the file */
  vm_prot_t maxprot;  /* maximum VM protection */
  vm_prot_t initprot; /* initial VM protection */
  uint32_t nsects;    /* number of sections in segment */
  uint32_t flags;     /* flags */
  struct section_64_t *sections;
};

struct build_version_command_t
{
  uint32_t platform; /* platform */
  uint32_t minos;    /* X.Y.Z is encoded in nibbles xxxx.yy.zz */
  uint32_t sdk;      /* X.Y.Z is encoded in nibbles xxxx.yy.zz */
  uint32_t ntools;   /* number of tool entries following this */
};

struct load_command_t
{
  uint32_t cmd;
  uint32_t cmd_size;
  union
  {
    struct segment_command_64_t cmd_seg_64;
    struct build_version_command_t cmd_build_version;
    struct symtab_command_t cmd_symtab;
    struct dysymtab_command_t cmd_dysymtab;
  };
};

struct mach_object_file_t
{
  uint32_t magic;
  uint32_t cpu_type;
  uint32_t cpu_subtype;
  uint32_t file_type;
  uint32_t number_of_load_commands;
  uint32_t size_of_load_commands;
  uint32_t flags;
  uint32_t reserved;
  struct load_command_t *commands;
};

// main file used for all operations
struct mach_object_file_t object_file;

void parse_file(FILE *file)
{
  fread(&object_file.magic, sizeof(uint32_t), 1, file);
  fread(&object_file.cpu_type, sizeof(uint32_t), 1, file);
  fread(&object_file.cpu_subtype, sizeof(uint32_t), 1, file);
  fread(&object_file.file_type, sizeof(uint32_t), 1, file);
  fread(&object_file.number_of_load_commands, sizeof(uint32_t), 1, file);
  fread(&object_file.size_of_load_commands, sizeof(uint32_t), 1, file);
  fread(&object_file.flags, sizeof(uint32_t), 1, file);
  fread(&object_file.reserved, sizeof(uint32_t), 1, file);

  object_file.commands = ALLOC(struct load_command_t, object_file.number_of_load_commands);
  for (uint32_t i = 0; i < object_file.number_of_load_commands; i++)
  {
    uint32_t cmd, cmd_size;
    fread(&cmd, sizeof(uint32_t), 1, file);
    fread(&cmd_size, sizeof(uint32_t), 1, file);
    switch (cmd)
    {
    case LC_SEGMENT_64:
    {
      struct segment_command_64_t segment;
      // parse the segment_64 section
      fread(segment.segname, sizeof(uint8_t), 16, file);
      fread(&segment.vmaddr, sizeof(uint64_t), 1, file);
      fread(&segment.vmsize, sizeof(uint64_t), 1, file);
      fread(&segment.fileoff, sizeof(uint64_t), 1, file);
      fread(&segment.filesize, sizeof(uint64_t), 1, file);
      fread(&segment.maxprot, sizeof(vm_prot_t), 1, file);
      fread(&segment.initprot, sizeof(vm_prot_t), 1, file);
      fread(&segment.nsects, sizeof(uint32_t), 1, file);
      fread(&segment.flags, sizeof(uint32_t), 1, file);

      // printf("segname : \"%s\"\n", segment.segname);
      // printf("vmaddr  : 0x%016llx\n", segment.vmaddr);
      // printf("vmsize  : 0x%016llx\n", segment.vmsize);
      // printf("fileoff  : 0x%016llx\n", segment.fileoff);
      // printf("filesize  : 0x%016llx\n", segment.filesize);
      // printf("maxprot  : 0x%08x\n", segment.maxprot);
      // printf("initprot  : 0x%08x\n", segment.initprot);
      // printf("nsects  : 0x%08x\n", segment.nsects);
      // printf("flags  : 0x%08x\n", segment.flags);

      segment.sections = ALLOC(struct section_64_t, segment.nsects);
      for (uint32_t k = 0; k < segment.nsects; k++)
      {
        struct section_64_t section;

        fread(section.sectname, sizeof(uint8_t), 16, file);
        fread(section.segname, sizeof(uint8_t), 16, file);
        fread(&section.addr, sizeof(uint64_t), 1, file);
        fread(&section.size, sizeof(uint64_t), 1, file);
        fread(&section.offset, sizeof(uint32_t), 1, file);
        fread(&section.align, sizeof(uint32_t), 1, file);
        fread(&section.reloff, sizeof(uint32_t), 1, file);
        fread(&section.nreloc, sizeof(uint32_t), 1, file);
        fread(&section.flags, sizeof(uint32_t), 1, file);
        fread(&section.reserved1, sizeof(uint32_t), 1, file);
        fread(&section.reserved2, sizeof(uint32_t), 1, file);
        fread(&section.reserved3, sizeof(uint32_t), 1, file);

        segment.sections[k] = section;

        // printf("sectname : \"%s\", %lu\n", section.sectname, strlen(section.sectname));
        // printf("segname : \"%s\"\n", section.segname);
        // printf("addr : 0x%016llx\n", section.addr);
        // printf("size : 0x%016llx\n", section.size);
        // printf("offset : 0x%08x\n", section.offset);
        // printf("align : 0x%08x\n", section.align);
        // printf("reloff : 0x%08x\n", section.reloff);
        // printf("nreloc : 0x%08x\n", section.nreloc);
        // printf("flags : 0x%08x\n", section.flags);
        // printf("reserved1 : 0x%08x\n", section.reserved1);
        // printf("reserved2 : 0x%08x\n", section.reserved2);
        // printf("reserved3 : 0x%08x\n", section.reserved3);
      }

      struct load_command_t command =
          {
              .cmd = cmd,
              .cmd_size = cmd_size,
              .cmd_seg_64 = segment,
          };
      object_file.commands[i] = command;

      break;
    }
    case LC_DYSYMTAB:
    {
      struct dysymtab_command_t dysymtab;

      fread(&dysymtab.ilocalsym, sizeof(uint32_t), 1, file);
      fread(&dysymtab.nlocalsym, sizeof(uint32_t), 1, file);
      fread(&dysymtab.iextdefsym, sizeof(uint32_t), 1, file);
      fread(&dysymtab.nextdefsym, sizeof(uint32_t), 1, file);
      fread(&dysymtab.iundefsym, sizeof(uint32_t), 1, file);
      fread(&dysymtab.nundefsym, sizeof(uint32_t), 1, file);
      fread(&dysymtab.tocoff, sizeof(uint32_t), 1, file);
      fread(&dysymtab.ntoc, sizeof(uint32_t), 1, file);
      fread(&dysymtab.modtaboff, sizeof(uint32_t), 1, file);
      fread(&dysymtab.nmodtab, sizeof(uint32_t), 1, file);
      fread(&dysymtab.extrefsymoff, sizeof(uint32_t), 1, file);
      fread(&dysymtab.nextrefsyms, sizeof(uint32_t), 1, file);
      fread(&dysymtab.indirectsymoff, sizeof(uint32_t), 1, file);
      fread(&dysymtab.nindirectsyms, sizeof(uint32_t), 1, file);
      fread(&dysymtab.extreloff, sizeof(uint32_t), 1, file);
      fread(&dysymtab.nextrel, sizeof(uint32_t), 1, file);
      fread(&dysymtab.locreloff, sizeof(uint32_t), 1, file);
      fread(&dysymtab.nlocrel, sizeof(uint32_t), 1, file);

      struct load_command_t command =
          {
              .cmd = cmd,
              .cmd_size = cmd_size,
              .cmd_dysymtab = dysymtab,
          };
      object_file.commands[i] = command;

      // printf("ilocalsym     : 0x%08x\n", dysymtab.ilocalsym);
      // printf("nlocalsym     : 0x%08x\n", dysymtab.nlocalsym);
      // printf("iextdefsym     : 0x%08x\n", dysymtab.iextdefsym);
      // printf("nextdefsym     : 0x%08x\n", dysymtab.nextdefsym);
      // printf("iundefsym     : 0x%08x\n", dysymtab.iundefsym);
      // printf("nundefsym     : 0x%08x\n", dysymtab.nundefsym);
      // printf("tocoff     : 0x%08x\n", dysymtab.tocoff);
      // printf("ntoc     : 0x%08x\n", dysymtab.ntoc);
      // printf("modtaboff     : 0x%08x\n", dysymtab.modtaboff);
      // printf("nmodtab     : 0x%08x\n", dysymtab.nmodtab);
      // printf("extrefsymoff     : 0x%08x\n", dysymtab.extrefsymoff);
      // printf("nextrefsyms     : 0x%08x\n", dysymtab.nextrefsyms);
      // printf("indirectsymoff     : 0x%08x\n", dysymtab.indirectsymoff);
      // printf("nindirectsyms     : 0x%08x\n", dysymtab.nindirectsyms);
      // printf("extreloff     : 0x%08x\n", dysymtab.extreloff);
      // printf("nextrel     : 0x%08x\n", dysymtab.nextrel);
      // printf("locreloff     : 0x%08x\n", dysymtab.locreloff);
      // printf("nlocrel     : 0x%08x\n", dysymtab.nlocrel);

      break;
    }
    case LC_SYMTAB:
    {
      struct symtab_command_t symtab;
      fread(&symtab.symoff, sizeof(uint32_t), 1, file);
      fread(&symtab.nsyms, sizeof(uint32_t), 1, file);
      fread(&symtab.stroff, sizeof(uint32_t), 1, file);
      fread(&symtab.strsize, sizeof(uint32_t), 1, file);

      // printf("symoff  : 0x%08x\n", symtab.symoff);
      // printf("nsyms   : 0x%08x\n", symtab.nsyms);
      // printf("stroff  : 0x%08x\n", symtab.stroff);
      // printf("strsize : 0x%08x\n", symtab.strsize);

      struct load_command_t command =
          {
              .cmd = cmd,
              .cmd_size = cmd_size,
              .cmd_symtab = symtab,
          };
      object_file.commands[i] = command;

      break;
    }
    case LC_BUILD_VERSION:
    {
      struct build_version_command_t build_ver;

      fread(&build_ver.platform, sizeof(uint32_t), 1, file);
      fread(&build_ver.minos, sizeof(uint32_t), 1, file);
      fread(&build_ver.sdk, sizeof(uint32_t), 1, file);
      fread(&build_ver.ntools, sizeof(uint32_t), 1, file);

      // printf("platform : 0x%08x\n", build_ver.platform);
      // printf("minos    : 0x%08x\n", build_ver.minos);
      // printf("sdk      : 0x%08x\n", build_ver.sdk);
      // printf("ntools   : 0x%08x\n", build_ver.ntools);

      struct load_command_t command =
          {
              .cmd = cmd,
              .cmd_size = cmd_size,
              .cmd_build_version = build_ver,
          };
      object_file.commands[i] = command;

      break;
    }
    default:
      printf("Encountered unexpected tag with value 0x%08x\n", cmd);
      exit(EXIT_FAILURE);
    }
  }

  for (uint32_t i = 0; i < object_file.number_of_load_commands; i++)
  {
    if (object_file.commands[i].cmd == LC_SYMTAB)
    {
      fseek(file, object_file.commands[i].cmd_symtab.stroff, SEEK_SET);
      char *string_table = malloc(sizeof(char) * object_file.commands[i].cmd_symtab.strsize);
      fread(string_table, sizeof(uint8_t), object_file.commands[i].cmd_symtab.strsize, file);

      object_file.commands[i].cmd_symtab.symbol_table = malloc(sizeof(struct nlist_64_t) * object_file.commands[i].cmd_symtab.nsyms);
      fseek(file, object_file.commands[i].cmd_symtab.symoff, SEEK_SET);
      for (uint32_t k = 0; k < object_file.commands[i].cmd_symtab.nsyms; k++)
      {
        struct nlist_64_t entry;

        fread(&entry.n_strx, sizeof(uint32_t), 1, file);
        fread(&entry.n_type, sizeof(uint8_t), 1, file);
        fread(&entry.n_sect, sizeof(uint8_t), 1, file);
        fread(&entry.n_desc, sizeof(uint16_t), 1, file);
        fread(&entry.n_value, sizeof(uint64_t), 1, file);

        object_file.commands[i].cmd_symtab.symbol_table[k] = entry;

        // printf("n_strx           : \"%s\", %lu\n", (char *)(string_table + entry.n_strx), strlen((char *)(string_table + entry.n_strx)));
        // printf("n_type           : 0x%02x\n", entry.n_type);
        // printf("n_sect           : 0x%02x\n", entry.n_sect);
        // printf("n_desc           : 0x%04x\n", entry.n_desc);
        // printf("n_value          : 0x%016llx\n", entry.n_value);
        // printf("---------------------------------------------------\n");
      }

      break; // early break out of the for loop
    }
  }
}

void pretty_print(void)
{
  printf("magic                       : 0x%08x\n"
         "cpu_type                    : 0x%08x\n"
         "cpu_subtype                 : 0x%08x\n"
         "file_type                   : 0x%08x\n"
         "number_of_load_commands     : 0x%08x\n"
         "size_of_load_commands       : 0x%08x\n"
         "flags                       : 0x%08x\n"
         "reserved                    : 0x%08x\n",
         object_file.magic, object_file.cpu_type, object_file.cpu_subtype, object_file.file_type, object_file.number_of_load_commands, object_file.size_of_load_commands, object_file.flags, object_file.reserved);
  printf("---------------------------------------\n");

  for (uint32_t i = 0; i < object_file.number_of_load_commands; i++)
  {
    struct load_command_t cmd = object_file.commands[i];

    switch (cmd.cmd)
    {
    case LC_SEGMENT_64:
    {
      printf("LC_SYMTAB (0x%08x)\n", cmd.cmd);
      printf("\tsegname       : \"%s\"\n", cmd.cmd_seg_64.segname);
      printf("\tvmaddr        : 0x%016llx\n", cmd.cmd_seg_64.vmaddr);
      printf("\tvmsize        : 0x%016llx\n", cmd.cmd_seg_64.vmaddr);
      printf("\tfileoff       : 0x%016llx\n", cmd.cmd_seg_64.fileoff);
      printf("\tfilesize      : 0x%016llx\n", cmd.cmd_seg_64.filesize);
      printf("\tmaxprot       : 0x%08x\n", cmd.cmd_seg_64.maxprot);
      printf("\tinitprot      : 0x%08x\n", cmd.cmd_seg_64.initprot);
      printf("\tnsects        : 0x%08x\n", cmd.cmd_seg_64.nsects);
      printf("\tflags         : 0x%08x\n", cmd.cmd_seg_64.flags);
      printf("\n");

      // print the sections

      break;
    }
    case LC_BUILD_VERSION:
    {
      printf("LC_BUILD_VERSION (0x%08x)\n", cmd.cmd);
      printf("\tplatform : 0x%08x\n", cmd.cmd_build_version.platform);
      printf("\tminos    : 0x%08x\n", cmd.cmd_build_version.minos);
      printf("\tsdk      : 0x%08x\n", cmd.cmd_build_version.sdk);
      printf("\tntools   : 0x%08x\n", cmd.cmd_build_version.ntools);
      printf("\n");
      break;
    }
    case LC_SYMTAB:
    {
      printf("LC_SYMTAB (0x%08x)\n", cmd.cmd);
      printf("\tsymoff  : 0x%08x\n", cmd.cmd_symtab.symoff);
      printf("\tnsyms   : 0x%08x\n", cmd.cmd_symtab.nsyms);
      printf("\tstroff  : 0x%08x\n", cmd.cmd_symtab.stroff);
      printf("\tstrsize : 0x%08x\n\n", cmd.cmd_symtab.strsize);
      break;
    }
    case LC_DYSYMTAB:
    {
      printf("LC_DYSYMTAB (0x%08x)\n", cmd.cmd);
      printf("\tilocalsym     : 0x%08x\n", cmd.cmd_dysymtab.ilocalsym);
      printf("\tnlocalsym     : 0x%08x\n", cmd.cmd_dysymtab.nlocalsym);
      printf("\tiextdefsym     : 0x%08x\n", cmd.cmd_dysymtab.iextdefsym);
      printf("\tnextdefsym     : 0x%08x\n", cmd.cmd_dysymtab.nextdefsym);
      printf("\tiundefsym     : 0x%08x\n", cmd.cmd_dysymtab.iundefsym);
      printf("\tnundefsym     : 0x%08x\n", cmd.cmd_dysymtab.nundefsym);
      printf("\ttocoff     : 0x%08x\n", cmd.cmd_dysymtab.tocoff);
      printf("\tntoc     : 0x%08x\n", cmd.cmd_dysymtab.ntoc);
      printf("\tmodtaboff     : 0x%08x\n", cmd.cmd_dysymtab.modtaboff);
      printf("\tnmodtab     : 0x%08x\n", cmd.cmd_dysymtab.nmodtab);
      printf("\textrefsymoff     : 0x%08x\n", cmd.cmd_dysymtab.extrefsymoff);
      printf("\tnextrefsyms     : 0x%08x\n", cmd.cmd_dysymtab.nextrefsyms);
      printf("\tindirectsymoff     : 0x%08x\n", cmd.cmd_dysymtab.indirectsymoff);
      printf("\tnindirectsyms     : 0x%08x\n", cmd.cmd_dysymtab.nindirectsyms);
      printf("\textreloff     : 0x%08x\n", cmd.cmd_dysymtab.extreloff);
      printf("\tnextrel     : 0x%08x\n", cmd.cmd_dysymtab.nextrel);
      printf("\tlocreloff     : 0x%08x\n", cmd.cmd_dysymtab.locreloff);
      printf("\tnlocrel     : 0x%08x\n\n", cmd.cmd_dysymtab.nlocrel);
      break;
    }
    default:
      printf("unknown opcode encountered : 0x%08x\n", cmd.cmd);
      break;
    }
  }
}

int main(int argc, const char **argv)
{
  if (argc != 2)
  {
    printf("%serror%s: incorrect number of arguments\n", RED_BOLD, RESET);
    printf("\n%susage%s: %s <filename>\n", WHITE_BOLD, RESET, argv[0]);
    exit(0);
  }

  FILE *file = fopen(argv[1], "rb"); // don't forget to fclose
  parse_file(file);
  pretty_print();

  return EXIT_SUCCESS;
}
