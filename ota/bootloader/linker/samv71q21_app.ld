/**
 * samv71q21_app.ld
 * Linker script da aplicação — DFP-compatível, flash em 0x00410000.
 *
 * Derivado do ATSAMV71Q21B.ld do DFP (Microchip SAMV71_DFP 4.12.237),
 * com o ORIGIN do flash alterado de 0x00400000 para 0x00410000 para que a
 * aplicação coexista com o bootloader OTA nos primeiros 64 KB.
 *
 * Diferenças face ao script padrão do DFP:
 *   - ORIGIN(rom) = 0x00410000  (era 0x00400000)
 *   - LENGTH(rom) = 0x001F0000  (1984 KB — eram 2048 KB)
 *
 * Todos os símbolos (_sfixed, _efixed, _etext, _szero, _ezero, __rom_end,
 * __ram_end, __bss_start__, __bss_end__) mantêm os nomes esperados pelo
 * DFP startup (cckIDs2G.o) e pelo runtime XC32 (libpic32c.a / data_init.o).
 *
 * NÃO adicionar startup_samv71.c ao projecto — o startup do DFP é usado.
 *
 * Como usar:
 *   Project Properties → XC32 → xc32-ld → Linker Script:
 *     ota/bootloader/linker/samv71q21_app.ld
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* =========================================================================
 * Regiões de memória
 * ========================================================================= */
MEMORY
{
    /* Flash — região da aplicação: começa após os 64 KB do bootloader */
    rom  (rx)  : ORIGIN = 0x00410000, LENGTH = 0x001F0000  /* 1984 KB */

    /* SRAM completa — o bootloader não retorna, a app usa toda a RAM */
    ram  (rwx) : ORIGIN = 0x20400000, LENGTH = 0x00060000  /* 384 KB  */

    /* TCM — tamanho 0 (não usado pelo bootloader nem pela app por omissão) */
    itcm (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00000000
    dtcm (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00000000
}

/* Símbolos de fim de memória — usados pelo DFP startup para o SP inicial
 * (primeira entrada da tabela de vectores = __ram_end = topo da SRAM). */
__rom_end = ORIGIN(rom) + LENGTH(rom);
__ram_end = ORIGIN(ram) + LENGTH(ram);

/* =========================================================================
 * Secções
 * ========================================================================= */
SECTIONS
{
    /* ------------------------------------------------------------------
     * .vectors — tabela de vectores do DFP startup (exception_table)
     *   _sfixed = 0x00410000: usado pelo SystemInit do DFP para VTOR.
     *   O bootloader lê [0] = SP inicial e [1] = Reset_Handler daqui.
     * ------------------------------------------------------------------ */
    .vectors :
    {
        . = ALIGN(4);
        _sfixed = .;
        KEEP(*(.vectors .vectors.* .vectors_default .vectors_default.*))
        KEEP(*(.isr_vector))
        KEEP(*(.reset*))
        KEEP(*(.after_vectors))
    } > rom

    /* ------------------------------------------------------------------
     * .text — código e tabelas de init/fini
     *   _efixed: fim da secção fixa (antes do .ARM.exidx).
     * ------------------------------------------------------------------ */
    .text :
    {
        . = ALIGN(4);
        *(.glue_7t)
        *(.glue_7)
        *(.gnu.linkonce.r.*)
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
        KEEP(*(.init))
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT_BY_NAME(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
        KEEP(*crtbegin.o(.ctors))
        KEEP(*(EXCLUDE_FILE(*crtend.o) .ctors))
        KEEP(*(SORT_BY_NAME(.ctors.*)))
        KEEP(*crtend.o(.ctors))
        . = ALIGN(4);
        KEEP(*(.fini))
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP(*(.fini_array))
        KEEP(*(SORT_BY_NAME(.fini_array.*)))
        __fini_array_end = .;
        KEEP(*crtbegin.o(.dtors))
        KEEP(*(EXCLUDE_FILE(*crtend.o) .dtors))
        KEEP(*(SORT_BY_NAME(.dtors.*)))
        KEEP(*crtend.o(.dtors))
        . = ALIGN(4);
        _efixed = .;
        PROVIDE(__exidx_start = .);
    } > rom

    /* ------------------------------------------------------------------
     * .ARM.exidx — índice de desenrolamento de stack (C++ exceptions)
     *   _etext: definido depois do exidx — LMA usado pelo data_init do XC32.
     * ------------------------------------------------------------------ */
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        PROVIDE(__exidx_end = .);
        . = ALIGN(4);
        _etext = .;
    } > rom

    /* ------------------------------------------------------------------
     * .bss — dados não inicializados em RAM
     *   O XC32 coloca automaticamente .bss.*, .data.*, .dinit nas regiões
     *   de memória corretas — não é necessário enumerá-las aqui.
     *   _szero/_ezero e _sbss/_ebss: nomes esperados pelo DFP startup.
     * ------------------------------------------------------------------ */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        _sbss  = .;
        _szero = .;
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        _ebss  = .;
        _ezero = .;
        . = ALIGN(4);
        _end = .;
        _ram_end_ = (ORIGIN(ram) + LENGTH(ram)) - 1;
    } > ram

    /* ------------------------------------------------------------------
     * .ARM.attributes — metadados ELF (não mapeado em memória)
     * ------------------------------------------------------------------ */
    .ARM.attributes 0 :
    {
        *(.ARM.attributes)
    }
}
