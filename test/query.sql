select log_time, instance, value from pmi_nt_perf_inst
where datapoint_id = 85
and log_time between '8/14/00' and '8/15/00'
and agent_id = 2004
and instance = 0
