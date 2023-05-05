import os
import sys

def main():
    args = sys.argv[1];
    args2 = sys.argv[2];
    os.system("g++ -Wall --std=c++11 -o bin/"+args+"-"+args2+" src/"+args+".cc lib/"+args2+".a")


if __name__ == "__main__":
    main()
