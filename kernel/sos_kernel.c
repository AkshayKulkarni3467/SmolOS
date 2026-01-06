volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;

void kernel_main(void) {
    const char* message = "Hello from SmolOS!";
    

    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = 0x0F00; 
    }
    

    int i = 0;
    while (message[i] != '\0') {
        vga_buffer[i] = 0x0F00 | message[i];
        i++;
    }
    

    while (1) {
        __asm__ volatile ("hlt");
    }
}