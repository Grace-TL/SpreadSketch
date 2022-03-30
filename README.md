# SpreadSketch

---
### Paper
__Lu Tang, Qun Huang, and Patrick P. C. Lee.__
SpreadSketch: Toward Invertible and Network-Wide Detection of Superspreaders.
_INFOCOM 2020_

---
### Files
- spreadsketch.hpp, spreadsketch.cpp: the implementation of SpreadSketch
- main\_ss.cpp: example about superspreader detection
- spreadsketch\_lc.hpp spreadsketch\_lc.cpp: SpreadSketch with Linear Counting
- spreadsketch\_hll.hpp spreadsketch\_hll.cpp: SpreadSketch with HyperLogLog
- spreadsketch\_kmv.hpp spreadsketch\_kmv.cpp: SpreadSketch with K-Minimal Values 
- p4/spreadsketch.p4: p4 implementation of SpreadSketch
- P4/hp\_spreadsketch.p4: p4 implementation of HP-SpreadSketch
---

### Compile and Run the examples
SpreadSketch is implemented with C++. We show how to compile the examples on
Ubuntu with g++ and make.

#### Requirements
- Ensure __g++__ and __make__ are installed.  Our experimental platform is
  equipped with Ubuntu 14.04, g++ 4.8.4 and make 3.81.

- Ensure the necessary library libpcap is installed.
    - It can be installed in most Linux distributions (e.g., apt-get install
      libpcap-dev in Ubuntu).

- Prepare the pcap files.
    - We provide two small pcap files
      [here](https://drive.google.com/file/d/1WLEjB-w4ZlNshl1vUMb98rrowFuMBWuJ/view?usp=sharing).
      You can download and put them in the "traces" folder for testing.  
    - Specify the path of each pcap file in "iptraces.txt". 
    - Note that one pcap file is regarded as one epoch in our examples. 

#### Compile
- Compile examples with make
    - Run the example of SpreadSketch 
```
    $ make main_ss
```
    - Run the example of HP-SpreadSketch

```
    $ make main_ss_hp
```

#### Run
- Run the examples, and the program will output some statistics about the detection accuracy. 

```
$ ./main_ss
```

- Note that you can change the configuration of SpreadSketch, e.g. number of rows and buckets in the example source code for testing.

#### Other distinct counter
- Run SpreadSketch with linear counting

```
$ make main_lc
$ ./main_lc
```

- Run SpreadSketch with hyperloglog

```
$ make main_hll
$ ./main_hll
```

- Run SpreadSketch with KMV

```
$ make main_kmv
$ ./main_kmv
```



