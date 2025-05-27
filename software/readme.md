Hand_Master--ESP32主机程序
Hand_Slaver_F1--食指
Hand_Slaver_F2--中指
Hand_Slaver_F3--无名指
Hand_Slaver_F4--小拇指
Hand_Slaver_F5--大拇指(TODO)

    error1 := 0	 //上次偏差
    integral := 0    //积分和
    integral_limit :=100 //积分限幅
    out := 0	     //输出
    //setpoint 设定值为入口参数
    //循环
    //采样周期为T
    //measured_value 采样得到值
    loop: 
        error0 := setpoint − measured_value	//本次偏差
        integral := integral + error0	//积分
        if(integral > integral_limit) integral = integral_limit	//积分限幅
        else if (integral < -integral_limit) integral = -integral_limit
        derivative := (error0 − error1) / T	//微分
        out := Kp × error0 + Ki × integral + Kd × derivative

        //记录偏差
        error1 := error0
        wait(T)	//间隔T运行
        goto loop



