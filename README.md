This is a simple application I hacked together over the last couple of days. If anyone is interested in this sort of thing, let me know and I'll refine it into something more properly.

What this will do is take a file (binary, not text) of 64-bit prime numbers, then iterate over each prime to then...
  - foreach prime...
    - foreach input keylen...
      - foreach key of keylen...
        - "Hash" the key by multiplying the prime it against sets of keys (ascii, memory-mapped, mo break between each key).
          - Count/accumulate the number of bits set at each bit position, summing with all other previous keys
          - "Avalanching" calculation by modifying each bit of the key, then "re-hashing" and counting the _differing_ bits.
    - When all keys have been analyzed for that particular prime:
      - Run additional analysis of bits being set, as well as avalanching properties.
      - Store the complete set of data (bit counts, avalanching bit counts, etc) for later writing in batch to disk.

Once a file has been written to disk, it can be read/filtered by using PrimeStats.Read.Main.
  - It should be noted that the data for each prime is nearly 10kb. Analyzing a million primes will result in 10gb of disk usage.
  - It's possible to compress these files afterwards, and they easily compress to about 1/3 their size.


Here is a sample output of the data:

```
  --------------------------------------------------------------------------------
  14847499675007046253

  .bits.cnt             64    3929   10000   10000   10000   10000   10000   10000
  .bits.bit.min         19    1888    4623    4684    4675    4675    4650    4715
  .bits.bit.max         35    2015    5160    5146    5269    5281    5246    5178
  .bits.bit.sum       1999  125896  320141  320014  320095  320668  320845  320876
  .bits.bit.gap         16     127     537     462     594     606     596     463
  .bits.bit.avg         31      32      32      32      32      32      32      32

  .bits.pop.min         22      18      17      18      17      14      18      16
  .bits.pop.max         46      48      48      49      46      45      50      48
  .bits.pop.sum       2063  129825  330141  330014  330095  330668  330845  330876
  .bits.pop.gap         24      30      31      31      29      31      32      32
  .bits.pop.avg         32      33      33      33      33      33      33      33

  .ava.cnt             512   62864  240000  320000  400000  480000  560000  640000
  .ava.bit.min          64    3929   10000   10000   10000   10000   10000   10000
  .ava.bit.max         343   38500  137520  164283  164225  164175  164484  164529
  .ava.bit.sum       14354 1699989 6424104 8337666 8334848 8333199 8345062 8346931
  .ava.bit.gap         279   34571  127520  154283  154225  154175  154484  154529
  .ava.bit.avg          28      27      26      26      20      17      14      13

  .ava.pop.min          10       7       4       1       0       0       0       0
  .ava.pop.max          54      59      58      59      59      59      59      59
  .ava.pop.sum       14866 1762853 6664104 8657666 8734848 8813199 8905062 8986931
  .ava.pop.gap          44      52      54      58      59      59      59      59
  .ava.pop.avg          29      28      27      27      21      18      15      14
```
