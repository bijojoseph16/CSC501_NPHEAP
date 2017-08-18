sudo insmod kernel_module/npheap.ko
sudo chmod 777 /dev/npheap
./benchmark/benchmark 256 256 4
cat *.log > trace
sort -n -t 3 trace > sorted_trace
./benchmark/validate 256 < sorted_trace
rm -f *.log
sudo rmmod npheap