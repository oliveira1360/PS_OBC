/**
 * samv71q21_boot.ld
 * Linker script para o BOOTLOADER OTA — ATSAMV71Q21B
 *
 * Mapa de memória do bootloader:
 *   Flash interna: 0x00400000 - 0x0040FFFF  (64 KB)
 *   SRAM:          0x20400000 - 0x2040FFFF  (64 KB para o bootloader)
 *
 * A aplicação fica em 0x00410000 em diante (gerida pelo samv71q21_app.ld).
 *
 * Nota: O XC32 (Microchip) suporta sintaxe GNU LD standard.
 *       Se usar MPLAB X com XC32, adicionar este .ld em:
 *         Project Properties → XC32 → xc32-ld → Linker Script
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* =========================================================================
 * Memória
 * ========================================================================= */
MEMORY
{
    /* Flash interna — região do bootloader (64 KB) */
    FLASH  (rx)  : ORIGIN = 0x00400000, LENGTH = 64K

    /* SRAM — stack e variáveis do bootloader (64 KB)
     * A aplicação pode usar toda a SRAM (384 KB) pois o bootloader
     * não retorna — o stack do bootloader é abandonado ao saltar. */
    RAM    (rwx) : ORIGIN = 0x20400000, LENGTH = 64K

    /* QSPI memory-mapped region (só leitura via hardware) */
    QSPIMEM (rx) : ORIGIN = 0x80000000, LENGTH = 16M
}

/* =========================================================================
 * Stack
 * ========================================================================= */
_stack_size = 4096;   /* 4 KB de stack para o bootloader */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* =========================================================================
 * Símbolos de início/fim das secções (usados no startup)
 * ========================================================================= */
PROVIDE(_sidata = LOADADDR(.data));  /* LMA de .data (endereço na flash)    */

/* =========================================================================
 * Secções
 * ========================================================================= */
SECTIONS
{
    /* ------------------------------------------------------------------
     * .isr_vector — tabela de vectores de excepção
     * Deve ser a primeira coisa na flash (0x00400000).
     * ------------------------------------------------------------------ */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* ------------------------------------------------------------------
     * .text — código executável do bootloader
     * ------------------------------------------------------------------ */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* ------------------------------------------------------------------
     * ARM exception unwinding tables
     * ------------------------------------------------------------------ */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* ------------------------------------------------------------------
     * .preinit_array / .init_array / .fini_array
     * O bootloader não usa C++, mas manter por compatibilidade com newlib.
     * ------------------------------------------------------------------ */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    /* ------------------------------------------------------------------
     * .data — variáveis inicializadas (cópia de flash → RAM no startup)
     * ------------------------------------------------------------------ */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;           /* Início de .data em RAM                    */
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;           /* Fim de .data em RAM                       */
    } > RAM AT > FLASH        /* VMA=RAM, LMA=FLASH                        */

    /* ------------------------------------------------------------------
     * .bss — variáveis não inicializadas (zerificadas no startup)
     * ------------------------------------------------------------------ */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* ------------------------------------------------------------------
     * Stack — reservado no fim da RAM
     * ------------------------------------------------------------------ */
    ._stack :
    {
        . = ALIGN(8);
        . = . + _stack_size;
        . = ALIGN(8);
    } > RAM

    /* ------------------------------------------------------------------
     * Verificação: garante que o bootloader não ultrapassa 64 KB.
     * Usa LOADADDR(.data) + SIZEOF(.data) para cobrir todo o conteúdo
     * que fica na flash (código + cópia de .data).
     * ------------------------------------------------------------------ */
    _boot_flash_end = LOADADDR(.data) + SIZEOF(.data);
    ASSERT(_boot_flash_end <= ORIGIN(FLASH) + LENGTH(FLASH),
           "ERRO: Bootloader excede 64 KB de flash!")

    /* ------------------------------------------------------------------
     * Descarta secções desnecessárias
     * ------------------------------------------------------------------ */
    .ARM.attributes 0 :
    {
        *(.ARM.attributes)
    }
}
