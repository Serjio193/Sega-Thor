"""Bounded mirror of the existing BO graphics grammar for Carver candidates."""


def _decode_graphics(rom, start, limit, output_cap=0x200000):
    position = start
    output = bytearray()
    work = 0

    def read_byte():
        nonlocal position, work
        work += 1
        if work > 4096 or position >= limit:
            raise ValueError("bounded source read")
        value = rom[position]
        position += 1
        return value

    def read_u16():
        return read_byte() | (read_byte() << 8)

    def write(value):
        nonlocal work
        work += 1
        if work > 4096 or len(output) >= output_cap:
            raise ValueError("bounded decoder output")
        output.append(value)

    def copy_match(distance, count):
        if distance == 0 or distance > len(output):
            raise ValueError("invalid back reference")
        for _ in range(count):
            write(output[-distance])

    def command_stream():
        while True:
            block_end = position + read_u16()
            if block_end > limit:
                raise ValueError("block boundary")
            while position < block_end:
                command = read_byte()
                if command & 0x80:
                    command &= 0x7F
                    count = ((command & 0x60) >> 5) + 4
                    distance = ((command & 0x1F) << 8) | read_byte()
                    copy_match(distance, count)
                    while position < block_end and rom[position] & 0xE0 == 0x60:
                        extension = read_byte() & 0x1F
                        copy_match(distance, extension or 256)
                    continue
                repeat = bool(command & 0x40)
                command &= 0x3F
                if repeat:
                    extended = bool(command & 0x10)
                    command &= ~0x10
                    counter = command
                    if extended:
                        counter = (counter << 8) | read_byte()
                    value = read_byte()
                    for _ in range(counter + 4):
                        write(value)
                    continue
                extended = bool(command & 0x20)
                command &= 0x1F
                count = command
                if extended:
                    count = (count << 8) | read_byte()
                for _ in range(count or 65536):
                    write(read_byte())
            if position != block_end:
                raise ValueError("command boundary")
            if read_byte() == 0:
                return

    def bit_stream():
        nonlocal position
        while True:
            if position + 3 > limit:
                raise ValueError("bit header")
            position += 3
            bits = read_byte()
            remaining = 8

            def read_bit():
                nonlocal bits, remaining
                remaining -= 1
                if remaining < 0:
                    bits = read_byte() | (read_byte() << 8)
                    remaining = 15
                value = bits & 1
                bits >>= 1
                return value

            def read_code(count):
                value = 0
                for _ in range(count):
                    value = (value << 1) | read_bit()
                return value

            while True:
                if not read_bit():
                    write(read_byte())
                    continue
                if not read_bit():
                    distance = read_byte()
                else:
                    distance = (read_code(5) << 8) | read_byte()
                    if distance == 0:
                        break
                    if distance == 1:
                        extended = read_bit()
                        count = read_code(4)
                        if extended:
                            count = (count << 8) | read_byte()
                        value = read_byte()
                        for _ in range(count + 14):
                            write(value)
                        continue
                if read_bit():
                    if read_bit():
                        if read_bit():
                            if read_bit():
                                if read_bit():
                                    count = read_code(3) + 6
                                else:
                                    count = read_byte() + 14
                            else:
                                count = 5
                        else:
                            count = 4
                    else:
                        count = 3
                else:
                    count = 2
                copy_match(distance, count)
            if read_byte() == 0:
                return

    if start + 4 > limit:
        raise ValueError("short stream")
    if rom[start + 2]:
        command_stream()
        mode = "command"
    else:
        bit_stream()
        mode = "bit"
    return {"start": start, "end": position, "consumed": position - start,
            "output_size": len(output), "mode": mode}


def decoder_spans(rom, start, end):
    """Scan every even candidate start and retain deterministic unique spans."""
    hits = {}
    starts = 0
    for position in range(start, max(start, end - 3), 2):
        starts += 1
        if rom[position + 2] and int.from_bytes(rom[position:position + 2], "little") < 4:
            continue
        if not rom[position + 2] and not any(rom[position + 3:position + 8]):
            continue
        try:
            hit = _decode_graphics(rom, position, end)
        except (IndexError, ValueError):
            continue
        if hit["consumed"] < 4 or hit["output_size"] < 8:
            continue
        key = (hit["start"], hit["end"], hit["output_size"], hit["mode"])
        hits[key] = hit
    ordered = [hits[key] for key in sorted(hits)]
    return {"starts_scanned": starts, "valid_hits": len(ordered),
            "spans": ordered[:128], "truncated": len(ordered) > 128}
