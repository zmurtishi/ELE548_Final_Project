
import os
import sys

def isFloat(num):
    try:
        int(num)
        if(int(num) < float(num)):
            return True
        else:
            return False
    except ValueError:
        return False

def findHit(txt):
    last = 0
    nums = [float(s) for s in txt.split() if isFloat(s)]
    print(nums)
    return str(nums)

def main():
   args = sys.argv[1]
   filesToSearch = ["group2","group3","group4","group5","group6"]
   #filesToSearch = ["400.perlbench", "416.gamess", "435.gromacs", "445.gobmk", "454.calculix", "462.libquantum", "471.omnetpp", "483.xalancbmk", "401.bzip2", "429.mcf", "436.cactusADM", "447.dealII", "456.hmmer", "464.h264ref", "473.astar", "403.gcc", "433.milc", "437.leslie3d", "450.soplex", "458.sjeng", "465.tonto", "481.wrf", "410.bwaves", "434.zeusmp", "444.namd", "453.povray", "459.GemsFDTD", "470.lbm", "482.sphinx3"]
   #filesToSearch = ["400.perlbench" "416.gamess" "435.gromacs" "445.gobmk" 454.calculix 462.libquantum 471.omnetpp 483.xalancbmk 401.bzip2 429.mcf 436.cactusADM 447.dealII 456.hmmer 464.h264ref 473.astar 403.gcc 433.milc 437.leslie3d 450.soplex 458.sjeng 465.tonto 481.wrf 410.bwaves 434.zeusmp 444.namd 453.povray 459.GemsFDTD 470.lbm 482.sphinx3]
   fileToSave = open(args+"-cloudstats.txt", "w")
   #fileToSave.write(" \n")
   hits = 0
   for x in filesToSearch:
       newString = "cloud"+x+"_"+args+"-config3"
       f = open(newString, "r")
       for y in f:
           if(y.find("Cumulative IPC") > -1):
               fileToSave.write(y)
               #hits = hits + 1
               #if(hits == 4):
               #    hits = 0;
               #    break
               #fileToSave.write("\n")
       f.close()
   fileToSave.close()
   os.system("cat "+args+"-cloudstats.txt")

if __name__ == "__main__":
    main()
