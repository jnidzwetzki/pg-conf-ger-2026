# Supplemental material for my talk at PostgreSQL Conf Ger 2026 

This repository contains the supplemental material for my talk ['Profiling PostgreSQL: perf, Flame Graphs, and eBPF Tools in Practice
'](https://www.postgresql.eu/events/pgconfde2026/schedule/session/7538-profiling-postgresql-perf-flame-graphs-and-ebpf-tools-in-practice/) at PostgreSQL Conf Ger 2026.

```sql
CREATE EXTENSION pg_slow;
CREATE TABLE data(value INT);
INSERT INTO data(value) SELECT generate_series(1, 10000);
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