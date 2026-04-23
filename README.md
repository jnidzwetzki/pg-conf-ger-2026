# Supplemental material for my talk at PostgreSQL Conf Ger 2026 

This repository contains the supplemental material for my talk ['Profiling PostgreSQL: perf, Flame Graphs, and eBPF Tools in Practice
'](https://www.postgresql.eu/events/pgconfde2026/schedule/session/7538-profiling-postgresql-perf-flame-graphs-and-ebpf-tools-in-practice/) at PostgreSQL Conf Ger 2026. You can find the slides for the talk in the [slides](slides) directory.

## Source Code
* `pg_slow` can be found in [`src/pg_slow`](src/pg_slow) directory. 
* A test workload to generate a simple flame graph can be found in [`src/workload`](src/workload) directory.

## Environment
```sql
CREATE EXTENSION pg_slow;
CREATE TABLE data(value INT);
INSERT INTO data(value) SELECT generate_series(1, 50000);
```

```sql
db1=# \timing on
Timing is on.
db1=# SELECT value, is_odd_slow(value) FROM data;
 value | is_odd_slow 
-------+-------------
     1 | t
     2 | f
     3 | t
     4 | f
     5 | t
     6 | f
     7 | t
[...]
 49994 | f
 49995 | t
 49996 | f
 49997 | t
 49998 | f
 49999 | t
 50000 | f
(50000 rows)
Time: 41194.986 ms (00:41.195)
```

```sql
db1=# EXPLAIN (VERBOSE, ANALYZE) SELECT value, (value%2)::boolean FROM data;
                                                       QUERY PLAN                                                        
-------------------------------------------------------------------------------------------------------------------------
 Seq Scan on public.data  (cost=0.00..16925.00 rows=50000 width=8) (actual time=0.057..85.583 rows=50000.00 loops=1)
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
                              
----------------------------------------------------------------------------------------
------------------------------
 Seq Scan on public.data  (cost=0.00..847.00 rows=50000 width=5) (actual time=0.165..407
74.297 rows=50000.00 loops=1)
   Output: value, is_odd_slow(value)
   Buffers: shared hit=222
 Planning Time: 0.898 ms
 Execution Time: 40775.788 ms
(5 rows)

Time: 40784.256 ms (00:40.784)
```

```sql
db1=# EXPLAIN (VERBOSE, ANALYZE) SELECT value, is_odd_fast(value) FROM data;
                                                    QUERY PLAN                                                     
-------------------------------------------------------------------------------------------------------------------
 Seq Scan on public.data  (cost=0.00..847.00 rows=50000 width=5) (actual time=0.072..10.066 rows=50000.00 loops=1)
   Output: value, is_odd_fast(value)
   Buffers: shared hit=222
 Planning Time: 0.512 ms
 Execution Time: 13.089 ms
(5 rows)
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

```
db1=# SELECT pg_backend_pid();
 pg_backend_pid 
----------------
          59514
(1 row)
```

## Flame Graphs
```C
int main(int argc, char *argv[]) {
   
    for (int i = 0; i < iterations; i++) {
        /* Burn some CPU cycles */
        func1(i);
        
        /* func2 takes 2 times more cycles than func1 */
        func2(i);

        /* func3 takes 2 times more cycles than func2 */
        /* It calls func3a internally that needs      */
        /* ~50% of the CPU cycles.                    */
        func3(i);
    }

    return 0;
}
```


## Funccount

```
$ sudo funccount-bpfcc /usr/local/postgresql-18.3/lib/pg_slow.so:'is_odd_slow|is_odd_1|is_odd_2|is_odd_3'
FUNC                                    COUNT
is_odd_1                                16666
is_odd_2                                16667
is_odd_3                                16667
is_odd_slow                             50000
Detaching...
```

## Function latency
```
vscode ➜ /workspaces/pg-conf-ger-2026/src/pg_slow (main) $ sudo funclatency-bpfcc /usr/local/postgresql-18.3/lib/pg_slow.so:'is_odd_1'
Tracing 1 functions for "/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_1"... Hit Ctrl-C to end.
^C

Function = [unknown] [181624]
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
       512 -> 1023       : 0        |                                        |
      1024 -> 2047       : 0        |                                        |
      2048 -> 4095       : 0        |                                        |
      4096 -> 8191       : 8        |                                        |
      8192 -> 16383      : 21       |                                        |
     16384 -> 32767      : 62       |                                        |
     32768 -> 65535      : 386      |*                                       |
     65536 -> 131071     : 1315     |******                                  |
    131072 -> 262143     : 2323     |***********                             |
    262144 -> 524287     : 4002     |********************                    |
    524288 -> 1048575    : 7981     |****************************************|
   1048576 -> 2097151    : 539      |**                                      |
   2097152 -> 4194303    : 23       |                                        |
   4194304 -> 8388607    : 6        |                                        |

avg = 551582 nsecs, total: 9192671940 nsecs, count: 16666

Detaching...

Tracing 1 functions for "/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_2"... Hit Ctrl-C to end.
^C
Function = [unknown] [194278]
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
       512 -> 1023       : 12159    |****************************************|
      1024 -> 2047       : 1154     |***                                     |
      2048 -> 4095       : 11       |                                        |
      4096 -> 8191       : 4        |                                        |
      8192 -> 16383      : 2        |                                        |
     16384 -> 32767      : 4        |                                        |
     32768 -> 65535      : 0        |                                        |
     65536 -> 131071     : 0        |                                        |
    131072 -> 262143     : 0        |                                        |
    262144 -> 524287     : 3047     |**********                              |
    524288 -> 1048575    : 286      |                                        |

avg = 101829 nsecs, total: 1697188060 nsecs, count: 16667

Tracing 1 functions for "/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_3"... Hit Ctrl-C to end.
^C

Function = [unknown] [170697]
               nsecs                         : count     distribution
                   0 -> 1                    : 0        |                    |
                   2 -> 3                    : 0        |                    |
                   4 -> 7                    : 0        |                    |
                   8 -> 15                   : 0        |                    |
                  16 -> 31                   : 0        |                    |
                  32 -> 63                   : 0        |                    |
                  64 -> 127                  : 0        |                    |
                 128 -> 255                  : 0        |                    |
                 256 -> 511                  : 0        |                    |
                 512 -> 1023                 : 15088    |********************|
                1024 -> 2047                 : 1224     |*                   |
                2048 -> 4095                 : 326      |                    |
                4096 -> 8191                 : 15       |                    |
                8192 -> 16383                : 10       |                    |
               16384 -> 32767                : 0        |                    |
               32768 -> 65535                : 1        |                    |
               65536 -> 131071               : 0        |                    |
              131072 -> 262143               : 0        |                    |
              262144 -> 524287               : 0        |                    |
              524288 -> 1048575              : 0        |                    |
             1048576 -> 2097151              : 0        |                    |
             2097152 -> 4194303              : 0        |                    |
             4194304 -> 8388607              : 0        |                    |
             8388608 -> 16777215             : 0        |                    |
            16777216 -> 33554431             : 0        |                    |
            33554432 -> 67108863             : 0        |                    |
            67108864 -> 134217727            : 0        |                    |
           134217728 -> 268435455            : 0        |                    |
           268435456 -> 536870911            : 0        |                    |
           536870912 -> 1073741823           : 0        |                    |
          1073741824 -> 2147483647           : 0        |                    |
          2147483648 -> 4294967295           : 0        |                    |
          4294967296 -> 8589934591           : 0        |                    |
          8589934592 -> 17179869183          : 3        |                    |

avg = 1802748 nsecs, total: 30046405110 nsecs, count: 16667

```

## Flame Graphs

```bash
perf record -g -F 111 -o data.perf -p 54221
perf script -i data.perf > data.stacks
~/FlameGraph/stackcollapse-perf.pl data.stacks > data.folded
~/FlameGraph/flamegraph.pl data.folded > data.svg
```

## Offline Flame Graphs
```bash
sudo offcputime-bpfcc -df -p 1955 > out.stacks
~/FlameGraph/flamegraph.pl --color=io --title="Off-CPU Time Flame Graph" --countname=us < out.stacks > out.svg
```

## BPFTrace
```
sudo bpftrace -e '
uprobe:/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_2
{
  @start[tid] = nsecs;
  @v[tid] = arg0;
}

uretprobe:/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_2
/@start[tid]/
{
  $dur = nsecs - @start[tid];
  $v = @v[tid];

  /* Assign input into 10000-wide buckets */
  $bucket = ((uint64)$v / 10000) * 10000;

  @count[$bucket] = count();
  @avg_ns[$bucket] = avg($dur);

  delete(@start[tid]);
  delete(@v[tid]);
}

interval:s:5
{
  print(@count);
  print(@avg_ns);
}
'
```

```
@count[0]: 3333
@count[30000]: 3333
@count[20000]: 3333
@count[10000]: 3334
@count[40000]: 3334
@avg_ns[10000]: 924
@avg_ns[20000]: 1060
@avg_ns[30000]: 1158
@avg_ns[0]: 1246
@avg_ns[40000]: 534634
```

```
sudo bpftrace -e '
uprobe:/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_1 { 
  printf("function enter\n"); 
}

uretprobe:/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_1 { 
  printf("function exit\n"); 
}
'
```

```
sudo bpftrace -e '
uprobe:/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_1 { 
  printf("function enter\n"); 
}

uretprobe:/usr/local/postgresql-18.3/lib/pg_slow.so:is_odd_1 { 
  printf("function exit\n"); 
}
'
Attaching 2 probes...
function enter
function exit
function enter
function exit
```

## Problem with inlining

### Release build
```
Tracing 4 functions for "b'/home/jan/postgresql-sandbox/bin/REL_17_1_RELEASE/lib/pg_slow.so:is_odd_slow|is_odd_1|is_odd_2|is_odd_3'"... Hit Ctrl-C to end.
^C
FUNC                                    COUNT
is_odd_1                                16665
is_odd_slow                             49995
Detaching...
```


## Relase and static
-fvisibility=hidden

```
Tracing 1 functions for "b'/home/jan/postgresql-sandbox/bin/REL_17_1_RELEASE/lib/pg_slow.so:is_odd_slow|is_odd_1|is_odd_2|is_odd_3'"... Hit Ctrl-C to end.
^C
FUNC                                    COUNT
is_odd_slow                                 8
Detaching...
```

```
jan@debian-arm64:~/git/pg-conf-ger-2026 (main)$ sudo bpftrace -e 'uprobe:/home/jan/postgresql-sandbox/bin/REL_17_1_RELEASE/lib/pg_slow.so:is_odd_1 { printf("function hit\n"); }'
Attaching 1 probe...
ERROR: Could not resolve symbol: /home/jan/postgresql-sandbox/bin/REL_17_1_RELEASE/lib/pg_slow.so:is_odd_1, cannot attach probe.
```

```
jan@debian-arm64:~/git/pg-conf-ger-2026 (main)$ sudo bpftrace -e 'uprobe:/home/jan/postgresql-sandbox/bin/REL_17_1_DEBUG/lib/pg_slow.so:is_odd_1 { printf("function hit\n"); }'
Attaching 1 probe...
```

## Readelf
```
readelf --debug-dump=info /home/jan/postgresql-sandbox/bin/REL_17_1_RELEASE/lib/pg_slow.so | less

 <1><1180>: Abbrev Number: 15 (DW_TAG_subprogram)
    <1181>   DW_AT_name        : (indirect string, offset: 0x22b5): is_odd_1
    <1185>   DW_AT_decl_file   : 1
    <1186>   DW_AT_decl_line   : 81
    <1187>   DW_AT_decl_column : 6
    <1188>   DW_AT_prototyped  : 1
    <1188>   DW_AT_type        : <0x125>
    <118c>   DW_AT_inline      : 1      (inlined)
    <118d>   DW_AT_sibling     : <0x11c1>
```

Using `pg_noinline`

```
 <1><11da>: Abbrev Number: 17 (DW_TAG_subprogram)
    <11db>   DW_AT_name        : (indirect string, offset: 0x22b5): is_odd_1
    <11df>   DW_AT_decl_file   : 1
    <11df>   DW_AT_decl_line   : 82
    <11e0>   DW_AT_decl_column : 6
    <11e1>   DW_AT_prototyped  : 1
    <11e1>   DW_AT_type        : <0x125>
    <11e5>   DW_AT_low_pc      : 0x920
    <11ed>   DW_AT_high_pc     : 0x94
    <11f5>   DW_AT_frame_base  : 1 byte block: 9c       (DW_OP_call_frame_cfa)
    <11f7>   DW_AT_call_all_calls: 1
    <11f7>   DW_AT_sibling     : <0x12c9>
```

```
jan@debian-arm64:~/git/pg-conf-ger-2026 (main)$ sudo bpftrace -e 'uprobe:/home/jan/postgresql-sandbox/bin/REL_17_1_RELEASE/lib/pg_slow.so:is_odd_1 { printf("function hit\n"); }'
Attaching 1 probe...
function hit
function hit
```
