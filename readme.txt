Hello!

This project uses the ChampSim simulator as configured for the Cache Replacement Championship 2. All simulations for this project were performed on an x86 system running Gentoo 2.9. Please download the traces for this simulator at https://crc2.ece.tamu.edu/. Specifics about the project's intent and findings are discussed in the PDF report and PowerPoint presentation in the main directory. These simulations make use of the SPEC2006 CPU benchmarks for single and multicore simulations and the Cloudsuite traces for multi-core simulations. These traces must be placed in the "traces" directory inside of the "Project" directory.

Follow any installaton instructions for any dependencies required by ChampSim. After this is done, the ChampSim framework is included (without traces) on this repository. The project as configured includes a set of scripts to enable easy use of the ChampSim simulator. The src folder contains all source files implementing the various cache replacement policies evaluated. These are in the form of C++ files. These files may be edited to make changes to any of the cache replacement policies included.

After editing, these source files may be compiled for use in ChampSim. To do this, a Python program named compile.py is included in the "ChampSim_CRC2" directory inside the of "Project" directory. This program is run as such;

python compile.py <filename in src directory> <configuration>

where the filename is the name of the file in src (a .cc extension is implied) and configuration is either configuration 1 (single core without prefetchers) or configuration 3 (multicore without prefetchers) of the CRC2 simulator.

The output files are placed in the "bin" directory, where they may be run. The format of the output file is <filename>-<configuration>, so that dra-rrip.cc compiled for configuration 1 will be compiled to an output file named dra-rrip-config1 and dra-rrip8MB.cc compiled for configuration 3 is compiled to an output file name dra-rrip-config3.

The RunScripts directory contains three executable files to make running simulations trivial. The run_traces, run_mp_traces, and run_cloud_traces all enable the user to run the ChampSim simulator for benchmarks with the cache replacement policy set as the one implemented in the previously compiled script. Note that run_traces will only run ChampSim for a cache replacement policy compiled for configuration 1 and run_mp_traces and run_cloud_traces will only run policies compiled fo configuraiton 3. run_traces and run_mp_traces run the cache replacement policy for SPEC2006 CPU benchmarks and run_cloud_traces will do it for Cloudsuite traces. The files should be used as such:

./run_traces dra-rrip-config1
./run_mp_traces dra-rrip8MB-config3
./run_cloud_traces dra-rrip8MB-config3

After simulations are complete, the result files are placed in the "results" directory. The results may be parsed with scripts included in the results directory: get_crc_results, analyze_mp.py, and analyze_cloud.py. get_crc_results is a bash script that will collect and tabulate the performance of each benchmark in terms of speedup over LRU. It may be called as such:cat /etc/os-release

./get_crc_traces <configuration> <original filename>

For example, if you want to collect the results for dra-rrip compiled for configuration 1, you would call:

./get_crc_traces config1 dra-rrip

You may also use multiple arguments for multiple policies to be compared, like if you wanted to compare drrip and dra-rrip:

./get_crc_traces config1 "dra-rrip drrip"

The last two files are Python scripts created to parse multicore result files. Note that they output the cumulative IPC of each group simulated and the individual IPCs of each core simulated and not the speedup over LRU as in the previous script. Parsed results are output to a filenamed <original filename>-cloudstats.txt or <original filename>-mpstats.txd for analyze_cloud and analyze_mp respectively. These results are then printed to the user. Multiple arguments are not supported. They may be called as such:

python analyze_mp.py <original filename>
python analyze_cloud.py <original filename>

For example, to collect data for dra-rrip-8MB running the Cloudsuite benchmarks, the following command should be run:

python analyze_cloud.py dra-rrip-8MB

Please note that the base directory for each of the shell scripts must be adjusted to suit your system's filepath.



