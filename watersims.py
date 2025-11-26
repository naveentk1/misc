import sys
import time

block = "█"
height = 20

# Move cursor up using ANSI codes
for i in range(height, -1, -1):
    sys.stdout.write("\x1b[2K")  # clear line
    sys.stdout.write(f"\033[{i};15H")  # move cursor to row i, column 15
    sys.stdout.write(block)
    sys.stdout.flush()
    time.sleep(0.12)

print("\n\n   Done! The block went all the way up! 🎉")