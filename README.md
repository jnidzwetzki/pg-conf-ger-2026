# Supplemental material for my talk at PostgreSQL Conf Ger 2026 

This repository contains the supplemental material for my talk ['Profiling PostgreSQL: perf, Flame Graphs, and eBPF Tools in Practice
'](https://www.postgresql.eu/events/pgconfde2026/schedule/session/7538-profiling-postgresql-perf-flame-graphs-and-ebpf-tools-in-practice/) at PostgreSQL Conf Ger 2026.

```sql
CREATE EXTENSION pg_slow;
CREATE TABLE data(value INT);
INSERT INTO data(value) SELECT generate_series(1, 100000);
```

```sql
db1=# EXPLAIN (VERBOSE, ANALYZE) SELECT value, (value%2)::boolean FROM data;
                                                       QUERY PLAN                                                        
-------------------------------------------------------------------------------------------------------------------------
 Seq Scan on public.data  (cost=0.00..16925.00 rows=10000 width=8) (actual time=0.057..85.583 rows=10000.00 loops=1)
   Output: value, (value % 2)
   Buffers: shared hit=4425
 Planning Time: 0.112 ms
 Execution Time: 117.374 ms
(5 rows)

Time: 118.821 ms
```

```sql
db1=# EXPLAIN (VERBOSE, ANALYZE) SELECT value, is_odd_slow(value) FROM data;
                                                       QUERY PLAN                                                        
-------------------------------------------------------------------------------------------------------------------------
 Seq Scan on public.data  (cost=0.00..16925.00 rows=10000 width=5) (actual time=0.174..95.195 rows=10000.00 loops=1)
   Output: value, is_odd_slow(value)
   Buffers: shared hit=4425
 Planning Time: 0.353 ms
 Execution Time: 123.223 ms
(5 rows)

Time: 125.638 ms
```

```sql
db1=# SELECT value, (value%2)::boolean, is_odd_slow(value) FROM data LIMIT 5;
 value | bool | is_odd_slow 
-------+------+-------------
     1 | t    | t
     2 | f    | f
     3 | t    | t
     4 | f    | f
     5 | t    | t
(5 rows)
```

```sql
SELECT count(*) AS mismatch_count FROM data WHERE (value % 2)::boolean <> is_odd_slow(value);
```


## Side notes
```
Warning:
PID/TID switch overriding SYSTEM
Error:
Access to performance monitoring and observability operations is limited.
Consider adjusting /proc/sys/kernel/perf_event_paranoid setting to open
access to performance monitoring and observability operations for processes
without CAP_PERFMON, CAP_SYS_PTRACE or CAP_SYS_ADMIN Linux capability.
More information can be found at 'Perf events and tool security' document:
https://www.kernel.org/doc/html/latest/admin-guide/perf-security.html
perf_event_paranoid setting is 3:
  -1: Allow use of (almost) all events by all users
      Ignore mlock limit after perf_event_mlock_kb without CAP_IPC_LOCK
>= 0: Disallow raw and ftrace function tracepoint access
>= 1: Disallow CPU event access
>= 2: Disallow kernel profiling
To make the adjusted perf_event_paranoid setting permanent preserve it
in /etc/sysctl.conf (e.g. kernel.perf_event_paranoid = <setting>)
```


## Cliff
```
$ sudo funclatency-bpfcc /usr/local/postgresql-18.3/lib/pg_slow.so:'is_odd_cliff'
Tracing 1 functions for "/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_cliff"... Hit Ctrl-C to end.
^C

Function = [unknown] [33619]
     nsecs               : count     distribution
         0 -> 1          : 0        |                                        |
         2 -> 3          : 0        |                                        |
         4 -> 7          : 0        |                                        |
         8 -> 15         : 0        |                                        |
        16 -> 31         : 0        |                                        |
        32 -> 63         : 0        |                                        |
        64 -> 127        : 0        |                                        |
       128 -> 255        : 0        |                                        |
       256 -> 511        : 0        |                                        |
       512 -> 1023       : 2629     |****************************************|
      1024 -> 2047       : 363      |*****                                   |
      2048 -> 4095       : 6        |                                        |
      4096 -> 8191       : 0        |                                        |
      8192 -> 16383      : 2        |                                        |
     16384 -> 32767      : 0        |                                        |
     32768 -> 65535      : 0        |                                        |
     65536 -> 131071     : 0        |                                        |
    131072 -> 262143     : 331      |*****                                   |
    262144 -> 524287     : 2        |                                        |
```

sudo offcputime-bpfcc -df -p 1955 > out.stacks


sudo bpftrace -e '
uprobe:/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_cliff
{
  @start[tid] = nsecs;
  @v[tid] = arg0;
}

uretprobe:/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_cliff
/@start[tid]/
{
  $dur_us = (nsecs - @start[tid]) / 1000;
  $v = @v[tid];
  if ($v < 0) {
    $v = -$v;
  }

  /* Raster input into 10000-wide buckets: 0, 10000, 20000, ... */
  $bucket = ((uint64)$v / 10000) * 10000;

  @count[$bucket] = count();
  @avg_us[$bucket] = avg($dur_us);

  delete(@start[tid]);
  delete(@v[tid]);
}

interval:s:1
{
  print(@count);
  print(@avg_us);
}

END
{
  clear(@start);
  clear(@v);
}
'

sudo bpftrace -e '
uprobe:/home/jan/postgresql-sandbox/bin/REL_17_1_DEBUG/lib/pg_slow.so:is_odd_cliff
{
    @start[tid] = nsecs;
    @v[tid] = arg0;
}

uretprobe:/home/jan/postgresql-sandbox/bin/REL_17_1_DEBUG/lib/pg_slow.so:is_odd_cliff
/ @start[tid] != 0 /
{
    $dur_us = (nsecs - @start[tid]) / 1000;
    $v = @v[tid] < 0 ? -@v[tid] : @v[tid];
    $bucket = 0;
    if ($v < 0) { $bucket = 0; }
    if ($v > 1000) { $bucket = 1000; }

    @duration_by_bucket[$bucket] = avg($dur_us);
    @count_by_bucket[$bucket] += 1;
    delete(@start[tid]);
    delete(@v[tid]);
}

interval:s:10
{
    print(@count_by_bucket);
    print(@duration_by_bucket);
    clear(@count_by_bucket);
    clear(@duration_by_bucket);
}
'