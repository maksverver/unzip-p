#!/usr/bin/env python3

import sys
from zipfile import ZipFile, ZIP_DEFLATED

with ZipFile('zip64.zip', mode='w', compression=ZIP_DEFLATED) as z:
    for filename in ['hello.txt', 'lore-ipsum.txt']:
        with z.open(filename, 'w', force_zip64=True) as zf:
            with open(filename, 'rb') as f:
                zf.write(f.read())
