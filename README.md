# susa

## About
An inobscure implementation of a Hashtable-based filesystem (Constant-Time\* file/directory look-up) using the FUSE (Filesystem in Userspace) interface.

\* This is as a best-case, since collisions are handled using a linear-search, and thus slows down performance.  This can be managed by dynamically resizing the Hashtable as determined by a given policy.

## Persistance & (Un-)Loading Schemes
### Demand Loading
### Policy-Based Writes

## Efficiency
### Memory & Disk usage
### Time-Complexity

## Build, Mount, & Use.

The first step is to download the source code and
make it.

```
$ git clone https://github.com/frederikgram/susa
$ cd src
$ make
```

Hereafter it is recommended to use the filesystem via. Temporary mounting to /tmp/xxx
```
$ mkdir /tmp/susa-filesystem
$ chmod 755 /tmp/susa-filesystem
$ cd src
$ ./susa ./tmp/susa-filesystem
$ cd /tmp/susa-filesystem
```
From here you can either use the terminal as usual, or a graphical file-explorer such as [nautilus](https://manpages.ubuntu.com/manpages/bionic/man1/nautilus.1.html)
