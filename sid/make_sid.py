#!/usr/bin/python3

import sys, os
import collections


if len(sys.argv) != 2:
    print("usage: %s data-file" % sys.argv[0])
    exit(1)

data = [[int(x, 16) for x in l.split()] for l in open(sys.argv[1])]


SCALE     = (1 << 24) / 985248
FRAME_LEN = 22050 // 50;


def get_noise(flags):
    t = (flags & 0b00000111) - 1
    print(t)
    return [
        (0xffff, 0xaa),
        (0xffff, 0x33),
        (0xffff, 0x66),
        (0xffff, 0x66),
        (0xffff, 0x66),
    ][t]


freq_slots = list(range(256))
freqs = collections.defaultdict(lambda: freq_slots.pop(0))


tmp = open("tmp.s", "w");
tmp.write(open("player.s").read())
tmp.write("data\n")

for i, row in enumerate(data):
    if i % 2 == 0: continue # skip every 2nd row
    flags, f1, f2, _, a1, a2, _, p = row

    xf0 = int(0.5 + 22050 / 3 / p * SCALE)
    xf1 = int(0.5 + 22050 / 3 * (f1 / 256) * SCALE)
    xf2 = int(0.5 + 22050 / 3 * (f2 / 256) * SCALE)


    xf0 *= a1 | a2 > 0
    xf1 *= a1 > 0
    xf2 *= a2 > 0

    q  = 0x00
    frame_count = 1

    if flags:
        xf2, q = get_noise(flags)

        if flags & 0b11111000:
            xf1 = 0
            length      = (flags & 0b11111000) * 8 + 8
            frame_count = max(1, (length + FRAME_LEN // 2) // FRAME_LEN)

    for _ in range(frame_count):
        tmp.write("\t.byt\t%s\n" % ", ".join("$%02X" % x for x in [
            freqs[xf0],
            freqs[xf1],
            freqs[xf2],
            q,
        ]))

for _ in range(20):
    tmp.write("\t.byt\t%s\n" % ", ".join("$%02X" % x for x in [
        freqs[0],
        freqs[0],
        freqs[0],
        0,
    ]))

tmp.write("dataend\n")


tmp.write("freqhi\n")
for f in sorted(freqs.keys(), key=freqs.__getitem__):
    tmp.write("\t.byt\t$%02x\n" % (f >> 8))
tmp.write("freqlo\n")
for f in sorted(freqs.keys(), key=freqs.__getitem__):
    tmp.write("\t.byt\t$%02x\n" % (f & 255))

tmp.close()
os.system("xa tmp.s -o out.sid")
#os.remove("tmp.s")
