/**
 * samv71q21_app.ld
 * Linker script para a APLICAÇÃO quando coexiste com o bootloader OTA.
 *
 * ATENÇÃO: Este ficheiro substitui o linker script gerado pelo MCC/MPLAB X
 * para a aplicação principal (PS_OBC.X). Deve ser configurado em:
 *   Project Properties → XC32 → xc32-ld → Linker Script
 *
 * Diferenças relativamente ao script padrão do MPLAB X:
 *   - Flash começa em 0x00410000 (não em 0x00400000)
 *     Os primeiros 64 KB (0x00400000-0x0040FFFF) pertencem ao bootloader.
 *   - Resto igual ao comportamento padrão para ATSAMV71Q21B.
 *
 * Mapa de memória da aplicação:
 *   Flash: 0x00410000 - 0x005FFFFF  (~1984 KB)
 *   SRAM:  0x20400000 - 0x2045FFFF  (384 KB — toda a SRAM)
 *
 * NOTA IMPORTANTE PARA O VECTOR TABLE OFFSET:
 *   A aplicação deve configurar SCB->VTOR = 0x00410000 no seu SystemInit()
 *   ou startup. Se usar o MCC/CMSIS SystemInit gerado, confirmar que
 *   __VECTOR_TABLE_ATTRIBUTE ou VTOR é inicializado para 0x00410000.
 *   Ver ota/docs/ota_protocol.md para instruções detalhadas.
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* =========================================================================
 * Memória
 * ========================================================================= */
MEMORY
{
    /* Flash interna — região da aplicação (~1984 KB)
     * Os primeiros 64 KB (0x00400000-0x0040FFFF) pertencem ao bootloader. */
    FLASH  (rx)  : ORIGIN = 0x00410000, LENGTH = 1984K

    /* SRAM completa (384 KB) — o bootloader não retorna, pelo que a
     * aplicação pode usar toda a SRAM. */
    RAM    (rwx) : ORIGIN = 0x20400000, LENGTH = 384K

    /* TCM (Tightly Coupled Memory) — opcional, para código crítico */
    ITCM   (rx)  : ORIGIN = 0x00000000, LENGTH = 16K
    DTCM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K

    /* QSPI memory-mapped */
    QSPIMEM (rx) : ORIGIN = 0x80000000, LENGTH = 16M
}

/* =========================================================================
 * Stack e Heap
 * ========================================================================= */
_stack_size = 0x2000;  /* 8 KB de stack para a aplicação                    */
_heap_size  = 0x1000;  /* 4 KB de heap (a aplicação usa alocação estática)  */
_estack = ORIGIN(RAM) + LENGTH(RAM);

PROVIDE(_sidata = LOADADDR(.data));

/* _sfixed: símbolo usado pelo DFP startup (SystemInit) para configurar VTOR.
 * Com flash a começar em 0x00410000, o VTOR fica correctamente definido. */
PROVIDE(_sfixed = ORIGIN(FLASH));

/* =========================================================================
 * Secções
 * ========================================================================= */
SECTIONS
{
    /* ------------------------------------------------------------------
     * .isr_vector — tabela de vectores da aplicação
     * DEVE estar em 0x00410000 para o bootloader saltar correctamente.
     * ------------------------------------------------------------------ */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* ------------------------------------------------------------------
     * .text — código da aplicação
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
     * .data e .bss
     * ------------------------------------------------------------------ */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

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

    /* Heap */
    ._heap :
    {
        . = ALIGN(8);
        _sheap = .;
        . = . + _heap_size;
        . = ALIGN(8);
        _eheap = .;
    } > RAM

    /* Stack */
    ._stack :
    {
        . = ALIGN(8);
        . = . + _stack_size;
        . = ALIGN(8);
    } > RAM

    /* ------------------------------------------------------------------
     * Verificação: garante que a aplicação não ultrapassa a flash disponível
     * ------------------------------------------------------------------ */
    _app_flash_end = LOADADDR(.data) + SIZEOF(.data);
    ASSERT(_app_flash_end <= ORIGIN(FLASH) + LENGTH(FLASH),
           "ERRO: Aplicação excede a flash disponível (0x00410000-0x005FFFFF)!")

    .ARM.attributes 0 :
    {
        *(.ARM.attributes)
    }
}
