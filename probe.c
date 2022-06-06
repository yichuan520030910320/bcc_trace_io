#include <uapi/linux/ptrace.h>
#include <linux/sched.h>


BPF_HASH(trace1_in_hash);
int usdt_trace1_in_bpf(struct pt_regs *ctx)
{
  // bpf_trace_printk("trace1_in-call\n");
  u64 time = bpf_ktime_get_ns();
  u64 key = 0, val;
  u64 zero = 0;
  u64 *cnt;
  cnt = trace1_in_hash.lookup(&key);
  if (cnt == NULL)
  {
    val = 1;
    trace1_in_hash.update(&key, &zero);
  }
  else
  {
    val = (*cnt) + 1;
    trace1_in_hash.update(&key, &val);
  }
  trace1_in_hash.update(&val, &time);
  return 0;
}

BPF_HASH(trace1_out_hash);
int usdt_trace1_out_bpf(struct pt_regs *ctx)
{
  u64 time = bpf_ktime_get_ns();
  u64 key = 0, val;
  u64 zero = 0;
  u64 *cnt;
  cnt = trace1_out_hash.lookup(&key);
  if (cnt == NULL)
  {
    val = 1;
    trace1_out_hash.update(&key, &zero);
  }
  else
  {
    val = (*cnt) + 1;
    trace1_out_hash.update(&key, &val);
  }
  trace1_out_hash.update(&val, &time);
  return 0;
}


BPF_HASH(trace2_in_hash);
int usdt_trace2_in_bpf(struct pt_regs *ctx)
{
  u64 time = bpf_ktime_get_ns();
  u64 key = 0, val;
  u64 zero = 0;
  u64 *cnt;
  cnt = trace2_in_hash.lookup(&key);
  if (cnt == NULL)
  {
    val = 1;
    trace2_in_hash.update(&key, &zero);
  }
  else
  {
    val = (*cnt) + 1;
    trace2_in_hash.update(&key, &val);
  }
  trace2_in_hash.update(&val, &time);
  return 0;
}

BPF_HASH(trace2_out_hash);
int usdt_trace2_out_bpf(struct pt_regs *ctx)
{
  u64 time = bpf_ktime_get_ns();
  u64 key = 0, val;
  u64 zero = 0;
  u64 *cnt;
  cnt = trace2_out_hash.lookup(&key);
  if (cnt == NULL)
  {
    val = 1;
    trace2_out_hash.update(&key, &zero);
  }
  else
  {
    val = (*cnt) + 1;
    trace2_out_hash.update(&key, &val);
  }
  trace2_out_hash.update(&val, &time);
  return 0;
}