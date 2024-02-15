import os
from datetime import datetime

Import("env")

current_time = datetime.now()
formatted_date = current_time.strftime("%Y.%m.%d")
formatted_time = hex(int(current_time.strftime("%H%M%S")))[2:].zfill(5)

FILE_NAME = "src/main.h"
SEEK_TEMPLATE = "#define SLIPPY_VERSION"
VERSION_TEMPLATE = f"#define SLIPPY_VERSION \"{formatted_date}-{formatted_time}\"" # Version string format "yyyy.mm.dd-HEX(hh+mm+ss)"


def set_version_string():
    with open(FILE_NAME, 'r') as file:
        lines = file.readlines()

    for i, line in enumerate(lines):
        if SEEK_TEMPLATE in line:
            lines[i] = VERSION_TEMPLATE+'\n'

    with open(FILE_NAME, 'w') as file:
        file.writelines(lines)

set_version_string()