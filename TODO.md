# Implemented

- 68k
  - Most instructions
  - Condition codes
  - Interrupts
- VDP
  - Scanline-based rendering
  - H40 and H30
  - V30 and V28
  - Shadow/highlight mode
  - Interlace Mode 2
  - Sprite/pixel limit
  - Sprite masking
  - Sprite table caching
- Joypads
  - 3-button Mega Drive controller
  - Two joypads


# Unimplemented

## Emulator
- 68k
  - Instruction cycle durations
  - User mode
  - Exceptions
  - The ABCD, ADDX, CHK, ILLEGAL, MOVE USP, NBCD, NEGX, RESET, SBCD, SUBX, STOP,
    TRAP, and TRAPV instructions
- VDP
  - Slot-based rendering
  - Interlacing in Interlace Mode 2
  - Edgecase when the sprite mask is the first sprite rendered on the scanline
  - DMA copy
  - Mode 4
- Z80
- FM
- PSG
- SRAM
- Cartridge mappers
- Joypads
  - Having more than two joypads connected at once
  - Types of joypad besides the standard 3-button Mega Drive controller
- Remaining quirks and undefined behaviour of the original Mega Drive hardware

## Frontend
- Save states
- Input rebinding
- Controller support
- Reset hotkey
