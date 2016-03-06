usage
=====

Prints resource usage of processes to stderr.  Currently Linux only.  Compile with
```bash
./compile.sh
```

Then use it:
```bash
./measure.sh true
```
You can also manually set `LD_PRELOAD` in the current shell:
```bash
export LD_PRELOAD=$PWD/libusage.so:/usr/lib64/librt.so
cat /dev/null
```

The fields are tab delimited.  Memory is measured in kB.  Time is measured in seconds.  CPU is the sum of user and system time.  Most fields apply to the process itself, but children (and not the process itself) are reported in fields that begin with `Child`.

# Rationale
The normal way to collect process statistics is to fork, exec, wait for SIGCHLD, and reap children with waitpid.  However, zombie processes do not have virtual memory statistics in `/proc/$pid/status`.  The `LD_PRELOAD` mechanism provides a way to inject code into dynamically linked processes so that statistics can be collected immediately before exit.  

# Bugs
This will not work on programs with static linkage, setuid/setgid, or programs that need specific file descriptor numbers.  Since some programs close stderr, fd 2 is duplicated at the beginning.  See the limitations of `getrusage`. 
