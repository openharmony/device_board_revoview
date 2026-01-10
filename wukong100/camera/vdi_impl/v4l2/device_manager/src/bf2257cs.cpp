/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bf2257cs.h"
#include <vector>

namespace OHOS::Camera {
IMPLEMENT_SENSOR(Bf2257cs)
Bf2257cs::Bf2257cs() : ISensor("Bf2257cs") {}

Bf2257cs::~Bf2257cs() {}
void Bf2257cs::InitPhysicalSize(Camera::CameraMetadata& camera_meta_data)
{
    ISensor::InitPhysicalSize(camera_meta_data);
}
void Bf2257cs::InitAntiBandingModes(Camera::CameraMetadata& camera_meta_data)
{
    ISensor::InitAntiBandingModes(camera_meta_data);
}
void Bf2257cs::InitAvailableModes(Camera::CameraMetadata& camera_meta_data)
{
    ISensor::InitAvailableModes(camera_meta_data);
}
void Bf2257cs::InitFpsTarget(Camera::CameraMetadata& camera_meta_data)
{
    ISensor::InitFpsTarget(camera_meta_data);
}
void Bf2257cs::InitCompensationRange(Camera::CameraMetadata& camera_meta_data)
{
    ISensor::InitCompensationRange(camera_meta_data);
}

void Bf2257cs::InitAwbModes(Camera::CameraMetadata& camera_meta_data)
{
    ISensor::InitAwbModes(camera_meta_data);
}

void Bf2257cs::InitSensitivityRange(Camera::CameraMetadata& camera_meta_data)
{
    ISensor::InitSensitivityRange(camera_meta_data);
}

void Bf2257cs::Init(Camera::CameraMetadata& camera_metaData)
{
    InitPhysicalSize(camera_metaData);
    InitAntiBandingModes(camera_metaData);
    InitAvailableModes(camera_metaData);
    InitFpsTarget(camera_metaData);
    InitCompensationRange(camera_metaData);

    const camera_rational_t aeCompensationStep[] = {{0, 1}};
    camera_metaData.addEntry(OHOS_CONTROL_AE_COMPENSATION_STEP, aeCompensationStep, 1);

    InitAwbModes(camera_metaData);
    InitSensitivityRange(camera_metaData);

    uint8_t faceDetectMode = OHOS_CAMERA_FACE_DETECT_MODE_OFF;
    camera_metaData.addEntry(OHOS_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);
}
} // namespace OHOS::Camera
