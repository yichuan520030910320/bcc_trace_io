# bash test.sh uring||sync read||write 32K
set -e
engine=$1
read_or_write=$2
size=$3
sudo ./gen ${size} 
sudo ./Benchmark ${engine} in.txt out.txt ${read_or_write}& sleep 1 && sudo python3 test.py --iotype ${engine}