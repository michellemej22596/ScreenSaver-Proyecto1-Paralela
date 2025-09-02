#!/usr/bin/env bash
set -e

SEQ_BIN=./bin/screensaver_seq
PAR_BIN=./bin/screensaver_par

Ns=(100 500 1000 2000)
THREADS=(1 2 4 8)
REPEATS=10
FRAMES=500

OUT=results_raw.csv
echo "binary,n_threads,N,rep,frames,time_total,time_update" > $OUT

make

# Secuencial
for N in "${Ns[@]}"; do
  for ((r=1; r<=REPEATS; r++)); do
    echo "Running SEQ N=$N rep=$r"
    line=$($SEQ_BIN $N $FRAMES)
    out=$(echo "$line" | grep TIME_TOTAL | awk '{print $2}')
    update=$(echo "$line" | grep TIME_UPDATE | awk '{print $2}')
    echo "screensaver_seq,1,$N,$r,$FRAMES,$out,$update" >> $OUT
  done
done

# Paralelo
for T in "${THREADS[@]}"; do
  export OMP_NUM_THREADS=$T
  for N in "${Ns[@]}"; do
    for ((r=1; r<=REPEATS; r++)); do
      echo "Running PAR T=$T N=$N rep=$r"
      line=$($PAR_BIN $N $FRAMES)
      out=$(echo "$line" | grep TIME_TOTAL | awk '{print $2}')
      update=$(echo "$line" | grep TIME_UPDATE | awk '{print $2}')
      echo "screensaver_par,$T,$N,$r,$FRAMES,$out,$update" >> $OUT
    done
  done
done

echo "DONE. raw results in $OUT"
