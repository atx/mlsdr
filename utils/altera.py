#! /usr/bin/env python3

import csv
from eeschema import *

lib = SchemaLib()
com = Component("10M08SCU169", nparts=7)
lib.add_component(com)

partbanks = {"PWR": 1, "JTAG": 2, "1A": 3, "1B": 3, "2": 4, "3": 5, "5": 6, "6": 7,  "8": 8}
ys = [0] * max(partbanks.values())

with open("tabula-10m08sc.csv") as inf:
    reader = csv.reader(inf)
    for row in reader:
        print(row)
        bank = row[0]
        name = row[2]
        pin = row[-1]
        if name == "VCCONE":
            name = "VCC_ONE"
        if not bank:
            if name in ["GND", "VCC_ONE"] or name.startswith("VCCA"):
                bank = "PWR"
            elif name.startswith("VCCIO"):
                bank = name[5:]
            else:
                assert False, name
        fullname = "/".join(filter(bool, [name, row[3], row[4]]))
        if any(s in fullname for s in ["JTAGEN", "TMS", "TCK", "TDI", "TDO",
                                       "DEV_", "CONF", "CRC", "nSTATUS"]):
            bank = "JTAG"
        partn = partbanks[bank]
        com.add_pin(Pin(fullname, pin, x=0, y=ys[partn - 1], part=partn, length=200))
        ys[partn - 1] += 100

lib.save("10m08scu169.lib")
