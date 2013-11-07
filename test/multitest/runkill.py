#!/usr/bin/env python

import sys,os
import subprocess as sp
import time

spiffy_cmd = "export SPIFFY_ROUTER=127.0.0.1:12345 && "
sim_cmd = "perl ./hupsim.pl -m topo.map -n nodes.map -p 12345 -v 2"

A_cmd = "./peer -p nodes.map -c etc/A.haschunks -f etc/C.masterchunks -m 4 -i "
B_cmd = "./peer -p nodes.map -c etc/B.haschunks -f etc/C.masterchunks -m 4 -i "

if len(sys.argv) < 2:
    exit()

cmddicts = {
        "0": sim_cmd,
        "1": A_cmd + "1",
        "2": B_cmd + "2",
        "3": A_cmd + "3",
        "4": B_cmd + "4",
        }

cmd = (cmddicts[sys.argv[1]]).split()
pid = sp.Popen(cmd)
time.sleep(5)
print "kill it"
pid.terminate()
pid.wait()
time.sleep(10)
sp.check_call(spiffy_cmd + cmddicts[sys.argv[1]], shell=True)



# GET etc/B.chunks verytemp
