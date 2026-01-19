# 【开发板名称】WUKONG100开发套件

**简介**

WUKONG100开发套件，基于UNISOC UIS7885，采用1×Cortex-A76@2.7GHz + 3×Cortex-A76@2.3GHz + 4×Cortex-A55@2.1GHz架构，Arm Mali-G57 GPU，支持最高4K 60fps视频解码和4K 30fps视频编码，提供流畅的3D图形渲染和UI动画，支持多屏高清显示。集成独立的NPU，典型AI算力为 4-8 TOPS，支持主流深度学习框架。支持4G/‌5G，并兼容LTE、WCDMA、GSM及Wi-Fi 2.4G/5.0G、蓝牙5.0、NFC等多种无线协议。支持星闪、支持 GPS, GLONASS, Galileo, 北斗（BDS） 多模卫星定位系统，拥有丰富的扩展接口，支持2 xUART串口, GPIO数量x20, 2xI2C接口, 2 x SPI接口, 2xADC输入, 2xPWM输出，1xI2S接口，支持耳机、配置千兆自适应RJ45以太网口，支持TF卡， 支持双sim卡等。配置8GB内存和256GB存储。

WUKONG100开发套件外观图如图1所示：

![WUKONG100开发套件外观图](../picture/wukong100.png)

![image-20260109163646247](../picture//front.png)

![image-20260109163704808](../picture/back.png)



​																						图1：WUKONG100开发套件外观图

**一、开发板详情**

**1、WUKONG100开发套件正面接口示意图**

![wukong100功能分布图](../picture/interface.png)

​																					图2：WUKONG100开发套件正面接口示意图

**二、开发板规格**

WUKONG100开发套件核心规格如表1所示：

| 芯片、处理器、存储 |                                   |
| :----------------- | --------------------------------- |
| SoC                | UIS7885                           |
| CPU                | ARM Cortex-A76×4+ARM Cortex-A55×4 |
| GPU                | Arm Mali-G57                      |
| NPU                | 8.0TOPs                           |
| RAM                | 8GB  LPDDR4X                      |
| ROM                | 256GB UFS                         |

​																表1 WUKONG100开发套件核心规格清单

WUKONG100开发套件接口规格说明如表2所示：

| wukong100开发套件接口规格说明 |                                                                                                                |
|------------------------------|----------------------------------------------------------------------------------------------------------------|
| 电源                   | DC IN 12V3A |
| 指示灯                  | 2 x PWR LED |
| 调试          | 1x UART_USB调试串口  1x JTAG                                         |
| 按键                   | 1xPower key、1x Reset key、1x Vol+/Recovery、1 x VoL- |
| 显示接口               | 1 x  4-lane MIPI_DSI、支持TYPEC视频输出(可以转DP或HDMI） |
| 摄像头接口                   | 3x MIPI_CSI（4LANE） |
| 显示屏 | 10.1 寸, FHD 1920*1200 60Hz |
| 触摸屏 | 电容触摸屏 |
| USB                          | 1x TYPE-C USB3.0、2 x USB3.0 TYPE-A HOST、支持USB摄像头 |
| sensor                     | 1x accel、 1x gyro、1x als、 1x proximity、1x magnetic |
| 马达                       | 1x 线性马达                                        |
| LAN                  | 1x RJ45 千兆                                  |
| 蜂窝数据                     | 2G频段：B2/3/5/8、3G频段：B1/2/5/8、4G频段：B1/2/3/5/7/8/20/34/38/39/40/41、5G频段：N1/3/5/8/28/41/77/78 |
| 无线网络                     | 支持WIFI5 5GHz/2.4GHz,  802.11b/g/n/ac  1x天线             |
| 蓝牙                         | BT5.0                                                        |
| 星闪                         | 支持SLE1.0协议                                               |
| 音频                         | 1x 耳机输出（3.5mm, CTIA）、左右声道SPK座子（2Pins 2W@8Ω）、2 x 硅麦 |
| SIM 卡                       | 2 x SIM（Nano-SIM）支持热插拔                                |
| SD卡槽                       | 1 x SD卡槽，支持SDXC UHS-II                                  |
| 定位 | 支持GPS, GLONASS, Galileo, 北斗（BDS） |
| PCIe接口 | 36针标准PCIe接口 |
| 其它拓展接口 | 2 xUART串口, GPIO数量x20, 2xI2C接口, 2 x SPI接口, 2xADC输入, 2xPWM输出，1xI2S接口 |

​																	表2 WUKONG100开发套件接口规格说明

**三、搭建开发环境**

**1、开发环境准备**

操作系统
•	Ubuntu18.04及以上版本，X86_64架构，内存推荐16 GB及以上。
•	Ubuntu系统的用户名不能包含中文字符。

**2、环境代码准备**

**前提条件**

1）注册码云gitee账号。

2）注册码云SSH公钥，请参考[码云帮助中心](https://gitee.com/help/articles/4191)。

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

**获取源码操作步骤**

1） 通过repo + ssh 下载（需注册公钥，请参考码云帮助中心）。

```
repo init -u git@gitee.com:openharmony/manifest.git -b master --no-repo-verify

repo sync -c

repo forall -c 'git lfs pull'
```

2） 通过repo + https 下载。

```
repo init -u https://gitee.com/openharmony/manifest.git -b master --no-repo-verify

repo sync -c

repo forall -c 'git lfs pull'
```

**执行prebuilts**

在源码根目录下执行脚本，安装编译器及二进制工具。

```
bash build/prebuilts_download.sh
```

下载的prebuilts二进制默认存放在与OpenHarmony同目录下的OpenHarmony_2.0_canary_prebuilts下。

**四、编译调试**

**1、编译**

在Linux环境进行如下操作:

1） 进入源码根目录，执行如下命令进行版本编译。

*./build.sh --product-name wukong100 --ccache --gn-args make_custom_image=true*

2） 检查编译结果。编译完成后，log中显示如下：

```
post_process

=====build wukong100 successful.

2026-01-11 11:11:11
```

编译所生成的文件归档在out/wukong100/目录下，

镜像输出在out/wukong100/packages/phone/images/ 目录下。

3） 编译源码完成，请进行镜像烧录。

**2、烧录工具**

烧写工具下载及使用。

工具路径：device/board/revoview/wukong100/tools/Download_R27.22.3801.7z

烧录说明文档见 Download_R27.22.3801/Doc/下载工具使用指南V4.3.pdf“






