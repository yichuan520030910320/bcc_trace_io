# bash test2.sh uring||sync read||write 32K
set -e
engine=$1
read_or_write=$2
size=$3
# sud?o ./gen ${size} 
# sudo ./Benchmark ${engine} in.txt out.txt ${read_or_write}& sleep 1 && sudo python3 funcslower.py c:read -u 1 -p 
sudo ./Benchmark ${engine} in.txt out.txt ${read_or_write}
