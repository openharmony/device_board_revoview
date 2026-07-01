# 【开发板名称】WUKONG100开发套件

# **简介**

​        WUKONG100开发套件是OpenHarmony开源项目上一款面向高性能智能应用与复杂多场景互联开发的嵌入式硬件平台。基于UNISOC UIS7885平台，搭载8核处理器与NPU，支持多款传感器，集成了4G/5G通信，多模定位及丰富的扩展接口，不仅能够流畅处理高分辨率多媒体任务与实时AI计算，同时支持多屏显示与广泛的无线连接，为智能终端，边缘计算及物联网设备开发提供了灵活而可靠的硬件环境。

​                                                                                              图1：WUKONG100开发套件外观图

![WUKONG100开发套件外观图](../picture/wukong100.png)

![image-20260109163646247](../picture//front.png)

![image-20260109163704808](../picture/back.png)



# **开发板详情**

​                                                                                   图2：WUKONG100开发套件正面接口示意图

![wukong100功能分布图](../picture/interface.png)



# **开发板规格**

​                                                                                             表1： WUKONG100开发套件核心规格清单



| 规格项目 | 规格详情                          |
| :------- | --------------------------------- |
| SoC      | UIS7885                           |
| CPU      | ARM Cortex-A76×4+ARM Cortex-A55×4 |
| GPU      | Arm Mali-G57                      |
| NPU      | 8.0TOPs                           |
| RAM      | 8GB  LPDDR4X                      |
| ROM      | 256GB UFS                         |



​                                                                                             表2： WUKONG100开发套件接口规格说明

| 功能模块 | WUKONG100开发套件接口规格说明 |
|------------------------------|----------------------------------------------------------------------------------------------------------------|
| 电源                   | DC IN 12V3A |
| 指示灯                  | 2 x PWR LED |
| 调试          | 1x UART_USB调试串口 ， 1x JTAG                                        |
| 按键                   | 1xPower key，1x Reset key，1x Vol+/Recovery，1 x VoL- |
| 显示接口               | 1 x  4-lane MIPI_DSI，支持TYPEC视频输出(可以转DP或HDMI） |
| 摄像头接口                   | 3x MIPI_CSI（4LANE） |
| 显示屏 | 10.1 寸，FHD 1920*1200 60Hz |
| 触摸屏 | 电容触摸屏 |
| USB                          | 1x TYPE-C USB3.0，2 x USB3.0 TYPE-A HOST，支持USB摄像头 |
| sensor                     | 1x accel，1x gyro，1x als，1x proximity，1x magnetic |
| 马达                       | 1x 线性马达                                        |
| LAN                  | 1x RJ45 千兆                                  |
| 蜂窝数据                     | 2G频段：B2/3/5/8，3G频段：B1/2/5/8，4G频段：B1/2/3/5/7/8/20/34/38/39/40/41，5G频段：N1/3/5/8/28/41/77/78 |
| 无线网络                     | 支持WIFI5 5GHz/2.4GHz，802.11b/g/n/ac，1x天线             |
| 蓝牙                         | BT5.0                                                        |
| 星闪                         | 支持SLE1.0协议                                               |
| 音频                         | 1x 耳机输出（3.5mm, CTIA），左右声道SPK座子（2Pins 2W@8Ω），2 x 硅麦 |
| SIM 卡                       | 2 x SIM（Nano-SIM）支持热插拔                                |
| SD卡槽                       | 1 x SD卡槽，支持SDXC UHS-II                                  |
| 定位 | 支持GPS，GLONASS，Galileo，北斗（BDS） |
| PCIe接口 | 36针标准PCIe接口 |
| 其它拓展接口 | 2 xUART串口，GPIO数量x20，2xI2C接口， 2 x SPI接口，2xADC输入，2xPWM输出，1xI2S接口 |

# **搭建开发环境**

## **开发环境准备**

### 操作系统

•	Ubuntu20.04及以上版本，X86_64架构，内存推荐16 GB及以上。

•	Ubuntu系统的用户名不能包含中文字符。

### **环境代码准备**

#### **前提条件**

1）注册GitCode账号。

2）注册SSH公钥。

3）安装[git客户端](http://git-scm.com/book/zh/v2/%E8%B5%B7%E6%AD%A5-%E5%AE%89%E8%A3%85-Git)和[git-lfs](https://gitee.com/vcs-all-in-one/git-lfs?_from=gitee_search#downloading)并配置用户信息。

```
git config --global user.name "yourname"

git config --global user.email "your-email-address"

git config --global credential.helper store
```

4）安装码云repo工具，可以执行如下命令。

```
curl -s https://gitee.com/oschina/repo/raw/fork_flow/repo-py3 \>
/usr/local/bin/repo \

chmod a+x /usr/local/bin/repo

pip3 install -i https://repo.huaweicloud.com/repository/pypi/simple requests
```

5）通过以下步骤安装编译OpenHarmony需要的库和工具。

```
sudo apt-get update && sudo apt-get install binutils binutils-dev git git-lfs gnupg flex bison gperf build-essential zip curl zlib1g-dev gcc-multilib g++-multilib libc6-dev-i386 lib32ncurses5-dev x11proto-core-dev libx11-dev lib32z1-dev ccache libgl1-mesa-dev libxml2-utils xsltproc unzip m4 bc gnutls-bin python3.8 python2.7 python3-pip ruby genext2fs device-tree-compiler make libffi-dev e2fsprogs pkg-config perl openssl libssl-dev libelf-dev libdwarf-dev u-boot-tools mtd-utils cpio doxygen liblz4-tool openjdk-8-jre gcc g++ texinfo dosfstools mtools default-jre default-jdk libncurses5 apt-utils wget scons python3.8-distutils tar rsync git-core libxml2-dev lib32z-dev grsync xxd libglib2.0-dev libpixman-1-dev kmod jfsutils reiserfsprogs xfsprogs squashfs-tools pcmciautils quota ppp libtinfo-dev libtinfo5 libncurses5-dev libncursesw5 libstdc++6 gcc-arm-none-eabi vim ssh locales libxinerama-dev libxcursor-dev libxrandr-dev libxi-dev dwarves libnl-3-dev libnl-genl-3-dev autoconf automake libtool
```

6）配置Python 查看Python 的位置。

```
which python2.7
which python3.8
```

7）将Python和Python3切换为Python 2.7和Python 3.8。

```
sudo update-alternatives --install /usr/bin/python python {Python 2.7 路径} 1    #{Python 2.7 路径}为上一步查看的Python 2.7的位置
sudo update-alternatives --install /usr/bin/python3 python3 {Python 3.8 路径} 1   #{Python 3.8 路径}为上一步查看的Python 3.8的位置
```

#### **获取源码操作步骤**

1） 通过repo + ssh 下载（需注册公钥，请参考码云帮助中心）。

```
repo init -u git@gitcode.com:openharmony/manifest.git -b master

repo sync -c

repo forall -c 'git lfs pull'
```

2） 通过repo + https 下载。

```
repo init -u https://gitcode.com/openharmony/manifest -b master

repo sync -c

repo forall -c 'git lfs pull'
```

3） 执行prebuilts。

在源码根目录下执行脚本，安装编译器及二进制工具。

```
bash build/prebuilts_download.sh
```

下载的prebuilts二进制默认存放在与OpenHarmony同目录下的openharmony_prebuilts下。

## **编译调试**

### **编译**

在Linux环境进行如下操作：

1） 进入源码根目录，执行如下命令进行版本编译。

*./build.sh --product-name wukong100 --ccache --gn-args make_pac_format_image=true*

2） 检查编译结果。编译完成后，log中显示如下：

```
post_process

=====build wukong100 successful.

2026-01-11 11:11:11
```

编译所生成的文件归档在out/wukong100/目录下。

镜像输出在out/wukong100/packages/phone/images/ 目录下。

3） 编译源码完成，请进行镜像烧录。

[镜像烧录说明文档](https://gitcode.com/openharmony/device_board_revoview/tree/master/wukong100/tools)

4）板卡烧录过程中若出现异常问题，请联系**板卡厂商**申请技术支持与咨询。



