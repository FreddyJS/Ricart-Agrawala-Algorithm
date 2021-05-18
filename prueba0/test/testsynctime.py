import os, sys, time
import shlex
import subprocess, signal
from subprocess import PIPE, run

bash = "make synctime"
os.system(bash)

bash = "rm -r logs"
os.system(bash)

bash = "mkdir logs"
os.system(bash)

def check_file(filename, nlines):
    try:
        log = open("%s" % (filename))
        lines = log.readlines()

        if (len(lines) >= nlines):
            return True

        log.close()
    except IOError:
        print("Test Failed! The file %s not exists!" % (filename))
        return False

    print("Test Failed! The file not have the requested lines!")
    return False

def test_code(nodes, process, wait_time, filename, lines):
    test = "./init %i %i" % (nodes, process) # ejecuta 1nodo 5 procesos

    print("Testing Inercia Time... Nodes: %i, Process: %i" % (nodes,process))            
    running = subprocess.Popen(shlex.split(test), shell=False, stdin=PIPE)
            
    print("Waiting %f secs... Process %i" % (wait_time, running.pid))
    time.sleep(wait_time)

    running.communicate(input=b'q') #introduce q a stdin del proceso

    if (check_file(filename, lines)):
        return True

    return False

nodes = 0

while (nodes != 5):
    nodes = nodes+1

    process = 0
    wait_time = 1 
    while(process != 100):
        process = process+5
        
        filename = "logs/waited%in%ip.log" % (nodes, process)

        tries = 0

        while(tries != 10):
            passed = False
            tries = tries+1
            while(not passed):
                passed = test_code(nodes, process, wait_time, filename, tries)
                if (not passed):
                    wait_time = wait_time+1

        log = open(filename)
        lines = log.readlines()

        sum = 0
        n = 0
        for line in lines:
            n = n+1
            sum = sum + int(line)
        log.close()

        log = open("plots/salida%in.plot" % (nodes), 'a')
        log.write("%i"%(process) + " " + str(sum/n) + '\n')
        log.close()

        print("\nTest Results. Nodes: %i, Process: %i, AvgInercia: %fms" % (nodes, process, sum/n))

print("Test Finished!")