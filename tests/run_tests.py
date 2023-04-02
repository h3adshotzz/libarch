import os
import argparse

LIBARCH = ""
RADARE2 = ""
TEST_FILE = ""


def parse_test_file(file):
    opcodes = []
    with open (file, "r") as stream:
        data = stream.read().split("\n")[2:]
        for l in data:
            sl = l.split("-")[0].strip(" ")
            if sl: opcodes.append(sl)

    return opcodes

if __name__ == "__main__":
    print ("hello world")

    parser = argparse.ArgumentParser()
    parser.add_argument('-l', '--libarch', action="store")
    parser.add_argument('-r', '--radare2', action="store")
    parser.add_argument('-t', '--testfile', action="store")

    args = parser.parse_args()
    print (args)

    opcodes = parse_test_file (args.testfile)

    for op in opcodes:

        # radare2
        r2_dat = os.popen("{} {} {}".format(args.radare2, "-a arm -b 64 -A -d", op)).read().strip()

        # libarch
        la_dat = os.popen("{} {}".format(args.libarch, op)).read().strip()

        print ("** TEST: {} **".format(op))
        print ("r2: {}".format(r2_dat))
        print ("la: {}\n".format(la_dat))

