# device_board_revoview

## Introduction

This repository is used to store content related to development boards based on UNISOC chips. For detailed information, please refer to the readme files in each development board subdirectory.

|                          Board Name                          | SoC Model | System Type |   Domain   |
| :----------------------------------------------------------: | :-----: | :------: | :------: |
| [wukong100](https://gitcode.com/openharmony-sig/device_board_revoview/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/wukong100/README_zh.md) | UIS7885 | Standard System | Smart Media |


## Directory
```
/device/board/revoview
└── wukong100               # wukong100 development board related code
    ├── audio_alsa          # audio alsa solution code
    ├── BUILD.gn
    ├── bundle.json
    ├── camera              # camera related code
    ├── cfg                 # fstab, init and other startup related cfg files
    ├── config.gni
    ├── device.gni
    ├── distributedhardware # distributed related configuration
    ├── kernel              # kernel compilation scripts, tools, defconfig configuration files
    ├── ohos.build
    └── pac
        └── uis7885_pac.sh  # Unisoc pac package packaging script
```


## Build and Usage

[WUKONG100开发板使用说明](https://gitcode.com/openharmony-sig/device_board_revoview/blob/OpenHarmony_standard_p7885_rk3588_d3000m_20251103/wukong100/README_zh.md)


## License

See the LICENSE file and code declarations in the corresponding directories.


## Related Repositories

[device_soc_revoview](https://gitcode.com/openharmony-sig/device_soc_unisoc)

[vendor_revoview](https://gitcode.com/openharmony-sig/vendor_revoview)
