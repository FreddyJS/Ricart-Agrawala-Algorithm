import os, sys, time
import shlex
import subprocess, signal
from subprocess import PIPE, run

bash = "make"
os.system(bash)

bash = "rm -r logs"
os.system(bash)

bash = "mkdir logs"
os.system(bash)

nodes = 0
wait_time = 60 

while (nodes != 5):
    nodes = nodes+1

    process = 0
    while(process != 100):
        process = process+5

        test = "./init %i %i" % (nodes, process) # ejecuta 1nodo 5 procesos
        
        print("\nTesting Sync Time... Nodes: %i, Process: %i" % (nodes,process))
            
        running = subprocess.Popen(shlex.split(test), shell=False, stdin=PIPE)
            

        print("\nWaiting %f secs... Process %i" % (wait_time, running.pid))
        time.sleep(wait_time)

        running.communicate(input=b'q') #introduce q a stdin del proceso
            
        time.sleep(0.5)

        type = 0
        while(type != 6):
            type = type+1
            
            try:
                log = open("logs/times%in%ip%it.log" % (nodes, process, type))
                lines = log.readlines()

                sum = 0
                n = 0
                for line in lines:
                    n = n+1
                    sum = sum + int(line)
                log.close()

                log = open("plots/times%in%it.plot" % (nodes, type), 'a')
                log.write("%i" % (process) + " " + str(sum) + '\n')
                log.close()

            except IOError:
                log = open("plots/times%in%it.plot" % (nodes, type), 'a')
                log.write("%i" % (process) + " " + str(0) + '\n')
                log.close()

            
        print("\nTest Results. Nodes: %i, Process: %i, AvgSyncTime: %fms" % (nodes, process, sum/n))

print("Test Finished!")