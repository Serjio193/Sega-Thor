from pathlib import Path

rom = Path("local-roms/Beyond Oasis (USA).md").read_bytes()

def show_words(addr, count):
    words = [f"{int.from_bytes(rom[addr+i*2:addr+i*2+2], 'big'):04X}" for i in range(count)]
    print(f"ROM 0x{addr:06X}: {' '.join(words)}")

show_words(0x0032EE, 8)
show_words(0x03A9AC, 8)
show_words(0x060310, 8)
show_words(0x00E284, 8)
show_words(0x002230, 8)
show_words(0x002600, 8)
