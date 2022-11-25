2021/11/16
1、优化底层内存消耗；
2、优化底层数据包累积时切换快速发包问题

2021/06/23
1、优化mesh_stop
2、优化mesh底层调度
3、优化app_mesh_send_user_adv_packet接口功能

2021/06/02
1、增加IV update接口
2、默认使能配网结束后开启发送secure beacon
3、支持应用层再开一条普通广播，用于普通数据传输，mac地址和mesh广播地址区分开了（普通gatt回调参考proj_main.c
中的gatt回调处理，主要是连接号等信息的处理）

1. 程序会打印出来DEV_KEY、NETWORK_KEY、APP_KEY
2. 在proj_main文件中的user_entry_after_ble_init函数里面可以调用system_set_tx_power(RF_TX_POWER_NEG_16dBm);将发射功率配置到最低，同时断开板载天线，用来降低provisonee的信号强度，从而实现近距离测试PB-Remote功能。
3. 当provisonee作为从机被provioner连接上之后会打印
    slave[0],connect. link_num:1
    0x81,0x5D,0x54,0x42,0x5F,0x62,
	通过打印的地址来判断主机是echo还是其他节点，从而区分入网是否通过PB-Remote。