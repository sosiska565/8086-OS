ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 0x300000;

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