# Cells locality experiment

The native C experiment compares `w = 2`, `3`, and `4` on fork-heavy trees with shared suffixes. Each family contains two distinct prefix spines and one suffix spine. Both prefixes eventually reach the same suffix. Each visit reads a left slot containing `~` or a one-byte fruit and follows the right ref, so the benchmark visits each suffix twice. All six layouts traverse the same shuffled roots and verify the same visit count and checksum.

- **Grouped.** Place the two prefixes and their suffix together. Pack complete families into each base window. Evaluator roots carry their base, and every edge in a family is direct.
- **Interleaved.** Place shuffled suffixes first, then the two batches of prefixes. If the whole buffer fits a ref window, all edges are direct. For the larger buffers, each prefix gets a nearby long ref that jumps backward to the suffix's terminal long ref. The two prefixes share that terminal ref. Their first hops occupy separate records.

This models two placement policies for the same graph. Grouped placement has advance knowledge of which trees share a suffix. Interleaved placement deliberately separates those trees and gives each prefix its own base when the buffer exceeds one window. A dynamic allocator may place objects between these cases. The timed region includes the native traversal and checksum. Heap construction and hop counting run outside it. Byte counts cover the packed buffer. The external roots use the same separate space for each width, reported as `root_bytes`. This experiment uses only forks, leaves, and one-byte fruits. It measures timings and hop counts, with no cache, TLB, or GC counters.

## Reproduce

From the repository root, build and run on a little-endian host with a GCC-compatible compiler. The recorded runs used GCC 14.2 on an AMD EPYC 7763 x86-64 host pinned to CPU 2. Each `cpu-ns/fork` is the median of per-thread CPU time per visit. `range` is the minimum and maximum across trials. The experiment checks the results of all timed traversals and rotates the layout order between trials.

```sh
cc -O3 -std=c11 -Wall -Wextra -Wpedantic -Werror \
  artifacts/20261119-142158/cells_locality.c -o /tmp/cells-locality
taskset -c 2 /tmp/cells-locality 8192 16 15
taskset -c 2 /tmp/cells-locality 131072 16 15
taskset -c 2 /tmp/cells-locality 32768 64 15
```

The initial three- and six-byte `memcpy` reads compiled to stack stores and reloads with GCC. The measured implementation uses exact-width loads and combines their results in registers.

## Results

- **Grouped.** The larger workload has 6,291,456 distinct forks. Grouped `w = 2` uses 24 MiB and follows zero long refs. Grouped `w = 4` uses 48 MiB. Grouped `w = 2` has the lower median in both runs of all three cases, with overlapping per-trial ranges.
- **Interleaved, 16-fork fragments.** `w = 2` follows two long refs per 32 fork visits. In the smaller workload its median is higher than `w = 4` in both runs. In the larger workload it uses 25.5 MiB versus 48 MiB for `w = 4`, and the median ranking reverses across the two runs.
- **Interleaved, 64-fork fragments.** With the same number of distinct forks as the larger 16-fork workload, `w = 2` follows two long refs per 128 visits. It uses 24.375 MiB and its two medians are 9.094 and 9.480 CPU ns per fork. The `w = 4` medians are 10.987 and 11.606 at 48 MiB.

A separate small run passed AddressSanitizer and UndefinedBehaviorSanitizer. Trial ranges are broad, so the per-case medians are descriptive. The space and hop counts are exact for these placements.

### First run

```text
families=8192 span=16 trials=15 distinct_forks=393216 visits=524288 root_bytes=131072
w=2 grouped bytes=1572864 long_refs=0 hops=0 cpu-ns/fork=6.805 range=5.771–10.290
w=2 interleaved bytes=1671168 long_refs=24576 hops=32768 cpu-ns/fork=12.220 range=8.762–18.860
w=3 grouped bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=9.676 range=7.007–12.629
w=3 interleaved bytes=2506752 long_refs=24576 hops=32768 cpu-ns/fork=14.112 range=8.934–19.801
w=4 grouped bytes=3145728 long_refs=0 hops=0 cpu-ns/fork=7.784 range=6.075–11.812
w=4 interleaved bytes=3145728 long_refs=0 hops=0 cpu-ns/fork=9.481 range=7.150–21.731
families=131072 span=16 trials=15 distinct_forks=6291456 visits=8388608 root_bytes=2097152
w=2 grouped bytes=25165824 long_refs=0 hops=0 cpu-ns/fork=16.052 range=12.432–26.268
w=2 interleaved bytes=26738688 long_refs=393216 hops=524288 cpu-ns/fork=21.718 range=18.336–25.153
w=3 grouped bytes=37748736 long_refs=0 hops=0 cpu-ns/fork=19.360 range=15.127–28.426
w=3 interleaved bytes=40108032 long_refs=393216 hops=524288 cpu-ns/fork=23.405 range=20.223–32.921
w=4 grouped bytes=50331648 long_refs=0 hops=0 cpu-ns/fork=17.900 range=13.235–26.627
w=4 interleaved bytes=50331648 long_refs=0 hops=0 cpu-ns/fork=23.864 range=16.197–33.895
families=32768 span=64 trials=15 distinct_forks=6291456 visits=8388608 root_bytes=524288
w=2 grouped bytes=25165824 long_refs=0 hops=0 cpu-ns/fork=8.264 range=7.282–13.933
w=2 interleaved bytes=25559040 long_refs=98304 hops=131072 cpu-ns/fork=9.480 range=6.097–11.222
w=3 grouped bytes=37748736 long_refs=0 hops=0 cpu-ns/fork=10.411 range=7.273–17.096
w=3 interleaved bytes=38338560 long_refs=98304 hops=131072 cpu-ns/fork=13.216 range=9.843–18.525
w=4 grouped bytes=50331648 long_refs=0 hops=0 cpu-ns/fork=11.050 range=7.750–16.620
w=4 interleaved bytes=50331648 long_refs=0 hops=0 cpu-ns/fork=10.987 range=9.414–15.900
```

### Repeat

```text
families=8192 span=16 trials=15 distinct_forks=393216 visits=524288 root_bytes=131072
w=2 grouped bytes=1572864 long_refs=0 hops=0 cpu-ns/fork=9.550 range=5.203–47.434
w=2 interleaved bytes=1671168 long_refs=24576 hops=32768 cpu-ns/fork=18.669 range=14.375–45.419
w=3 grouped bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=13.532 range=6.812–58.013
w=3 interleaved bytes=2506752 long_refs=24576 hops=32768 cpu-ns/fork=18.327 range=12.924–39.463
w=4 grouped bytes=3145728 long_refs=0 hops=0 cpu-ns/fork=14.429 range=8.902–88.874
w=4 interleaved bytes=3145728 long_refs=0 hops=0 cpu-ns/fork=15.263 range=7.869–25.685
families=131072 span=16 trials=15 distinct_forks=6291456 visits=8388608 root_bytes=2097152
w=2 grouped bytes=25165824 long_refs=0 hops=0 cpu-ns/fork=13.174 range=10.900–34.354
w=2 interleaved bytes=26738688 long_refs=393216 hops=524288 cpu-ns/fork=21.970 range=14.667–30.021
w=3 grouped bytes=37748736 long_refs=0 hops=0 cpu-ns/fork=17.788 range=12.535–30.114
w=3 interleaved bytes=40108032 long_refs=393216 hops=524288 cpu-ns/fork=22.348 range=19.127–28.822
w=4 grouped bytes=50331648 long_refs=0 hops=0 cpu-ns/fork=18.159 range=15.001–21.920
w=4 interleaved bytes=50331648 long_refs=0 hops=0 cpu-ns/fork=19.436 range=15.882–27.838
families=32768 span=64 trials=15 distinct_forks=6291456 visits=8388608 root_bytes=524288
w=2 grouped bytes=25165824 long_refs=0 hops=0 cpu-ns/fork=8.859 range=7.602–12.458
w=2 interleaved bytes=25559040 long_refs=98304 hops=131072 cpu-ns/fork=9.094 range=6.884–14.697
w=3 grouped bytes=37748736 long_refs=0 hops=0 cpu-ns/fork=10.436 range=8.422–17.531
w=3 interleaved bytes=38338560 long_refs=98304 hops=131072 cpu-ns/fork=12.867 range=10.682–16.884
w=4 grouped bytes=50331648 long_refs=0 hops=0 cpu-ns/fork=9.460 range=8.029–11.743
w=4 interleaved bytes=50331648 long_refs=0 hops=0 cpu-ns/fork=11.606 range=10.364–15.683
```

## Follow-up within the three-byte ref window

The first series put the interleaved heaps beyond the 2 MiB ref window at `w = 3`. This follow-up uses 294,912 distinct forks. At `w = 3` the packed heap is 1.6875 MiB and every ref is direct. At `w = 2` the interleaved heap is 1.195–1.266 MiB and the traversal follows either two long refs per 32 visits or two per 16, depending on fragment length. At `w = 4` the heap is 2.25 MiB and every ref is direct. The benchmark source, exact-width loads, compiler flags, and CPU affinity are the same.

- With eight-fork fragments, interleaved `w = 3` eliminated 49,152 long-ref lookups per pass but took 15.718 and 17.418 CPU ns per visited fork in the two runs. Interleaved `w = 2` took 9.433 and 10.404, and `w = 4` took 13.160 and 13.831.
- With 16-fork fragments, `w = 3` eliminated 24,576 long-ref lookups per pass. Its interleaved median and the `w = 2` median switched order across runs. `w = 4` had the lowest interleaved median in both.
- With grouped placement, `w = 2` used 1.125 MiB and had the lowest median in all four runs.

The three-byte layout offers a larger direct window and 25% less packed space than four-byte refs. On these fork-heavy fixtures, the window alone gave it no repeatable traversal advantage over two-byte refs. The fixtures use leaves and one-byte fruits, so they give no estimate of the second in-place fruit byte's value.

Run the same compiled benchmark with `12288 8 21`, `6144 16 21`, `6144 16 21`, and `12288 8 21` in that order.

```text
families=12288 span=8 trials=21 distinct_forks=294912 visits=393216 root_bytes=196608
w=2 grouped bytes=1179648 long_refs=0 hops=0 cpu-ns/fork=7.856 range=6.306–12.536
w=2 interleaved bytes=1327104 long_refs=36864 hops=49152 cpu-ns/fork=9.433 range=6.781–13.993
w=3 grouped bytes=1769472 long_refs=0 hops=0 cpu-ns/fork=12.410 range=9.122–21.675
w=3 interleaved bytes=1769472 long_refs=0 hops=0 cpu-ns/fork=15.718 range=11.891–38.673
w=4 grouped bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=8.777 range=6.799–14.851
w=4 interleaved bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=13.160 range=9.537–18.044
families=6144 span=16 trials=21 distinct_forks=294912 visits=393216 root_bytes=98304
w=2 grouped bytes=1179648 long_refs=0 hops=0 cpu-ns/fork=5.332 range=4.191–8.306
w=2 interleaved bytes=1253376 long_refs=18432 hops=24576 cpu-ns/fork=8.450 range=6.691–11.331
w=3 grouped bytes=1769472 long_refs=0 hops=0 cpu-ns/fork=7.180 range=5.650–12.015
w=3 interleaved bytes=1769472 long_refs=0 hops=0 cpu-ns/fork=10.192 range=7.349–26.019
w=4 grouped bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=6.887 range=4.830–11.565
w=4 interleaved bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=8.003 range=5.228–15.928
families=6144 span=16 trials=21 distinct_forks=294912 visits=393216 root_bytes=98304
w=2 grouped bytes=1179648 long_refs=0 hops=0 cpu-ns/fork=5.027 range=3.768–7.872
w=2 interleaved bytes=1253376 long_refs=18432 hops=24576 cpu-ns/fork=9.819 range=7.209–12.838
w=3 grouped bytes=1769472 long_refs=0 hops=0 cpu-ns/fork=7.758 range=5.785–10.095
w=3 interleaved bytes=1769472 long_refs=0 hops=0 cpu-ns/fork=9.447 range=7.818–31.948
w=4 grouped bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=6.642 range=4.209–9.607
w=4 interleaved bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=7.630 range=5.730–24.156
families=12288 span=8 trials=21 distinct_forks=294912 visits=393216 root_bytes=196608
w=2 grouped bytes=1179648 long_refs=0 hops=0 cpu-ns/fork=8.408 range=5.882–27.015
w=2 interleaved bytes=1327104 long_refs=36864 hops=49152 cpu-ns/fork=10.404 range=6.463–20.105
w=3 grouped bytes=1769472 long_refs=0 hops=0 cpu-ns/fork=12.835 range=8.979–23.377
w=3 interleaved bytes=1769472 long_refs=0 hops=0 cpu-ns/fork=17.418 range=10.830–35.561
w=4 grouped bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=8.587 range=6.596–25.962
w=4 interleaved bytes=2359296 long_refs=0 hops=0 cpu-ns/fork=13.831 range=9.490–20.278
```
