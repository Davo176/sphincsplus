#! /usr/bin/env python3
import fileinput
import itertools
import os
import sys
from subprocess import DEVNULL, run

implementations = [
                   ('ref', ['shake', 'sha2', 'haraka']),
                   ]

options = ["f", "s"]
sizes = [128, 192, 256]
thashes = ['robust']

for impl, fns in implementations:
    params = os.path.join(impl, "params.h")
    for fn in fns:
        for opt, size, thash in itertools.product(options, sizes, thashes):
            paramset = "sphincs-{}-{}{}".format(fn, size, opt)
            paramfile = "params-{}.h".format(paramset)

            params = 'PARAMS={}'.format(paramset)  # overrides Makefile var
            thash = 'THASH={}'.format(thash)  # overrides Makefile var
            print(f'make -C {impl} {thash} {params}')

            run(["make", "checkSigned", "-C", impl, thash, params],
                stdout=DEVNULL, stderr=sys.stderr)
            run([f'./ref/checkSigned'])
            run(["rm","ref/checkSigned"])
        