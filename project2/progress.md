## Project2 Progress

Finished Phase1. 03/31
G-share and Y-P predictor

./proj2sim -i traces/deepsjeng531_50K.trace


Finish first draft.  Start debugging.   04/03

Finish debugging. 04/05:

Pass big_xz557_50K.out



==> Running med_nomiss nab544...
--- ref_outs/med_nomiss_nab544_2M.out   2024-03-19 03:02:04.000000000 +0000
+++ student_outs/med_nomiss_nab544_2M.out       2024-04-06 03:36:10.697379700 +0000
@@ -13,21 +13,4 @@
 Predictor (M):  Gshare
 SETUP COMPLETE - STARTING SIMULATION

-SIMULATION OUTPUT
-Cycles:                    649886
-Trace instructions:        2000000
-Fetched instructions:      2000000
-Retired instructions:      2000000
-Total branch instructions: 301908
-Branch Mispredictions:     0
-I-cache misses:            0
-D-cache misses:            0
-Cycles with no fires:      24803
-No dispatch cycles due to ROB:   39
-Max DispQ usage:           128
-Average DispQ usage:       127.979
-Max SchedQ usage:          35
-Average SchedQ usage:      33.878
-Max ROB usage:             128
-Average ROB usage:         64.094
-IPC:                       3.077
+It has been 256 cycles since the last retirement. Does the simulator have a deadlock?

Please examine the differences printed above. Benchmark: nab544. Config name: med_nomiss. Flags to cachesim used: -b 3 -p 15 -s 5 -a 3 -m 2 -l 2 -f 4 -d


Testing Tomasulo: med config
============================
==> Running med deepsjeng531...
--- ref_outs/med_deepsjeng531_2M.out    2024-03-19 03:02:12.000000000 +0000
+++ student_outs/med_deepsjeng531_2M.out        2024-04-06 04:43:48.050069100 +0000
@@ -14,20 +14,20 @@
 SETUP COMPLETE - STARTING SIMULATION

 SIMULATION OUTPUT
-Cycles:                    1433989
+Cycles:                    1595993
 Trace instructions:        2000000
 Fetched instructions:      2000000
 Retired instructions:      2000000
 Total branch instructions: 171719
-Branch Mispredictions:     52466
+Branch Mispredictions:     79151
 I-cache misses:            1023
 D-cache misses:            12840
-Cycles with no fires:      550090
-No dispatch cycles due to ROB:   5069
-Max DispQ usage:           128
-Average DispQ usage:       2.570
+Cycles with no fires:      644189
+No dispatch cycles due to ROB:   5003
+Max DispQ usage:           114
+Average DispQ usage:       2.180
 Max SchedQ usage:          35
-Average SchedQ usage:      10.940
+Average SchedQ usage:      9.662
 Max ROB usage:             128
-Average ROB usage:         24.213
-IPC:                       1.395
+Average ROB usage:         21.674
+IPC:                       1.253

Please examine the differences printed above. Benchmark: deepsjeng531. Config name: med. Flags to cachesim used: -b 3 -p 15 -s 5 -a 3 -m 2 -l 2 -f 4

Please examine the differences printed above. Benchmark: deepsjeng531. Config name: med. Flags to cachesim used: -b 3 -p 15 -s 5 -a 3 -m 2 -l 2 -f 4

==> Running med leela541...
--- ref_outs/med_leela541_2M.out        2024-03-19 03:02:13.000000000 +0000
+++ student_outs/med_leela541_2M.out    2024-04-06 04:48:55.049340900 +0000
@@ -14,20 +14,20 @@
 SETUP COMPLETE - STARTING SIMULATION

 SIMULATION OUTPUT
-Cycles:                    1221944
+Cycles:                    1323755
 Trace instructions:        2000000
 Fetched instructions:      2000000
 Retired instructions:      2000000
 Total branch instructions: 175075
-Branch Mispredictions:     55763
+Branch Mispredictions:     69706
 I-cache misses:            237
 D-cache misses:            8662
-Cycles with no fires:      339310
-No dispatch cycles due to ROB:   489
+Cycles with no fires:      401280
+No dispatch cycles due to ROB:   325
 Max DispQ usage:           128
-Average DispQ usage:       3.540
+Average DispQ usage:       2.348
 Max SchedQ usage:          35
-Average SchedQ usage:      10.759
+Average SchedQ usage:      9.753
 Max ROB usage:             128
-Average ROB usage:         18.869
-IPC:                       1.637
+Average ROB usage:         17.302
+IPC:                       1.511

Please examine the differences printed above. Benchmark: leela541. Config name: med. Flags to cachesim used: -b 3 -p 15 -s 5 -a 3 -m 2 -l 2 -f 4



==> Running med xz557...
--- ref_outs/med_xz557_2M.out   2024-03-19 03:02:16.000000000 +0000
+++ student_outs/med_xz557_2M.out       2024-04-06 04:51:21.261695400 +0000
@@ -14,20 +14,20 @@
 SETUP COMPLETE - STARTING SIMULATION

 SIMULATION OUTPUT
-Cycles:                    1885399
+Cycles:                    6547600
 Trace instructions:        2000000
 Fetched instructions:      2000000
 Retired instructions:      2000000
 Total branch instructions: 938000
-Branch Mispredictions:     2436
+Branch Mispredictions:     932400
 I-cache misses:            0
 D-cache misses:            3200
-Cycles with no fires:      917715
+Cycles with no fires:      3706800
 No dispatch cycles due to ROB:   0
-Max DispQ usage:           128
-Average DispQ usage:       119.701
-Max SchedQ usage:          35
-Average SchedQ usage:      34.063
-Max ROB usage:             77
-Average ROB usage:         35.723
-IPC:                       1.061
+Max DispQ usage:           4
+Average DispQ usage:       0.305
+Max SchedQ usage:          19
+Average SchedQ usage:      1.056
+Max ROB usage:             44
+Average ROB usage:         1.405
+IPC:                       0.305

Please examine the differences printed above. Benchmark: xz557. Config name: med. Flags to cachesim used: -b 3 -p 15 -s 5 -a 3 -m 2 -l 2 -f 4



==> Running med nab544...
--- ref_outs/med_nab544_2M.out  2024-03-19 03:02:17.000000000 +0000
+++ student_outs/med_nab544_2M.out      2024-04-06 04:51:25.821835800 +0000
@@ -13,21 +13,4 @@
 Predictor (M):  Gshare
 SETUP COMPLETE - STARTING SIMULATION

-SIMULATION OUTPUT
-Cycles:                    1553352
-Trace instructions:        2000000
-Fetched instructions:      2000000
-Retired instructions:      2000000
-Total branch instructions: 301908
-Branch Mispredictions:     74673
-I-cache misses:            0
-D-cache misses:            14553
-Cycles with no fires:      570094
-No dispatch cycles due to ROB:   0
-Max DispQ usage:           128
-Average DispQ usage:       2.950
-Max SchedQ usage:          35
-Average SchedQ usage:      8.282
-Max ROB usage:             113
-Average ROB usage:         14.420
-IPC:                       1.288
+It has been 256 cycles since the last retirement. Does the simulator have a deadlock?

Please examine the differences printed above. Benchmark: nab544. Config name: med. Flags to cachesim used: -b 3 -p 15 -s 5 -a 3 -m 2 -l 2 -f 4



Testing Tomasulo: big config
============================
==> Running big deepsjeng531...
--- ref_outs/big_deepsjeng531_2M.out    2024-03-19 03:02:19.000000000 +0000
+++ student_outs/big_deepsjeng531_2M.out        2024-04-06 04:54:29.291302900 +0000
@@ -14,20 +14,20 @@
 SETUP COMPLETE - STARTING SIMULATION

 SIMULATION OUTPUT
-Cycles:                    1358656
+Cycles:                    1521414
 Trace instructions:        2000000
 Fetched instructions:      2000000
 Retired instructions:      2000000
 Total branch instructions: 171719
-Branch Mispredictions:     52863
+Branch Mispredictions:     79151
 I-cache misses:            1023
 D-cache misses:            12840
-Cycles with no fires:      537769
+Cycles with no fires:      628775
 No dispatch cycles due to ROB:   0
-Max DispQ usage:           221
-Average DispQ usage:       2.377
+Max DispQ usage:           210
+Average DispQ usage:       1.872
 Max SchedQ usage:          72
-Average SchedQ usage:      15.467
+Average SchedQ usage:      13.469
 Max ROB usage:             207
-Average ROB usage:         30.641
-IPC:                       1.472
+Average ROB usage:         27.429
+IPC:                       1.315

Please examine the differences printed above. Benchmark: deepsjeng531. Config name: big. Flags to cachesim used: -b 3 -p 15 -s 8 -a 4 -m 3 -l 2 -f 8



==> Running big leela541...
--- ref_outs/big_leela541_2M.out        2024-03-19 03:02:20.000000000 +0000
+++ student_outs/big_leela541_2M.out    2024-04-06 04:59:15.728937600 +0000
@@ -14,20 +14,20 @@
 SETUP COMPLETE - STARTING SIMULATION

 SIMULATION OUTPUT
-Cycles:                    1130267
+Cycles:                    1239624
 Trace instructions:        2000000
 Fetched instructions:      2000000
 Retired instructions:      2000000
 Total branch instructions: 175075
-Branch Mispredictions:     55425
+Branch Mispredictions:     69706
 I-cache misses:            237
 D-cache misses:            8662
-Cycles with no fires:      326715
+Cycles with no fires:      390594
 No dispatch cycles due to ROB:   0
-Max DispQ usage:           256
-Average DispQ usage:       4.676
+Max DispQ usage:           164
+Average DispQ usage:       2.821
 Max SchedQ usage:          72
-Average SchedQ usage:      16.573
-Max ROB usage:             166
-Average ROB usage:         26.902
-IPC:                       1.769
+Average SchedQ usage:      14.873
+Max ROB usage:             203
+Average ROB usage:         24.399
+IPC:                       1.613

Please examine the differences printed above. Benchmark: leela541. Config name: big. Flags to cachesim used: -b 3 -p 15 -s 8 -a 4 -m 3 -l 2 -f 8


==> Running big xz557...
--- ref_outs/big_xz557_2M.out   2024-03-19 03:02:24.000000000 +0000
+++ student_outs/big_xz557_2M.out       2024-04-06 05:01:36.284885000 +0000
@@ -14,20 +14,20 @@
 SETUP COMPLETE - STARTING SIMULATION

 SIMULATION OUTPUT
-Cycles:                    1881396
+Cycles:                    6542400
 Trace instructions:        2000000
 Fetched instructions:      2000000
 Retired instructions:      2000000
 Total branch instructions: 938000
-Branch Mispredictions:     2436
+Branch Mispredictions:     932400
 I-cache misses:            0
 D-cache misses:            3200
-Cycles with no fires:      916512
+Cycles with no fires:      3704800
 No dispatch cycles due to ROB:   0
-Max DispQ usage:           256
-Average DispQ usage:       234.896
-Max SchedQ usage:          72
-Average SchedQ usage:      70.007
-Max ROB usage:             142
-Average ROB usage:         72.051
-IPC:                       1.063
+Max DispQ usage:           8
+Average DispQ usage:       0.306
+Max SchedQ usage:          50
+Average SchedQ usage:      1.074
+Max ROB usage:             77
+Average ROB usage:         1.420
+IPC:                       0.306

Please examine the differences printed above. Benchmark: xz557. Config name: big. Flags to cachesim used: -b 3 -p 15 -s 8 -a 4 -m 3 -l 2 -f 8





肯定是还有bug，我们还得好好检查。

现在想先把50K的跑完。2M的跑太慢了



big deep: 

./proj2sim -b 3 -p 15 -s 8 -a 4 -m 3 -l 2 -f 8 -traces/

deepsjeng 对的

leela 对的

mcf 对的

nab 对的

xz  对的



med always taken:

./proj2sim -b 0 -s 5 -a 3 -m 2 -l 2 -f 4 -i traces/xz557_50

deep 对的

leela 对的

mcf 对的

nab 对的

xz 对的



med: 

./proj2sim -b 3 -p 15 -s 5 -a 3 -m 2 -l 2 -f 4 -i traces/

deep 对的

lee 对的

mcf 对的

nab 不对 有debug可以de了 哭哭 bug找到了，我check fu是不是full的时候，check MUL写错了，check了三个。。。

xz 对的



med no miss:

./proj2sim -b 3 -p 15 -s 5 -a 3 -m 2 -l 2 -f 4 -d -i traces/

deep 对的

lee 对的

mcf 对的

nab 不对，有bug 。把上面改完，这个也对了。。

xz 对的



tiny

./proj2sim -b 3 -p 15 -s 4 -a 1 -m 1 -l 1 -f 2 -i traces/

deep 对的

lee  对的

mcf 对的

nab  不对 改完一个bug，这个也对了，哈哈。

xz 对的

































