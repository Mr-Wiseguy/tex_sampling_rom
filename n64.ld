
OUTPUT_ARCH(mips)

#define START_SEGMENT(segmentName, ramAddr) \
    _##segmentName##SegmentStart = ADDR(.segmentName); \
    _##segmentName##SegmentRomStart = romPos; \
    .segmentName ramAddr : AT(romPos) SUBALIGN(16)

#define START_NOLOAD(segmentName, ramAddr) \
    _##segmentName##SegmentBssStart = ADDR(.segmentName.noload); \
    .segmentName.noload ramAddr (NOLOAD) : SUBALIGN(16)

#define START_SEGMENT_AFTER(segmentName, previousSegment) \
    _##segmentName##SegmentStart = ADDR(.segmentName); \
    _##segmentName##SegmentRomStart = romPos; \
    .segmentName ADDR(.previousSegment) + SIZEOF(.previousSegment) : AT(romPos) SUBALIGN(16)

#define START_NOLOAD_AFTER(segmentName, previousSegment) \
    _##segmentName##SegmentBssStart = ADDR(.segmentName.noload); \
    .segmentName.noload ADDR(.previousSegment) + SIZEOF(.previousSegment) (NOLOAD) : SUBALIGN(16)

#define END_SEGMENT(segmentName) \
    _##segmentName##SegmentEnd = ADDR(.segmentName) + SIZEOF(.segmentName); \
    romPos += SIZEOF(.segmentName); \
    _##segmentName##SegmentSize = SIZEOF(.segmentName); \
    _##segmentName##SegmentRomEnd = romPos

#define END_NOLOAD(segmentName) \
    _##segmentName##SegmentBssEnd = ADDR(.segmentName.noload) + SIZEOF(.segmentName.noload); \
    _##segmentName##SegmentBssSize = SIZEOF(.segmentName.noload)

SECTIONS
{
    romPos = 0;

    START_SEGMENT(header, 0xA4000000)
    {
        KEEP(BUILD_ROOT/platforms/n64/src/header/header.o(.*data));
        KEEP(BUILD_ROOT/platforms/n64/boot/boot.6102.o);
    }
    END_SEGMENT(header);

    romPos = 0x1000;

    START_SEGMENT(main, 0x80000400)
    {
        KEEP(BUILD_ROOT/platforms/n64/boot/entry.o(.text*));
        *(.text*);
        
        PR/rspboot.o(.*text*);
        PR/gspF3DEX2.fifo.o(.*text*);
        /* PR/gspF3DEX2.Rej.fifo.o(.text*); */
        /* PR/gspL3DEX2.fifo.o(.text*); */
        /* PR/gspS2DEX2.fifo.o(.text*); */
        PR/n_aspMain.o(.*text*);

        . = ALIGN(64);
        *(.data*);
        *(COMMON*);
        *(.scommon*);
        . = ALIGN(16);
        _ptrBankStart = .;
        KEEP(BUILD_ROOT/sounds.ptr.o(.*data*));
        . = ALIGN(16);
        _fxBankStart = .;
        KEEP(BUILD_ROOT/fxbank.bin.o(.*data*));
        BUILD_ROOT/platforms/n64/src/gfx/font.o(.*data*);

        PR/rspboot.o(.*data*);
        PR/gspF3DEX2.fifo.o(.*data*);
        /* PR/gspF3DEX2.Rej.fifo.o(.data*); */
        /* PR/gspL3DEX2.fifo.o(.*data*); */
        /* PR/gspS2DEX2.fifo.o(.*data*); */
        PR/n_aspMain.o(.*data*);
        *(.rodata*);
        . = ALIGN(16);
    }
    END_SEGMENT(main);

    __popcountsi2 = numberOfSetBits;
    __popcountdi2 = numberOfSetBits;

    START_NOLOAD_AFTER(main, main)
    {
        . = ALIGN(64);
        *(.bss*);
        . = ALIGN(16);
    }
    END_NOLOAD(main);

    memPoolStart = .;

    START_SEGMENT(assets, romPos)
    {
        KEEP(BUILD_ROOT/assets.o(.*data*));
        . = ALIGN(16);
    }
    END_SEGMENT(assets);
    START_SEGMENT(wavetable, romPos)
    {
        KEEP(BUILD_ROOT/sounds.wbk.o(.*data*));
        . = ALIGN(16);
    }
    END_SEGMENT(wavetable);

  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end ) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /* DWARF 3 */
  .debug_pubtypes 0 : { *(.debug_pubtypes) }
  .debug_ranges   0 : { *(.debug_ranges) }
  /* DWARF Extension.  */
  .debug_macro    0 : { *(.debug_macro) }
  .gnu.attributes 0 : { KEEP (*(.gnu.attributes)) }

    /DISCARD/ :
    {
        *(*);
    }
}