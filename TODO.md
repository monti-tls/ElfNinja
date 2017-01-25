### Plug-in handlers from libelfninja_core and libelfninja_dump :

- `enj_elf_get_user_content_view(enj_elf_shdr*, enj_elf_content_view**, enj_error**)` : setup a new ELF section content view

- `enj_note_get_user_content_view(enj_note*, enj_note_content_view**, enj_error**)` : setup a new SHT_NOTE content view

- `int enj_dump_note_get_user_register_handler(size_t, int(**handler)(enj_dump_formatter*, enj_error**), enj_error** err)` : setup a new SHT_NOTE dump formatter

### TODO :
- ElfNinja program plugin API :
  - [dump] custom default format string and headers for user SHT_NOTE contents
- Ensure alignment of note descriptor when pushing enj_note
- Add / remove a note entry from an SHT_NOTE
- Fix dependency problem for ElfNinja program <-> libraries

### TODO :
- [prog] add phdr tool + add
- [prog] find a way to display layout w/ segments
- [prog] for 'data <file> insert/write --file=<file>', specify fragment to use from the file ?
- [lib]  add flag in enj_blob_insert (and others ?) : MODE_BEFORE_END and MODE_AFTER_START, where :
           - anchor shifted if == start if MODE_BEFORE_END
           - anchor not shifted if == start if MODE_AFTER_START
- [lib]  see enj_blob_move and anchor management

- [*]    error-robust ELF construction (think of an invalid PHDR/SHDR)
- [*]    no SHDR ? (+ ehdr add table)
- [*]    no PHDR ? (+ ehdr add table)
