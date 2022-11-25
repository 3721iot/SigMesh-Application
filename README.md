#3721 Open SigMesh

English | [中文文档](./README.zh.md)

#### introduce

Bluetooth Mesh (SigMesh) is a Bluetooth Mesh standard released by the Bluetooth SIG organization on July 17, 2017, which supports the creation of large-scale device networks, appropriately builds automation, and provides a variety of Internet of Things (IoT) solutions.


This project is an open-source collection of SigMesh application products, from sub-node devices, gateways, and APP full series open-source, to create more standard SigMesh hardware ecology for the industry community, everyone is welcome to participate.


This Mesh node uses Furikun FR8016HA main control chip and adapter SDK to realize PDV and GATT Mesh distribution network mode, which can control standard equipment, such as the switch of colored lights, color and brightness settings. Developers of other functions can implement it based on their own business needs. Demo use cases can be programmed directly using common modules, or they can customize PCBA chips for programming.

#### code：
	Node_led     Smart lights
	Node_switch  Smart switch（In internal testing, please contact your email for the code)
	Node_plug    Smart plug  （In internal testing, please contact your email for the code)
	

####  Development Tutorial

1.  [development environment](Doc/FR8016HA/GettingStarted/README.md)
2.  [Getting Started](Doc/FR8016HA/README.md)
3.  APP
4.  WIFI SigMesh Gateway

#### APP

Android/iOS support, divided into MQTT cloud version and stand-alone version. 
Support single control, group control, and other functions 
stand-alone

<img style="width:200px;" src="./Doc/APP/media/4.jpg"  /><img style="width:200px;" src="./Doc/APP/media/1.jpg"  /> <img style="width:200px;" src="./Doc/APP/media/3.jpg"  />

MQTT  Cloud

<img style="width:200px;height:425px" src="./Doc/APP/media/9.png"  /> <img style="width:200px;;height:425px" src="./Doc/APP/media/7.png"  />

#### Module
Specifications

#### WIFI Gateway

WIFI gateway，The uplink and downlink of MQTT and SigMesh models have been realized, and the standard MQTT3.1.1 is supported.

#### Email address 
tommy0906i@gmail.com

#### Contribute
1. Fork this warehouse
2. Create a new Feat_xxx branch
3. Submit the code
4. Create a new Pull Request#3721 Open SigMesh
