# device_board_revoview

## 简介

本仓用于存放基于UNISOC芯片开发板相关内容，详细信息可以参考各开发板子目录下的readme文件

|                          开发板名称                          | SoC型号 | 系统类型 |   领域   |
| :----------------------------------------------------------: | :-----: | :------: | :------: |
| [wukong100](https://gitcode.com/openharmony/device_board_revoview/blob/master/wukong100/README_zh.md) | UIS7885 | 标准系统 | 智慧媒体 |


## 目录
```
/device/board/revoview
└── wukong100               #wukong100开发板相关代码
    ├── audio_alsa          #audio alsa方案代码
    ├── BUILD.gn
    ├── bundle.json
    ├── camera              #camera相关代码
    ├── cfg                 #fstab, init等启动相关cfg文件
    ├── config.gni
    ├── device.gni
    ├── distributedhardware #分布式相关配置
    ├── kernel              #内核编译脚本, 工具, defconfig配置文件
    ├── ohos.build
    └── pac
        └── uis7885_pac.sh  #展锐pac包打包脚本
```


## 编译构建/使用方法

[WUKONG100开发板使用说明](https://gitcode.com/openharmony/device_board_revoview/blob/master/wukong100/README_zh.md)


## 许可说明

参见对应目录的LICENSE文件及代码声明


## 相关仓

[device_soc_revoview](https://gitcode.com/openharmony/device_soc_unisoc)

[vendor_revoview](https://gitcode.com/openharmony/vendor_revoview)