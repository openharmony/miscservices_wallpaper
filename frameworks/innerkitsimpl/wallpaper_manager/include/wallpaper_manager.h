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

#ifndef INNERKITS_WALLPAPER_MANAGER_H
#define INNERKITS_WALLPAPER_MANAGER_H

#include <vector>
#include <string>
#include <mutex>
#include <list>
#include <map>
#include "ipc_skeleton.h"
#include "wallpaper_manager_kits.h"
#include "i_wallpaper_service.h"
#include "singleton.h"
#include "reporter.h"

using JScallback = bool (*) (int);
namespace OHOS {
using namespace MiscServices;
namespace WallpaperMgrService {
class WallpaperManager final : public WallpaperManagerkits, public DelayedRefSingleton<WallpaperManager> {
    DECLARE_DELAYED_REF_SINGLETON(WallpaperManager);
public:
    DISALLOW_COPY_AND_MOVE(WallpaperManager);

    /**
    * Wallpaper set.
    * @param  uriOrPixelMap Wallpaper picture; wallpaperType Wallpaper type,
    * values for WALLPAPER_SYSTEM or WALLPAPER_LOCKSCREEN
    * @return  true or false
    */
    bool SetWallpaper(std::string url, int wallpaperType) final;

      /**
    * Wallpaper set.
    * @param  pixelMap:picture pixelMap struct; wallpaperType Wallpaper type,
    * values for WALLPAPER_SYSTEM or WALLPAPER_LOCKSCREEN
    * @return  true or false
    */
    bool SetWallpaper(std::unique_ptr<OHOS::Media::PixelMap> &pixelMap, int wallpaperType) final;

    /**
        *Obtains the default pixel map of a wallpaper of the specified type.
        * @param wallpaperType Wallpaper type, values for WALLPAPER_SYSTEM or WALLPAPER_LOCKSCREEN
        * @return image.PixelMap png type The bitmap file of wallpaper
        * @permission ohos.permission.GET_WALLPAPER
        * @permission ohos.permission.READ_USER_STORAGE
        * @systemapi Hide this for inner system use.
    */
    std::shared_ptr<OHOS::Media::PixelMap> GetPixelMap(int wallpaperType) final;

    /**
     * Obtains the WallpaperColorsCollection instance for the wallpaper of the specified type.
     * @param wallpaperType Wallpaper type, values for WALLPAPER_SYSTEM or WALLPAPER_LOCKSCREEN
     * @return RgbaColor type of array callback function
     */
    std::vector<RgbaColor> GetColors(int wallpaperType) final;

    /**
     * Obtains the ID of the wallpaper of the specified type.
     * @param wallpaperType Wallpaper type, values for WALLPAPER_SYSTEM or WALLPAPER_LOCKSCREEN
     * @return number type of callback function
     */
    int  GetWallpaperId(int wallpaperType) final;

     /**
     * Obtains the minimum height of the wallpaper.
     * @return number type of callback function
     */
    int  GetWallpaperMinHeight() final;

     /**
     * Obtains the minimum width of the wallpaper.
     * @return number type of callback function
     */
    int GetWallpaperMinWidth() final;

     /**
     * Checks whether to allow the application to change the wallpaper for the current user.
     * @return boolean type of callback function
     */
    bool IsChangePermitted() final;

    /**
     * Checks whether a user is allowed to set wallpapers.
     * @return boolean type of callback function
     */
    bool IsOperationAllowed() final;

     /**
     * Removes a wallpaper of the specified type and restores the default one.
     * @param wallpaperType  Wallpaper type, values for WALLPAPER_SYSTEM or WALLPAPER_LOCKSCREEN
     * @permission ohos.permission.SET_WALLPAPER
     */
    bool ResetWallpaper(std::int32_t wallpaperType) final;

    /**
     * Screen shot live wallpaper
     * @param scale
     * @param pixelFormat
     * @return image.PixelMap png type The bitmap file of wallpaper
     * @permission ohos.permission.CAPTURE_SCREEN
     * @systemapi Hide this for inner system use.
     * @return  true or false
     */
    bool ScreenshotLiveWallpaper(int wallpaperType, OHOS::Media::PixelMap pixelMap) final;

    /**
     * Registers a listener for wallpaper color changes to receive notifications about the changes.
     * @param type The incoming colorChange table open receiver pick a color change wallpaper wallpaper color changes
     * @param callback Provides dominant colors of the wallpaper.
     * @return  true or false
     */
    bool On(std::shared_ptr<WallpaperColorChangeListener> listener) final;

    /**
     * Registers a listener for wallpaper color changes to receive notifications about the changes.
     * @param type Incoming 'colorChange' table delete receiver to pick up a color change wallpaper wallpaper color
     * changes
     * @param callback Provides dominant colors of the wallpaper.
     */
    bool Off(std::shared_ptr<WallpaperColorChangeListener> listener) final;

    bool RegisterWallpaperCallback(bool (*callback) (int)) final;

    JScallback GetCallback() final;

    void SetCallback(bool (*cb) (int)) final;

    void ReporterFault(FaultType faultType, FaultCode faultCode);

private:
    class DeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        DeathRecipient() = default;
        ~DeathRecipient() final = default;
        DISALLOW_COPY_AND_MOVE(DeathRecipient);

        void OnRemoteDied(const wptr<IRemoteObject>& remote) final;
    };

    template<typename F, typename... Args>
    ErrCode CallService(F func, Args&&... args);

    void ResetService(const wptr<IRemoteObject>& remote);
    sptr<IWallpaperService> GetService();
    int64_t WritePixelMapToFile(const std::string &filePath, std::unique_ptr< OHOS::Media::PixelMap> pixelMap);
    int64_t WritePixelMapToStream(std::ostream &outputStream, std::unique_ptr< OHOS::Media::PixelMap> pixelMap);

    sptr<IWallpaperService> wpProxy_ {};
    sptr<IRemoteObject::DeathRecipient> deathRecipient_ {};
    std::mutex wpProxyLock_;
    std::map<WallpaperColorChangeListener *, sptr<IWallpaperColorChangeListener>> registeredListeners_;
    std::mutex listenerMapMutex_;
    bool (*callback) (int);
};
}
}
#endif

