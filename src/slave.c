void doSlave(){
	slave_init();//开节点日志：1
	wait_task();//等任务:2
	run_client();//运行客户程序:3
	update_slave_record();//更新记录状态:4
	send_back_result();//返回结果:5
}
