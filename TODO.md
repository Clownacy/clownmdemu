# Implemented
- 68k
  - Most instructions
  - Condition codes
  - Interrupts
  - Exceptions (very incomplete and broken)
- Z80
  - Most instructions
  - Flags
  - Interrupt mode 1
  - Instruction cycle durations
- VDP
  - Scanline-based rendering
  - H40 and H30
  - V30 and V28
  - Shadow/highlight mode
  - Interlace Mode 2
  - Sprite/pixel limit
  - Sprite masking
  - Sprite table caching
- FM
  - 6 FM channels
    - Phase Generator
    - Envelope Generator
      - ASDR envelopes
    - Operators
      - Feedback
      - Algorithms
  - DAC channel
- PSG
  - 3 tone channels
  - Noise channel
    - White noise mode
    - Periodic noise mode
- Joypads
  - 3-button Mega Drive controller
  - Two joypads


# Unimplemented
- 68k
  - Instruction cycle durations
  - User mode
  - Exceptions
  - The ABCD, ADDX, NBCD, NEGX, RESET, SBCD, SUBX, and STOP instructions
- VDP
  - Slot-based rendering
  - Interlacing in Interlace Mode 1 and Interlace Mode 2
  - Edgecase when the sprite mask is the first sprite rendered on the scanline
  - VRAM-to-VRAM DMA
  - DMA transfer durations
  - HV counter
  - Mode 4
- Z80
  - The DAA, HALT, OUT, IN, RETN, RETI, IM, RRD, RLD, CPI, CPD, CPIR, CPDR,
    INI, IND, INIR, INDR, OTDI, OUTD, OTIR, and OTDR instructions.
  - Interrupt modes 0 and 2
  - Proper interrupt timings
- FM
  - Envelope Generator
    - SSG-EG
  - Low-Frequency Oscillator
  - Timer A and Timer B
  - Per-operator frequencies
  - Debug registers
  - Busy flag
- SRAM
- Cartridge mappers
- Joypads
  - Having more than two joypads connected at once
  - Types of joypad besides the standard 3-button Mega Drive controller
  - The latch delay that Decap Attack requires in order for its input to work
- Master System support
- Game Gear support
- Mega CD support
- 32X support
- Remaining quirks and undefined behaviour of the original Mega Drive hardware
