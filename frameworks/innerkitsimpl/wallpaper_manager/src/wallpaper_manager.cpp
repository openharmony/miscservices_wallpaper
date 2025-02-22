/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include <mutex>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include "dfx_types.h"
#include "hilog_wrapper.h"
#include "hitrace_meter.h"
#include "wallpaper_service_proxy.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "i_wallpaper_service.h"
#include "image_source.h"
#include "image_type.h"
#include "image_packer.h"
#include "file_ex.h"
#include "file_deal.h"
#include "file_util.h"
#include "wallpaper_service_cb_stub.h"
#include "wallpaper_manager.h"

namespace OHOS {
using namespace MiscServices;
namespace WallpaperMgrService {
constexpr int OPTION_QUALITY = 100;
WallpaperManager::WallpaperManager() {}
WallpaperManager::~WallpaperManager() {}

void WallpaperManager::ResetService(const wptr<IRemoteObject>& remote)
{
    HILOG_INFO("Remote is dead, reset service instance");
    std::lock_guard<std::mutex> lock(wpProxyLock_);
    if (wpProxy_ != nullptr) {
        sptr<IRemoteObject> object = wpProxy_->AsObject();
        if ((object != nullptr) && (remote == object)) {
            object->RemoveDeathRecipient(deathRecipient_);
            wpProxy_ = nullptr;
        }
    }
}

sptr<IWallpaperService> WallpaperManager::GetService()
{
    std::lock_guard<std::mutex> lock(wpProxyLock_);
    if (wpProxy_ != nullptr) {
        return wpProxy_;
    }

    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        HILOG_ERROR("Get samgr failed");
        return nullptr;
    }
    sptr<IRemoteObject> object = samgr->GetSystemAbility(WALLPAPER_MANAGER_SERVICE_ID);
    if (object == nullptr) {
        HILOG_ERROR("Get wallpaper object from samgr failed");
        return nullptr;
    }

    if (deathRecipient_ == nullptr) {
        deathRecipient_ = new DeathRecipient();
    }

    if ((object->IsProxyObject()) && (!object->AddDeathRecipient(deathRecipient_))) {
        HILOG_ERROR("Failed to add death recipient");
    }

    HILOG_INFO("get remote object ok");
    wpProxy_ = iface_cast<WallpaperServiceProxy>(object);
    if (wpProxy_ == nullptr) {
        HILOG_ERROR("iface_cast failed");
    }
    return wpProxy_;
}

void WallpaperManager::DeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& remote)
{
    DelayedRefSingleton<WallpaperManager>::GetInstance().ResetService(remote);
}

template<typename F, typename... Args>
ErrCode WallpaperManager::CallService(F func, Args&&... args)
{
    auto service = GetService();
    if (service == nullptr) {
        HILOG_ERROR("get service failed");
        return ERR_DEAD_OBJECT;
    }

    ErrCode result = (service->*func)(std::forward<Args>(args)...);
    if (SUCCEEDED(result)) {
        return ERR_OK;
    }

    // Reset service instance if 'ERR_DEAD_OBJECT' happened.
    if (result == ERR_DEAD_OBJECT) {
        ResetService(service);
    }

    HILOG_ERROR("Callservice failed with: %{public}d", result);
    return result;
}

std::vector<RgbaColor> WallpaperManager::GetColors(int wallpaperType)
{
    std::vector<RgbaColor> tmp;
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return tmp;
    }
    return wpServerProxy->GetColors(wallpaperType);
}

bool WallpaperManager::SetWallpaper(std::string url, int wallpaperType)
{
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return false;
    }
    if (!OHOS::FileExists(url)) {
        HILOG_ERROR("file is not exist!");
        HILOG_ERROR("file is not exist! %{public}s", url.c_str());
        return false;
    }
    FILE *pixMap = std::fopen(url.c_str(), "rb");
    if (pixMap == nullptr) {
        HILOG_ERROR("fopen faild, %{public}s", strerror(errno));
        HILOG_ERROR("fopen faild, %{public}s", url.c_str());
        return false;
    }
    int fend = fseek(pixMap, 0, SEEK_END);
    if (fend != 0) {
        HILOG_ERROR("fseek faild");
        return false;
    }
    int length = ftell(pixMap);
    if (length <= 0) {
        HILOG_ERROR("ftell faild");
        return false;
    }
    int fset = fseek(pixMap, 0, SEEK_SET);
    if (fset != 0) {
        HILOG_ERROR("fseek faild");
        return false;
    }
    int closeRes = fclose(pixMap);
    if (closeRes != 0) {
        HILOG_ERROR("fclose faild");
        return false;
    }

    int fd = open(url.c_str(), O_RDONLY, 0660);
    if (fd < 0) {
        HILOG_ERROR("open file failed");
        ReporterFault(FaultType::SET_WALLPAPER_FAULT, FaultCode::RF_FD_INPUT_FAILED);
        return false;
    }
    StartAsyncTrace(HITRACE_TAG_MISC, "SetWallpaper", static_cast<int32_t>(TraceTaskId::SET_WALLPAPER));
    bool bRet = wpServerProxy->SetWallpaperByFD(fd, wallpaperType, length);
    FinishAsyncTrace(HITRACE_TAG_MISC, "SetWallpaper", static_cast<int32_t>(TraceTaskId::SET_WALLPAPER));
    return bRet;
}

bool WallpaperManager::SetWallpaper(std::unique_ptr<OHOS::Media::PixelMap> &pixelMap, int wallpaperType)
{
    auto wpServerProxy = GetService();
    std::string urlRet = "";
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return false;
    }

    std::stringbuf* stringBuf = new std::stringbuf();
    std::ostream ostream(stringBuf);
    int mapSize = WritePixelMapToStream(ostream, std::move(pixelMap));
    if (mapSize <= 0) {
        HILOG_ERROR("WritePixelMapToStream faild");
        return false;
    }
    char* buffer = new char[mapSize];
    stringBuf->sgetn(buffer, mapSize);

    int fd[2];
    pipe(fd);
    fcntl(fd[1], F_SETPIPE_SZ, mapSize);
    fcntl(fd[0], F_SETPIPE_SZ, mapSize);
    int32_t writeSize = write(fd[1], buffer, mapSize);
    if (writeSize != mapSize) {
        HILOG_ERROR("write to fd faild");
        ReporterFault(FaultType::SET_WALLPAPER_FAULT, FaultCode::RF_FD_INPUT_FAILED);
        return false;
    }
    close(fd[1]);
    return wpServerProxy->SetWallpaperByMap(fd[0], wallpaperType, mapSize);
}
int64_t WallpaperManager::WritePixelMapToStream(std::ostream &outputStream,
    std::unique_ptr< OHOS::Media::PixelMap> pixelMap)
{
    OHOS::Media::ImagePacker imagePacker;
    OHOS::Media::PackOption option;
    option.format = "image/jpeg";
    option.quality = OPTION_QUALITY;
    option.numberHint = 1;
    std::set<std::string> formats;
    uint32_t ret = imagePacker.GetSupportedFormats(formats);
    if (ret != 0) {
        HILOG_ERROR("image packer get supported format failed, ret=%{public}u.", ret);
    }

    imagePacker.StartPacking(outputStream, option);
    imagePacker.AddImage(*pixelMap);
    int64_t packedSize = 0;
    imagePacker.FinalizePacking(packedSize);
    HILOG_INFO("FrameWork WritePixelMapToFile End! packedSize=%{public}lld.", static_cast<long long>(packedSize));
    return packedSize;
}

int64_t WallpaperManager::WritePixelMapToFile(const std::string &filePath,
    std::unique_ptr< OHOS::Media::PixelMap> pixelMap)
{
    OHOS::Media::ImagePacker imagePacker;
    OHOS::Media::PackOption option;
    option.format = "image/jpeg";
    option.quality = OPTION_QUALITY;
    option.numberHint = 1;
    std::set<std::string> formats;
    uint32_t ret = imagePacker.GetSupportedFormats(formats);
    if (ret != 0) {
        HILOG_ERROR("image packer get supported format failed, ret=%{public}u.", ret);
    }
    imagePacker.StartPacking(filePath, option);
    imagePacker.AddImage(*pixelMap);
    int64_t packedSize = 0;
    imagePacker.FinalizePacking(packedSize);
    HILOG_INFO("FrameWork WritePixelMapToFile End! packedSize=%{public}lld.", static_cast<long long>(packedSize));
    return packedSize;
}
std::shared_ptr<OHOS::Media::PixelMap> WallpaperManager::GetPixelMap(int wallpaperType)
{
    HILOG_INFO("FrameWork GetPixelMap Start by FD");
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return nullptr;
    }
    IWallpaperService::mapFD mapFd = wpServerProxy->GetPixelMap(wallpaperType);
    uint32_t errorCode = 0;
    OHOS::Media::SourceOptions opts;
    opts.formatHint = "image/jpeg";
    HILOG_INFO(" CreateImageSource by FD");
    std::unique_ptr<OHOS::Media::ImageSource> imageSource =
        OHOS::Media::ImageSource::CreateImageSource(mapFd.fd, opts, errorCode);
    if (errorCode != 0) {
        HILOG_ERROR("ImageSource::CreateImageSource failed,errcode= %{public}d", errorCode);
        return nullptr;
    }
    OHOS::Media::DecodeOptions decodeOpts;
    HILOG_INFO(" CreatePixelMap");
    std::unique_ptr<OHOS::Media::PixelMap> tmp = imageSource->CreatePixelMap(decodeOpts, errorCode);

    if (errorCode != 0) {
        HILOG_ERROR("ImageSource::CreatePixelMap failed,errcode= %{public}d", errorCode);
        return nullptr;
    }
    close(mapFd.fd);
    return tmp;
}

int  WallpaperManager::GetWallpaperId(int wallpaperType)
{
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return -1;
    }
    return wpServerProxy->GetWallpaperId(wallpaperType);
}

int  WallpaperManager::GetWallpaperMinHeight()
{
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return -1;
    }
    return wpServerProxy->GetWallpaperMinHeight();
}

int  WallpaperManager::GetWallpaperMinWidth()
{
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return -1;
    }
    return wpServerProxy->GetWallpaperMinWidth();
}

bool WallpaperManager::IsChangePermitted()
{
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return false;
    }
    return wpServerProxy->IsChangePermitted();
}

bool WallpaperManager::IsOperationAllowed()
{
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return false;
    }
    return wpServerProxy->IsOperationAllowed();
}
bool WallpaperManager::ResetWallpaper(std::int32_t wallpaperType)
{
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return false;
    }
    return wpServerProxy->ResetWallpaper(wallpaperType);
}
bool WallpaperManager::ScreenshotLiveWallpaper(int wallpaperType, OHOS::Media::PixelMap pixelMap)
{
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return false;
    }
    return wpServerProxy->ScreenshotLiveWallpaper(wallpaperType, pixelMap);
}

bool WallpaperManager::On(std::shared_ptr<WallpaperColorChangeListener> listener)
{
    HILOG_DEBUG("WallpaperManager::On in");
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return false;
    }
    if (listener == nullptr) {
        HILOG_ERROR("listener is nullptr.");
        return false;
    }
    std::lock_guard<std::mutex> lck(listenerMapMutex_);

    if (registeredListeners_.count(listener.get()) == 1) {
        HILOG_ERROR("already listened");
        return false;
    }

    sptr<WallpaperColorChangeListenerClient> ipcListener =
            new (std::nothrow) WallpaperColorChangeListenerClient(listener);
    if (ipcListener == nullptr) {
        HILOG_ERROR("new WallpaperColorChangeListenerClient failed");
        return false;
    }
    bool status = wpServerProxy->On(ipcListener);
    if (status == false) {
        const auto temp = registeredListeners_.insert({ listener.get(), ipcListener });
        if (!temp.second) {
            HILOG_ERROR("local insert error");
            return false;
        }
    }
    HILOG_DEBUG("WallpaperManager::On out");
    return true;
}

bool WallpaperManager::Off(std::shared_ptr<WallpaperColorChangeListener> listener)
{
    HILOG_DEBUG("WallpaperManager::Off in");
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return false;
    }
    if (listener == nullptr) {
        HILOG_ERROR("listener is nullptr.");
        return false;
    }
    std::lock_guard<std::mutex> lck(listenerMapMutex_);
    auto it = registeredListeners_.find(listener.get());
    if (it == registeredListeners_.end()) {
        HILOG_ERROR("never listened");
        return true;
    }
    bool status = wpServerProxy->Off(it->second);
    if (status == false) {
        HILOG_ERROR("off failed code=%d.", ERR_NONE);
        return false;
    }
    registeredListeners_.erase(it);
    HILOG_DEBUG("WallpaperManager::Off out");
    return true;
}

JScallback WallpaperManager::GetCallback()
{
    return callback;
}

void WallpaperManager::SetCallback(bool (*cb) (int))
{
    callback = cb;
}

bool WallpaperManager::RegisterWallpaperCallback(bool (*callback)(int))
{
    HILOG_ERROR("  WallpaperManager::RegisterWallpaperCallback statrt");
    SetCallback(callback);
    auto wpServerProxy = GetService();
    if (wpServerProxy == nullptr) {
        HILOG_ERROR("Get proxy failed");
        return false;
    }

    if (callback == NULL) {
        HILOG_ERROR("callback is NULL.");
        return false;
    }
    HILOG_INFO("  WallpaperManager::RegisterWallpaperCallback");

    bool status = wpServerProxy->RegisterWallpaperCallback(new WallpaperServiceCbStub());
    if (status == false) {
        HILOG_ERROR("off failed code=%d.", ERR_NONE);
        return false;
    }

    return 0;
}

void WallpaperManager::ReporterFault(FaultType faultType, FaultCode faultCode)
{
    MiscServices::FaultMsg msg;
    msg.faultType = faultType;
    msg.errorCode = faultCode;
    Reporter::GetInstance().Fault().ReportRuntimeFault(msg);
}
} // namespace WallpaperMgrService
} // namespace OHOS