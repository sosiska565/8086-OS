ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 0x400000;

    .text : {
        *(.text.entry) 
        *(.text)
    }

    .data : {
        *(.data)
    }

    .rodata : {
        *(.rodata)
    }

    .bss : {
        *(.bss)
    }
}