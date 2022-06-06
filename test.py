from __future__ import print_function
from bcc import BPF, USDT
import sys
import argparse
from time import sleep
def main(args):
    iotype=args.iotype
    pid = open('pid_file','r').readline()
    bpf_text = open('probe.c', 'r').read()
    print(pid)
    u = USDT(pid=int(pid))
    # u.enable_probe(probe="http__server__request", fn_name="do_trace")
    # initialize BPF
    u.enable_probe(probe="runcode_trace1_in", fn_name="usdt_trace1_in_bpf")
    u.enable_probe(probe="runcode_trace1_out", fn_name="usdt_trace1_out_bpf")
    
    u.enable_probe(probe="runcode_trace2_in", fn_name="usdt_trace2_in_bpf")
    u.enable_probe(probe="runcode_trace2_out", fn_name="usdt_trace2_out_bpf")
    b = BPF(text=bpf_text, usdt_contexts=[u])
    try:
	    sleep(9999999)
    except KeyboardInterrupt:
	        pass

    print("nmmd")
    
    start_trace1_in=b["trace1_in_hash"]
    finish_trace1_out=b["trace1_out_hash"]
    
    start_trace2_in=b["trace2_in_hash"]
    finish_trace2_out=b["trace2_out_hash"]
    
    
    start_trace1_list=[]
    finish_trace1_list=[]
    trace1_time=0
    for _, v in start_trace1_in.items():
        # print("trace1 In ",v)
        start_trace1_list.append(v)
    for _, v in finish_trace1_out.items():
        # print("trace1 out ",v)
        finish_trace1_list.append(v)
    for i in range(0, len(start_trace1_list)):
        trace1_time =trace1_time+ finish_trace1_list[i].value-start_trace1_list[i].value
    print("duration trace1: ", trace1_time)
    
    
    
    start_trace2_list=[]
    finish_trace2_list=[]
    trace2_time=0
    for _, v in start_trace2_in.items():
        # print("trace2 In ",v)
        start_trace2_list.append(v)
    for _, v in finish_trace2_out.items():
        # print("trace2 out ",v)
        finish_trace2_list.append(v)
    print("len start",len(start_trace2_list))
    print("finish",len(finish_trace2_list))
    for i in range(0, len(start_trace2_list)):
        trace2_time =trace2_time+ finish_trace2_list[i].value-start_trace2_list[i].value
    print("duration trace2:  ", trace2_time)
    
    
    print("all duration : ", trace1_time+trace2_time)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='begin test')
    parser.add_argument('--iotype', type=str, help='')
    args = parser.parse_args()
    main(args)