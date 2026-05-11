<table style="width: 100%; table-layout: fixed; border-collapse: collapse;">
  <thead>
    <tr>
      <th style="width: 15%; border: 1px solid #ddd; padding: 8px;">模块</th>
      <th style="width: 15%; border: 1px solid #ddd; padding: 8px;">文件名</th>
      <th style="width: 40%; border: 1px solid #ddd; padding: 8px;">文件路径</th>
      <th style="width: 15%; border: 1px solid #ddd; padding: 8px;">文件说明</th>
      <th style="width: 15%; border: 1px solid #ddd; padding: 8px;">后续计划</th>
    </tr>
  </thead>
  <tbody>
    <!-- 第1行：安全启动,第1列合并第1-3行 -->
  <tr>
    <td rowspan="14">安全启动或镜像签名证书/密钥</td>
    <td>rsa2048_private.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_private.pem</td>
    <td rowspan="14">安全启动或镜像签名证书/私钥</td>
    <td rowspan="14">N/A</td>
  </tr>
  <tr>
    <td>rsa2048_private_boot.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_private_boot.pem</td>
  </tr>
  <tr>
    <td>rsa2048_private_modem<br>.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_private_modem.pem</td>
  </tr>
  <tr>
    <td>rsa2048_private_product<br>.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_private_product.pem</td>
  </tr>
  <tr>
    <td>rsa2048_private_system<br>.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_private_system.pem</td>
  </tr>
  <tr>
    <td>rsa2048_private_vendor<br>.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_private_vendor.pem</td>
  </tr>
  <tr>
    <td>rsa2048_private_vendor<br>_boot.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_private_vendor_boot.pem</td>
  </tr>
  <!-- 公钥部分（7行） -->
  <tr>
    <td>rsa2048_public.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_public.pem</td>
  </tr>
  <tr>
    <td>rsa2048_public_boot.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_public_boot.pem</td>
  </tr>
  <tr>
    <td>rsa2048_public_modem<br>.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_public_modem.pem</td>
  </tr>
  <tr>
    <td>rsa2048_public_product<br>.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_public_product.pem</td>
  </tr>
  <tr>
    <td>rsa2048_public_system<br>.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_public_system.pem</td>
  </tr>
  <tr>
    <td>rsa2048_public_vendor<br>.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_public_vendor.pem</td>
  </tr>
  <tr>
    <td>rsa2048_public_vendor<br>_boot.pem</td>
    <td>\device_board_revoview\wukong100\pac\secureboot_key\<br>config\rsa2048_public_vendor_boot.pem</td>
  </tr>
  <!-- CAMERA(isp_lib相关) -->
  <tr>
    <td rowspan="27">Camera HAL 库</td>
    <td>libcam-ini-parser.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcam-ini-parser.z.so</td>
    <td rowspan="18">摄像头硬件抽象层 (HAL) 或算法库</td>
    <td rowspan="18">N/A</td>
  </tr>
  <tr>
    <td>libcambr.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcambr.z.so</td>
  </tr>
  <tr>
    <td>libcamcommon.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcamcommon.z.so</td>
  </tr>
  <tr>
    <td>libcammem.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcammem.z.so</td>
  </tr>
  <tr>
    <td>libcamoem.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcamoem.z.so</td>
  </tr>
  <tr>
    <td>libcampmloader.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcampmloader.z.so</td>
  </tr>
  <tr>
    <td>libcampm_isp2.7.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcampm_isp2.7.z.so</td>
  </tr>
  <tr>
    <td>libcamppm.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcamppm.z.so</td>
  </tr>
  <tr>
    <td>libcamps.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcamps.z.so</td>
  </tr>
  <tr>
    <td>libcamsensor.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcamsensor.z.so</td>
  </tr>
  <tr>
    <td>libcamv4l2adapter.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcamv4l2adapter.z.so</td>
  </tr>
  <tr>
    <td>libcamv4l2yuv2rgb.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcamv4l2yuv2rgb.z.so</td>
  </tr>
  <tr>
    <td>libcamxmlparser.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcamxmlparser.z.so</td>
  </tr>
  <tr>
    <td>libcam_otp_parser.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcam_otp_parser.z.so</td>
  </tr>
  <tr>
    <td>libcppdrv.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libcppdrv.z.so</td>
  </tr>
  <tr>
    <td>libispalg.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libispalg.z.so</td>
  </tr>
  <tr>
    <td>libotp_general.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libotp_general.z.so</td>
  </tr>
  <tr>
    <td>libpss_isp2.7.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libpss_isp2.7.z.so</td>
  </tr>
  <tr>
    <td>libsensor_gc05a2_f<br>_n100.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libsensor_gc05a2_f_n100.z.so</td>
    <td rowspan="3">摄像头模组驱动</td>
    <td rowspan="3">N/A</td>
  </tr>
  <tr>
    <td>libsensor_gc08a3_22<br>_n100.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libsensor_gc08a3_22_n100.z.so</td>
  </tr>
  <tr>
    <td>libsensor_gc08a3_n100<br>.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libsensor_gc08a3_n100.z.so</td>
  </tr>
  <tr>
    <td>libsprd_h264enc_wrapper<br>.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libsprd_h264enc_wrapper.z.so</td>
    <td rowspan="2">编解码接口动态封装转换库</td>
    <td rowspan="2">N/A</td>
  </tr>
  <tr>
    <td>libsprd_jpegenc_wrapper<br>.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libsprd_jpegenc_wrapper.z.so</td>
  </tr>
  <tr>
    <td>libvcm_dw9714.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\camera_hal\libvcm_dw9714.z.so</td>
    <td>控制VCM自动对焦</td>
  </tr>
  <!-- Tuning 参数组（3行合并） -->
  <tr>
    <td>gc05a2_f.tar.gz.gpg</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\tuning\gc05a2_f\com\gc05a2_f.tar.gz.gpg</td>
    <td>gc05a2_f 摄像头效果调试与校准参数</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td>gc08a3.tar.gz.gpg</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\tuning\gc08a3\com\gc08a3.tar.gz.gpg</td>
    <td>gc08a3 摄像头效果调试与校准参数</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td>isp_lib.tar.gz.gpg</td>
    <td>\device_soc_unisoc\p7885\hardware\camera_unisoc\<br>soc_p7885\isp_lib\isp_lib.tar.gz.gpg</td>
    <td>ISP闭源库加密包</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td rowspan="7">Modem电话服务</td>
    <td>tele_adapter.para.dac</td>
    <td>\device_soc_unisoc\p7885\hardware\modem\telephony\<br>etc\param\tele_adapter.para.dac</td>
    <td>电话适配器的参数</td>
    <td rowspan="6">N/A</td>
  </tr>
  <tr>
    <td>cp_diskserver.bin</td>
    <td>\device_soc_unisoc\p7885\hardware\modem\telephony\<br>bin\cp_diskserver.bin</td>
    <td rowspan="2">通信控制服务</td>
  </tr>
  <tr>
    <td>modem_control.bin</td>
    <td>\device_soc_unisoc\p7885\hardware\modem\telephony\<br>bin\modem_control.bin</td>
  </tr>
  <tr>
    <td>libmodem-utils.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\modem\telephony\<br>lib64\libmodem-utils.z.so</td>
    <td rowspan="3">电话服务或 Modem 适配工具库</td>
  </tr>
  <tr>
    <td>libmodem_halo.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\modem\telephony\<br>lib64\libmodem_halo.z.so</td>
  </tr>
  <tr>
    <td>libreadfixednv.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\modem\telephony\<br>lib64\libreadfixednv.z.so</td>
  </tr>
  <tr>
    <td>libsprd7885_tele_<br>adapter_1.0.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\modem\telephony\<br>lib64\libsprd7885_tele_adapter_1.0.z.so</td>
    <td>电话服务或 Modem 适配工具库</td>
    <td>后续将ril相关的标准接口实现并开源，7月8号前在OH7.0版本中完成</td>
  </tr>
  <!-- NPU 组（7行合并） -->
  <tr>
    <td rowspan="7">NPU 神经网络加速</td>
    <td>libuniai.so</td>
    <td>\device_soc_unisoc\p7885\hardware\npu\core\<br>libuniai.so</td>
    <td rowspan="7">NPU 神经网络加速推理或核心库</td>
    <td rowspan="7">N/A</td>
  </tr>
  <tr>
    <td>unisoc_NPU_backend.so</td>
    <td>\device_soc_unisoc\p7885\hardware\npu\core\<br>unisoc_NPU_backend.so</td>
  </tr>
  <tr>
    <td>libc++_shared.so</td>
    <td>\device_soc_unisoc\p7885\hardware\npu\lib64\<br>libc++_shared.so</td>
  </tr>
  <tr>
    <td>libimgcustom.so</td>
    <td>\device_soc_unisoc\p7885\hardware\npu\lib64\<br>libimgcustom.so</td>
  </tr>
  <tr>
    <td>libimgdnn.so</td>
    <td>\device_soc_unisoc\p7885\hardware\npu\lib64\<br>libimgdnn.so</td>
  </tr>
  <tr>
    <td>libnnacompiler.so</td>
    <td>\device_soc_unisoc\p7885\hardware\npu\lib64\<br>libnnacompiler.so</td>
  </tr>
  <tr>
    <td>libnnaruntime.so</td>
    <td>\device_soc_unisoc\p7885\hardware\npu\lib64\<br>libnnaruntime.so</td>
  </tr>
  <!-- OMX IL 多媒体编解码组（5行合并） -->
  <tr>
    <td rowspan="5">OMX多媒体编解码</td>
    <td>libomx_avcdec_hw_sprd<br>.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\omx_il\components\<br>video\avc\dec\hw\lib64\libomx_avcdec_hw_sprd.z.so</td>
    <td rowspan="5">多媒体硬件编解码 (OpenMAX IL) 组件</td>
    <td rowspan="5">N/A</td>
  </tr>
  <tr>
    <td>libjpeg_hw_sprd.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\omx_il\components\<br>video\avc\enc\hw\lib64\libjpeg_hw_sprd.z.so</td>
  </tr>
  <tr>
    <td>libomx_avcenc_hw_sprd<br>.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\omx_il\components\<br>video\avc\enc\hw\lib64\libomx_avcenc_hw_sprd.z.so</td>
  </tr>
  <tr>
    <td>libomx_hevcdec_hw_sprd<br>.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\omx_il\components\<br>video\hevc\dec\lib64\libomx_hevcdec_hw_sprd.z.so</td>
  </tr>
  <tr>
    <td>libomx_hevcenc_hw_sprd<br>.z.so</td>
    <td>\device_soc_unisoc\p7885\hardware\omx_il\components\<br>video\hevc\enc\lib64\libomx_hevcenc_hw_sprd.z.so</td>
  </tr>
  <!-- 无线连接与定位组（9行合并） -->
  <tr>
    <td rowspan="9">WCN无线连接</td>
    <td>btc_cali.bin</td>
    <td>\device_soc_unisoc\p7885\modules\ws73\<br>btc_cali.bin</td>
    <td>星闪校准固件，厂家提供</td>
    <td rowspan="9">N/A</td>
  </tr>
  <tr>
    <td>libbt_vendor.tar.gz.gpg</td>
    <td>\device_soc_unisoc\p7885\hardware\bluetooth\lib64\<br>libbt_vendor.tar.gz.gpg</td>
    <td>蓝牙闭源库加密包</td>
  </tr>
  <tr>
    <td>gnss.tar.gz.gpg</td>
    <td>\device_soc_unisoc\p7885\hardware\location\bin\<br>gnss.tar.gz.gpg</td>
    <td>gnss闭源库加密包</td>
  </tr>
  <tr>
    <td>wcn.tar.gz.gpg</td>
    <td>\device_soc_unisoc\p7885\hardware\wcn\bin\<br>wcn.tar.gz.gpg</td>
    <td>wcn闭源库加密包</td>
  </tr>
  <tr>
    <td>sle_client_sample</td>
    <td>\device_soc_unisoc\p7885\modules\ws73\<br>sle_client_sample</td>
    <td>SLE client功能示例程序</td>
  </tr>
  <tr>
    <td>sle_server_sample</td>
    <td>\device_soc_unisoc\p7885\modules\ws73\<br>sle_server_sample</td>
    <td>SLE server功能示例程序</td>
  </tr>
  <tr>
    <td>ws73.bin</td>
    <td>\device_soc_unisoc\p7885\modules\ws73\<br>ws73.bin</td>
    <td>ws73固件，厂家提供</td>
  </tr>
  <tr>
    <td>regulatory.db</td>
    <td>\device_soc_unisoc\p7885\hardware\wcn\<br>regulatory.db</td>
    <td rowspan="2">Wi-Fi国家码数据库文件，包括各个国家的无线频段配置</td>
  </tr>
  <tr>
    <td>regulatory.db.p7s</td>
    <td>\device_soc_unisoc\p7885\hardware\wcn\<br>regulatory.db.p7s</td>
  </tr>
  <!-- Audio 音频驱动组（28行合并） -->
  <tr>
    <td rowspan="33">内核驱动</td>
    <td>agdsp_pd.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>agdsp_pd.ko</td>
    <td rowspan="27">内核模块，音频驱动，DSP或功放</td>
    <td rowspan="30">内核6.6适配完成后，将删除ko。7月8号前在OH7.0版本中完成</td>
  </tr>
  <tr>
    <td>audio-dsp-dump.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>audio-dsp-dump.ko</td>
  </tr>
  <tr>
    <td>audio-pipe.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>audio-pipe.ko</td>
  </tr>
  <tr>
    <td>audio_mem.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>audio_mem.ko</td>
  </tr>
  <tr>
    <td>audio_sipc.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>audio_sipc.ko</td>
  </tr>
  <tr>
    <td>mcdt_hw_r2p0.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>mcdt_hw_r2p0.ko</td>
  </tr>
  <tr>
    <td>smp_aw87xxx.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>smp_aw87xxx.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-card.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-card.ko</td>
  <tr>
    <td>snd-soc-sprd-codec-<br>ump9620-power-dev.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-codec-ump9620-power-dev.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-codec-<br>ump9620-power.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-codec-ump9620-power.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-codec-<br>ump9620.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-codec-ump9620.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-dai.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-dai.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-dummy<br>-codec.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-dummy-codec.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-pa-ucp1301.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-pa-ucp1301.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-pdm-r2p0.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-pdm-r2p0.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-tdm.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-tdm.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-vbc-fe.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-vbc-fe.ko</td>
  </tr>
  <tr>
    <td>snd-soc-sprd-vbc-v4.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-sprd-vbc-v4.ko</td>
  </tr>
  <tr>
    <td>snd-soc-unisoc-dp-codec.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>snd-soc-unisoc-dp-codec.ko</td>
  </tr>
  <tr>
    <td>sprd-compr-2stage-dma.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>sprd-compr-2stage-dma.ko</td>
  </tr>
  <tr>
    <td>sprd-dmaengine-pcm.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>sprd-dmaengine-pcm.ko</td>
  </tr>
  <tr>
    <td>sprd-platform-pcm-routing.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>sprd-platform-pcm-routing.ko</td>
  </tr>
  <tr>
    <td>sprd_apipe.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>sprd_apipe.ko</td>
  </tr>
  <tr>
    <td>sprd_audcp_boot.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>sprd_audcp_boot.ko</td>
  </tr>
  <tr>
    <td>sprd_audcp_dvfs.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>sprd_audcp_dvfs.ko</td>
  </tr>
  <tr>
    <td>sprd_audio_usb_offload.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>sprd_audio_usb_offload.ko</td>
  </tr>
  <tr>
    <td>voice_trigger_irq.ko</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>voice_trigger_irq.ko</td>
  </tr>
  <!-- Touch 触摸屏驱动组 -->
  <tr>
    <td>gslx680.ko</td>
    <td>\device_soc_unisoc\p7885\modules\<br>input\touchscreen\gslx680.ko</td>
    <td>触摸屏内核驱动</td>
  </tr>
  <!-- VPU 驱动组 -->
  <tr>
    <td>vpu.ko</td>
    <td>\device_soc_unisoc\p7885\modules\<br>vpu\vpu.ko</td>
    <td>vpu驱动</td>
  </tr>
  <!-- 加密压缩包组 -->
  <tr>
    <td>modules.tar.gz.gpg</td>
    <td>\device_soc_unisoc\p7885\modules\<br>modules.tar.gz.gpg</td>
    <td>maili，wcn驱动</td>
  </tr>
  <!-- WS73 星闪驱动组 -->
  <tr>
    <td>plat_soc.ko</td>
    <td>\device_soc_unisoc\p7885\modules\ws73\<br>plat_soc.ko</td>
    <td>星闪内核驱动</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td>sle_soc.ko</td>
    <td>\device_soc_unisoc\p7885\modules\ws73\<br>sle_soc.ko</td>
    <td>星闪内核驱动</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td>aw87xxx_acf.bin</td>
    <td>\device_soc_unisoc\p7885\modules\audio\<br>aw87xxx_acf.bin</td>
    <td>aw87xxx功放固件</td>
    <td>N/A</td>
  </tr>
  <!-- 系统镜像组 -->
  <tr>
  <td rowspan="8">boot镜像打包</td>
    <td>boot.img</td>
    <td>\device_soc_unisoc\p7885\pac\ImageFiles\<br>boot.img</td>
    <td>内核镜像</td>
    <td>在内核6.6适配完成后，删除img，7月8号前在OH7.0版本中完成</td>
  </tr>
  <tr>
    <td>dtbo.img</td>
    <td>\device_soc_unisoc\p7885\pac\ImageFiles\<br>dtbo.img</td>
    <td>DTBO镜像</td>
    <td>在内核6.6适配完成后，删除img，7月8号前在OH7.0版本中完成</td>
  </tr>
  <!-- PAC 镜像组 -->
  <tr>
    <td>uboot.tar.gz.gpg</td>
    <td>\device_soc_unisoc\p7885\pac\ImageFiles\<br>uboot.tar.gz.gpg</td>
    <td>展锐fdl1， fdl2等闭源加密包</td>
    <td rowspan="6">N/A</td>
  </tr>
  <tr>
    <td>UpdatedPacCRC_Linux</td>
    <td>\device_soc_unisoc\p7885\pac\ImageFiles\Script\<br>UpdatedPacCRC_Linux</td>
    <td>PAC CRC校验</td>
  </tr>
  <tr>
    <td>logo.bin</td>
    <td>\device_soc_unisoc\p7885\pac\ImageFiles\<br>logo.bin</td>
    <td>开机视频或图片 Logo 资源</td>
  </tr>
  <tr>
    <td>prodnv.img</td>
    <td>\device_soc_unisoc\p7885\pac\ImageFiles\<br>prodnv.img</td>
    <td>产品校准、NV 数据分区镜像</td>
  </tr>
  <tr>
    <td>vendor_boot.img</td>
    <td>\device_soc_unisoc\p7885\pac\ImageFiles\<br>vendor_boot.img</td>
    <td>供应商启动分区镜像</td>
  </tr>
  <tr>
    <td>vendor_boot_sec.img</td>
    <td>\device_soc_unisoc\p7885\pac\ImageFiles\<br>vendor_boot_sec.img</td>
    <td>供应商启动分区加密镜像</td>
  </tr>
  <!-- GPU 驱动组 -->
  <tr>
    <td rowspan="25">GPU 图形库</td>
    <td>gpu.tar.gz.gpg</td>
    <td>\device_soc_unisoc\p7885\hardware\gpu\lib64\<br>gpu.tar.gz.gpg</td>
    <td>mali闭源加密包</td>
    <td>N/A</td>
  </tr>
  <!-- 图形接口适配层组（展示12行带详细说明） -->
  <tr>
    <td>libgralloctypes.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\frameworks\<br>native\libs\gralloc\types\lib64\libgralloctypes.z.so</td>
    <td rowspan="3">Unisoc需要配合OHOS接口</td>
    <td rowspan="3">后续删除so，7月8号前在OH7.0版本中完成</td>
  </tr>
  <tr>
    <td>libnativewindow.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\frameworks\<br>native\libs\nativewindow\lib64\libnativewindow.z.so</td>
  </tr>
  <tr>
    <td>libui.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\frameworks\<br>native\libs\ui\lib64\libui.z.so</td>
  </tr>
  <tr>
    <td>liballocator2.0.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\allocator\2.0\lib64\liballocator2.0.z.so</td>
    <td rowspan="11">Unisoc图形显示、内存分配 (Gralloc) ，Mapper的关联支持库</td>
    <td rowspan="11">需要配合mali库做统一整改，整改后删除so，7月8号前在OH7.0版本中完成</td>
  </tr>
  <tr>
    <td>liballocator3.0.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\allocator\3.0\lib64\liballocator3.0.z.so</td>
  </tr>
  <tr>
    <td>liballocator4.0.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\allocator\4.0\lib64\liballocator4.0.z.so</td>
  </tr>
  <tr>
    <td>libcommon1.0.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\common\1.0\lib64\libcommon1.0.z.so</td>
  </tr>
  <tr>
    <td>libcommon1.1.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\common\1.1\lib64\libcommon1.1.z.so</td>
  </tr>
  <tr>
    <td>libcommon1.2.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\common\1.2\lib64\libcommon1.2.z.so</td>
  </tr>
  <tr>
    <td>libcommon-V3-ndk.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\common\aidl\android\hardware\<br>graphics\common\lib64\libcommon-V3-ndk.z.so</td>
  </tr>
  <tr>
    <td>libmapper2.0.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\mapper\2.0\lib64\libmapper2.0.z.so</td>
  </tr>
  <tr>
    <td>libmapper2.1.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\mapper\2.1\lib64\libmapper2.1.z.so</td>
  </tr>
  <!-- 图形接口适配层组（续） -->
  <tr>
    <td>libmapper3.0.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\mapper\3.0\lib64\libmapper3.0.z.so</td>
  </tr>
  <tr>
    <td>libmapper4.0.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>interfaces\graphics\mapper\4.0\lib64\libmapper4.0.z.so</td>
  </tr>
  <tr>
    <td>libhardware.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>libhardware\lib64\libhardware.z.so</td>
    <td>加载gralloc.default.so</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td>libgralloc.default.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\hardware\<br>libhardware\modules\gralloc\lib64\libgralloc.default.z.so</td>
    <td>内存分配</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td>libgrallocusage.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\system\core\<br>libgrallocusage\lib64\libgrallocusage.z.so</td>
    <td>buffer usage转换</td>
    <td>Unisoc需要配合OHOS接口整改，整改后删除so，7月8号前在OH7.0版本中完成</td>
  </tr>
  <tr>
    <td>libsync_adapter.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\system\<br>core\libsync\lib64\libsync_adapter.z.so</td>
    <td rowspan="4">支撑工具</td>
    <td rowspan="4">整改后删除，7月8号前在OH7.0版本中完成</td>
  </tr>
  <tr>
    <td>libhidlbase.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\system\<br>libhidl\lib64\libhidlbase.z.so</td>
  </tr>
  <tr>
    <td>libhwbinder.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\system\<br>libhwbinder\lib64\libhwbinder.z.so</td>
  </tr>
  <tr>
    <td>libutils_wrapper.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\wrapper\lib64\<br>libutils_wrapper.z.so</td>
  </tr>
  <tr>
    <td>libion.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\system\memory\<br>libion\lib64\libion.z.so</td>
    <td rowspan="3">相机获取ion内存的API</td>
    <td rowspan="3">N/A</td>
  </tr>
  <tr>
    <td>libmemion.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\system\memory\<br>libmemion\lib64\libmemion.z.so</td>
  </tr>
  <tr>
    <td>libvideo_share_memory.z.so</td>
    <td>\device_soc_unisoc\p7885\unisoc_wrapper\system\memory\<br>libvideosharememory\lib64\libvideo_share_memory.z.so</td>
  </tr>
  </tbody>
</table>