/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "js_wallpaper_extension_context.h"

#include <cstdint>

#include "hilog_wrapper.h"
#include "js_extension_context.h"
#include "js_data_struct_converter.h"
#include "js_runtime.h"
#include "js_runtime_utils.h"
#include "napi/native_api.h"
#include "napi_common_want.h"
#include "napi_common_util.h"
#include "napi_remote_object.h"
#include "napi_common_start_options.h"
#include "start_options.h"

namespace OHOS {
namespace AbilityRuntime {
namespace {
constexpr int32_t INDEX_ZERO = 0;
constexpr int32_t INDEX_ONE = 1;
constexpr int32_t INDEX_TWO = 2;
constexpr int32_t ERROR_CODE_ONE = 1;
constexpr int32_t ERROR_CODE_TWO = 2;
constexpr size_t ARGC_ZERO = 0;
constexpr size_t ARGC_ONE = 1;
constexpr size_t ARGC_TWO = 2;
constexpr size_t ARGC_THREE = 3;
constexpr size_t ARGC_FOUR = 4;

class JsWallpaperExtensionContext final {
public:
    explicit JsWallpaperExtensionContext(const std::shared_ptr<WallpaperExtensionContext>& context) : context_(context)
    {
    }
    ~JsWallpaperExtensionContext() = default;

    static void Finalizer(NativeEngine* engine, void* data, void* hint)
    {
        HILOG_INFO("JsAbilityContext::Finalizer is called");
        std::unique_ptr<JsWallpaperExtensionContext>(static_cast<JsWallpaperExtensionContext*>(data));
    }

    static NativeValue* StartAbility(NativeEngine* engine, NativeCallbackInfo* info)
    {
        JsWallpaperExtensionContext* me = CheckParamsAndGetThis<JsWallpaperExtensionContext>(engine, info);
        return (me != nullptr) ? me->OnStartAbility(*engine, *info) : nullptr;
    }

    static NativeValue* StartAbilityWithAccount(NativeEngine* engine, NativeCallbackInfo* info)
    {
        JsWallpaperExtensionContext* me = CheckParamsAndGetThis<JsWallpaperExtensionContext>(engine, info);
        return (me != nullptr) ? me->OnStartAbilityWithAccount(*engine, *info) : nullptr;
    }

    static NativeValue* ConnectAbilityWithAccount(NativeEngine* engine, NativeCallbackInfo* info)
    {
        JsWallpaperExtensionContext* me = CheckParamsAndGetThis<JsWallpaperExtensionContext>(engine, info);
        return (me != nullptr) ? me->OnConnectAbilityWithAccount(*engine, *info) : nullptr;
    }

    static NativeValue* TerminateAbility(NativeEngine* engine, NativeCallbackInfo* info)
    {
        JsWallpaperExtensionContext* me = CheckParamsAndGetThis<JsWallpaperExtensionContext>(engine, info);
        return (me != nullptr) ? me->OnTerminateAbility(*engine, *info) : nullptr;
    }

    static NativeValue* ConnectAbility(NativeEngine* engine, NativeCallbackInfo* info)
    {
        JsWallpaperExtensionContext* me = CheckParamsAndGetThis<JsWallpaperExtensionContext>(engine, info);
        return (me != nullptr) ? me->OnConnectAbility(*engine, *info) : nullptr;
    }

    static NativeValue* DisconnectAbility(NativeEngine* engine, NativeCallbackInfo* info)
    {
        JsWallpaperExtensionContext* me = CheckParamsAndGetThis<JsWallpaperExtensionContext>(engine, info);
        return (me != nullptr) ? me->OnDisconnectAbility(*engine, *info) : nullptr;
    }
private:
    std::weak_ptr<WallpaperExtensionContext> context_;

    NativeValue* OnStartAbility(NativeEngine& engine, NativeCallbackInfo& info)
    {
        HILOG_INFO("OnStartAbility is called");
        // only support one or two or three params
        if (info.argc != ARGC_ONE && info.argc != ARGC_TWO && info.argc != ARGC_THREE) {
            HILOG_ERROR("Not enough params");
            return engine.CreateUndefined();
        }

        decltype(info.argc) unwrapArgc = 0;
        AAFwk::Want want;
        OHOS::AppExecFwk::UnwrapWant(reinterpret_cast<napi_env>(&engine),
            reinterpret_cast<napi_value>(info.argv[INDEX_ZERO]), want);
        HILOG_INFO("%{public}s bundlename:%{public}s abilityname:%{public}s",
            __func__,
            want.GetBundle().c_str(),
            want.GetElement().GetAbilityName().c_str());
        unwrapArgc++;

        AAFwk::StartOptions startOptions;
        if (info.argc > ARGC_ONE && info.argv[INDEX_ONE]->TypeOf() == NATIVE_OBJECT) {
            HILOG_INFO("OnStartAbility start options is used.");
            AppExecFwk::UnwrapStartOptions(reinterpret_cast<napi_env>(&engine),
                reinterpret_cast<napi_value>(info.argv[INDEX_ONE]), startOptions);
            unwrapArgc++;
        }

        AsyncTask::CompleteCallback complete =
            [weak = context_, want, startOptions, unwrapArgc](NativeEngine& engine, AsyncTask& task, int32_t status) {
                HILOG_INFO("startAbility begin");
                auto context = weak.lock();
                if (!context) {
                    HILOG_WARN("context is released");
                    task.Reject(engine, CreateJsError(engine, ERROR_CODE_ONE, "Context is released"));
                    return;
                }

                ErrCode errcode = ERR_OK;
                (unwrapArgc == 1) ? errcode = context->StartAbility(want) :
                    errcode = context->StartAbility(want, startOptions);
                if (errcode == 0) {
                    task.Resolve(engine, engine.CreateUndefined());
                } else {
                    task.Reject(engine, CreateJsError(engine, errcode, "Start Ability failed."));
                }
            };

        NativeValue* lastParam = (info.argc == unwrapArgc) ? nullptr : info.argv[unwrapArgc];
        NativeValue* result = nullptr;
        AsyncTask::Schedule("WallpaperExtensionContext::OnStartAbility",
            engine, CreateAsyncTaskWithLastParam(engine, lastParam, nullptr, std::move(complete), &result));
        return result;
    }

    NativeValue* OnStartAbilityWithAccount(NativeEngine& engine, NativeCallbackInfo& info)
    {
        HILOG_INFO("OnStartAbilityWithAccount is called");
        // only support two or three or four params
        if (info.argc != ARGC_TWO && info.argc != ARGC_THREE && info.argc != ARGC_FOUR) {
            HILOG_ERROR("Not enough params");
            return engine.CreateUndefined();
        }

        decltype(info.argc) unwrapArgc = 0;
        AAFwk::Want want;
        OHOS::AppExecFwk::UnwrapWant(reinterpret_cast<napi_env>(&engine),
            reinterpret_cast<napi_value>(info.argv[INDEX_ZERO]), want);
        HILOG_INFO("%{public}s bundlename:%{public}s abilityname:%{public}s",
            __func__,
            want.GetBundle().c_str(),
            want.GetElement().GetAbilityName().c_str());
        unwrapArgc++;

        int32_t accountId = 0;
        if (!OHOS::AppExecFwk::UnwrapInt32FromJS2(reinterpret_cast<napi_env>(&engine),
            reinterpret_cast<napi_value>(info.argv[INDEX_ONE]), accountId)) {
            HILOG_INFO("%{public}s called, the second parameter is invalid.", __func__);
            return engine.CreateUndefined();
        }
        HILOG_INFO("%{public}d accountId:", accountId);
        unwrapArgc++;

        AAFwk::StartOptions startOptions;
        if (info.argc > ARGC_TWO && info.argv[INDEX_TWO]->TypeOf() == NATIVE_OBJECT) {
            HILOG_INFO("OnStartAbilityWithAccount start options is used.");
            AppExecFwk::UnwrapStartOptions(reinterpret_cast<napi_env>(&engine),
                reinterpret_cast<napi_value>(info.argv[INDEX_TWO]), startOptions);
            unwrapArgc++;
        }

        AsyncTask::CompleteCallback complete =
            [weak = context_, want, accountId, startOptions, unwrapArgc](
                NativeEngine& engine, AsyncTask& task, int32_t status) {
                    HILOG_INFO("startAbility begin");
                    auto context = weak.lock();
                    if (!context) {
                        HILOG_WARN("context is released");
                        task.Reject(engine, CreateJsError(engine, ERROR_CODE_ONE, "Context is released"));
                        return;
                    }

                    ErrCode errcode = ERR_OK;
                    (unwrapArgc == ARGC_TWO) ? errcode = context->StartAbilityWithAccount(want, accountId) :
                    errcode = context->StartAbilityWithAccount(want, accountId, startOptions);
                    if (errcode == 0) {
                        task.Resolve(engine, engine.CreateUndefined());
                    } else {
                        task.Reject(engine, CreateJsError(engine, errcode, "Start Ability failed."));
                    }
                };

        NativeValue* lastParam = (info.argc == unwrapArgc) ? nullptr : info.argv[unwrapArgc];
        NativeValue* result = nullptr;
        AsyncTask::Schedule("WallpaperExtensionContext::OnStartAbilityWithAccount",
            engine, CreateAsyncTaskWithLastParam(engine, lastParam, nullptr, std::move(complete), &result));
        return result;
    }

    NativeValue* OnTerminateAbility(NativeEngine& engine, NativeCallbackInfo& info)
    {
        HILOG_INFO("OnTerminateAbility is called");
        // only support one or zero params
        if (info.argc != ARGC_ZERO && info.argc != ARGC_ONE) {
            HILOG_ERROR("Not enough params");
            return engine.CreateUndefined();
        }

        AsyncTask::CompleteCallback complete =
            [weak = context_](NativeEngine& engine, AsyncTask& task, int32_t status) {
                HILOG_INFO("TerminateAbility begin");
                auto context = weak.lock();
                if (!context) {
                    HILOG_WARN("context is released");
                    task.Reject(engine, CreateJsError(engine, ERROR_CODE_ONE, "Context is released"));
                    return;
                }

                auto errcode = context->TerminateAbility();
                if (errcode == 0) {
                    task.Resolve(engine, engine.CreateUndefined());
                } else {
                    task.Reject(engine, CreateJsError(engine, errcode, "Terminate Ability failed."));
                }
            };

        NativeValue* lastParam = (info.argc == ARGC_ZERO) ? nullptr : info.argv[INDEX_ZERO];
        NativeValue* result = nullptr;
        AsyncTask::Schedule("WallpaperExtensionContext::OnTerminateAbility",
            engine, CreateAsyncTaskWithLastParam(engine, lastParam, nullptr, std::move(complete), &result));
        return result;
    }

    NativeValue* OnConnectAbility(NativeEngine& engine, NativeCallbackInfo& info)
    {
        HILOG_INFO("OnConnectAbility is called");
        // only support two params
        if (info.argc != ARGC_TWO) {
            HILOG_ERROR("Not enough params");
            return engine.CreateUndefined();
        }

        // unwrap want
        AAFwk::Want want;
        OHOS::AppExecFwk::UnwrapWant(reinterpret_cast<napi_env>(&engine),
            reinterpret_cast<napi_value>(info.argv[INDEX_ZERO]), want);
        HILOG_INFO("%{public}s bundlename:%{public}s abilityname:%{public}s",
            __func__,
            want.GetBundle().c_str(),
            want.GetElement().GetAbilityName().c_str());
        // unwarp connection
        sptr<JSWallpaperExtensionConnection> connection = new JSWallpaperExtensionConnection(engine);
        connection->SetJsConnectionObject(info.argv[1]);
        int64_t connectId = serialNumber_;
        ConnecttionKey key;
        key.id = serialNumber_;
        key.want = want;
        connects_.emplace(key, connection);
        if (serialNumber_ < INT64_MAX) {
            serialNumber_++;
        } else {
            serialNumber_ = 0;
        }
        HILOG_INFO("%{public}s not find connection, make new one:%{public}p.", __func__, connection.GetRefPtr());
        AsyncTask::CompleteCallback complete =
            [weak = context_, want, connection, connectId](NativeEngine& engine, AsyncTask& task, int32_t status) {
                HILOG_INFO("OnConnectAbility begin");
                auto context = weak.lock();
                if (!context) {
                    HILOG_WARN("context is released");
                    task.Reject(engine, CreateJsError(engine, ERROR_CODE_ONE, "Context is released"));
                    return;
                }
                HILOG_INFO("context->ConnectAbility connection:%{public}d", (int32_t)connectId);
                if (!context->ConnectAbility(want, connection)) {
                    connection->CallJsFailed(ERROR_CODE_ONE);
                }
                task.Resolve(engine, engine.CreateUndefined());
            };
        NativeValue* result = nullptr;
        AsyncTask::Schedule("WallpaperExtensionContext::OnConnectAbility",
            engine, CreateAsyncTaskWithLastParam(engine, nullptr, nullptr, std::move(complete), &result));
        return engine.CreateNumber(connectId);
    }

    NativeValue* OnConnectAbilityWithAccount(NativeEngine& engine, NativeCallbackInfo& info)
    {
        HILOG_INFO("OnConnectAbilityWithAccount is called");
        // only support three params
        if (info.argc != ARGC_THREE) {
            HILOG_ERROR("Not enough params");
            return engine.CreateUndefined();
        }

        // unwrap want
        AAFwk::Want want;
        OHOS::AppExecFwk::UnwrapWant(reinterpret_cast<napi_env>(&engine),
            reinterpret_cast<napi_value>(info.argv[INDEX_ZERO]), want);
        HILOG_INFO("%{public}s bundlename:%{public}s abilityname:%{public}s",
            __func__,
            want.GetBundle().c_str(),
            want.GetElement().GetAbilityName().c_str());

        int32_t accountId = 0;
        if (!OHOS::AppExecFwk::UnwrapInt32FromJS2(reinterpret_cast<napi_env>(&engine),
            reinterpret_cast<napi_value>(info.argv[INDEX_ONE]), accountId)) {
            HILOG_INFO("%{public}s called, the second parameter is invalid.", __func__);
            return engine.CreateUndefined();
        }

        // unwarp connection
        sptr<JSWallpaperExtensionConnection> connection = new JSWallpaperExtensionConnection(engine);
        connection->SetJsConnectionObject(info.argv[1]);
        int64_t connectId = serialNumber_;
        ConnecttionKey key;
        key.id = serialNumber_;
        key.want = want;
        connects_.emplace(key, connection);
        if (serialNumber_ < INT64_MAX) {
            serialNumber_++;
        } else {
            serialNumber_ = 0;
        }
        HILOG_INFO("%{public}s not find connection, make new one:%{public}p.", __func__, connection.GetRefPtr());
        AsyncTask::CompleteCallback complete =
            [weak = context_, want, accountId, connection, connectId](
                NativeEngine& engine, AsyncTask& task, int32_t status) {
                    HILOG_INFO("OnConnectAbilityWithAccount begin");
                    auto context = weak.lock();
                    if (!context) {
                        HILOG_WARN("context is released");
                        task.Reject(engine, CreateJsError(engine, ERROR_CODE_ONE, "Context is released"));
                        return;
                    }
                    HILOG_INFO("context->ConnectAbilityWithAccount connection:%{public}d", (int32_t)connectId);
                    if (!context->ConnectAbilityWithAccount(want, accountId, connection)) {
                        connection->CallJsFailed(ERROR_CODE_ONE);
                    }
                    task.Resolve(engine, engine.CreateUndefined());
                };
        NativeValue* result = nullptr;
        AsyncTask::Schedule("WallpaperExtensionContext::OnConnectAbilityWithAccount",
            engine, CreateAsyncTaskWithLastParam(engine, nullptr, nullptr, std::move(complete), &result));
        return engine.CreateNumber(connectId);
    }

    NativeValue* OnDisconnectAbility(NativeEngine& engine, NativeCallbackInfo& info)
    {
        HILOG_INFO("OnDisconnectAbility is called");
        // only support one or two params
        if (info.argc != ARGC_ONE && info.argc != ARGC_TWO) {
            HILOG_ERROR("Not enough params");
            return engine.CreateUndefined();
        }

        // unwrap want
        AAFwk::Want want;
        // unwrap connectId
        int64_t connectId = -1;
        sptr<JSWallpaperExtensionConnection> connection = nullptr;
        napi_get_value_int64(reinterpret_cast<napi_env>(&engine),
            reinterpret_cast<napi_value>(info.argv[INDEX_ZERO]), &connectId);
        HILOG_INFO("OnDisconnectAbility connection:%{public}d", (int32_t)connectId);
        auto item = std::find_if(connects_.begin(), connects_.end(),
            [&connectId](
                const std::map<ConnecttionKey, sptr<JSWallpaperExtensionConnection>>::value_type &obj) {
                    return connectId == obj.first.id;
            });
        if (item != connects_.end()) {
            // match id
            want = item->first.want;
            connection = item->second;
            HILOG_INFO("%{public}s find conn ability:%{public}p exist", __func__, item->second.GetRefPtr());
        } else {
            HILOG_INFO("%{public}s not find conn exist.", __func__);
        }
        // begin disconnect
        AsyncTask::CompleteCallback complete =
            [weak = context_, want, connection](
                NativeEngine& engine, AsyncTask& task, int32_t status) {
                HILOG_INFO("OnDisconnectAbility begin");
                auto context = weak.lock();
                if (!context) {
                    HILOG_WARN("context is released");
                    task.Reject(engine, CreateJsError(engine, ERROR_CODE_ONE, "Context is released"));
                    return;
                }
                if (connection == nullptr) {
                    HILOG_WARN("connection nullptr");
                    task.Reject(engine, CreateJsError(engine, ERROR_CODE_TWO, "not found connection"));
                    return;
                }
                HILOG_INFO("context->DisconnectAbility");
                auto errcode = context->DisconnectAbility(want, connection);
                errcode == 0 ? task.Resolve(engine, engine.CreateUndefined()) :
                    task.Reject(engine, CreateJsError(engine, errcode, "Disconnect Ability failed."));
            };

        NativeValue* lastParam = (info.argc == ARGC_ONE) ? nullptr : info.argv[INDEX_ONE];
        NativeValue* result = nullptr;
        AsyncTask::Schedule("WallpaperExtensionContext::OnDisconnectAbility",
            engine, CreateAsyncTaskWithLastParam(engine, lastParam, nullptr, std::move(complete), &result));
        return result;
    }
};
} // namespace

NativeValue* CreateJsMetadata(NativeEngine& engine, const AppExecFwk::Metadata &Info)
{
    HILOG_INFO("CreateJsMetadata");
    NativeValue* objValue = engine.CreateObject();
    NativeObject* object = ConvertNativeValueTo<NativeObject>(objValue);

    object->SetProperty("name", CreateJsValue(engine, Info.name));
    object->SetProperty("value", CreateJsValue(engine, Info.value));
    object->SetProperty("resource", CreateJsValue(engine, Info.resource));
    return objValue;
}

NativeValue* CreateJsMetadataArray(NativeEngine& engine, const std::vector<AppExecFwk::Metadata> &info)
{
    HILOG_INFO("CreateJsMetadataArray");
    NativeValue* arrayValue = engine.CreateArray(info.size());
    NativeArray* array = ConvertNativeValueTo<NativeArray>(arrayValue);
    uint32_t index = 0;
    for (const auto& item : info) {
        array->SetElement(index++, CreateJsMetadata(engine, item));
    }
    return arrayValue;
}

NativeValue* CreateJsExtensionAbilityInfo(NativeEngine& engine, const AppExecFwk::ExtensionAbilityInfo& info)
{
    HILOG_INFO("CreateJsExtensionAbilityInfo");
    NativeValue* objValue = engine.CreateObject();
    NativeObject* object = ConvertNativeValueTo<NativeObject>(objValue);
    object->SetProperty("bundleName", CreateJsValue(engine, info.bundleName));
    object->SetProperty("moduleName", CreateJsValue(engine, info.moduleName));
    object->SetProperty("name", CreateJsValue(engine, info.name));
    object->SetProperty("labelId", CreateJsValue(engine, info.labelId));
    object->SetProperty("descriptionId", CreateJsValue(engine, info.descriptionId));
    object->SetProperty("iconId", CreateJsValue(engine, info.iconId));
    object->SetProperty("isVisible", CreateJsValue(engine, info.visible));
    object->SetProperty("extensionAbilityType", CreateJsValue(engine, info.type));
    NativeValue *permissionArrayValue = engine.CreateArray(info.permissions.size());
    NativeArray *permissionArray = ConvertNativeValueTo<NativeArray>(permissionArrayValue);
    if (permissionArray != nullptr) {
        int index = 0;
        for (auto permission : info.permissions) {
            permissionArray->SetElement(index++, CreateJsValue(engine, permission));
        }
    }
    object->SetProperty("permissions", permissionArrayValue);
    object->SetProperty("applicationInfo", CreateJsApplicationInfo(engine, info.applicationInfo));
    object->SetProperty("metadata", CreateJsMetadataArray(engine, info.metadata));
    object->SetProperty("enabled", CreateJsValue(engine, info.enabled));
    object->SetProperty("readPermission", CreateJsValue(engine, info.readPermission));
    object->SetProperty("writePermission", CreateJsValue(engine, info.writePermission));
    return objValue;
}

NativeValue* CreateJsWallpaperExtensionContext(NativeEngine& engine, std::shared_ptr<WallpaperExtensionContext> context)
{
    HILOG_INFO("CreateJsWallpaperExtensionContext begin");
    NativeValue* objValue = CreateJsExtensionContext(engine, context);
    NativeObject* object = ConvertNativeValueTo<NativeObject>(objValue);

    std::unique_ptr<JsWallpaperExtensionContext> jsContext = std::make_unique<JsWallpaperExtensionContext>(context);
    object->SetNativePointer(jsContext.release(), JsWallpaperExtensionContext::Finalizer, nullptr);

    // make handler
    handler_ = std::make_shared<AppExecFwk::EventHandler>(AppExecFwk::EventRunner::GetMainEventRunner());

    BindNativeFunction(engine, *object, "startAbility", JsWallpaperExtensionContext::StartAbility);
    BindNativeFunction(engine, *object, "terminateSelf", JsWallpaperExtensionContext::TerminateAbility);
    BindNativeFunction(engine, *object, "connectAbility", JsWallpaperExtensionContext::ConnectAbility);
    BindNativeFunction(engine, *object, "disconnectAbility", JsWallpaperExtensionContext::DisconnectAbility);
    BindNativeFunction(
        engine, *object, "startAbilityWithAccount", JsWallpaperExtensionContext::StartAbilityWithAccount);
    BindNativeFunction(
        engine, *object, "connectAbilityWithAccount", JsWallpaperExtensionContext::ConnectAbilityWithAccount);

    if (context) {
        HILOG_INFO("Set ExtensionAbilityInfo Property");
        auto abilityInfo = context->GetAbilityInfo();
        auto hapModuleInfo = context->GetHapModuleInfo();
        if (abilityInfo && hapModuleInfo) {
            auto isExist = [&abilityInfo](const AppExecFwk::ExtensionAbilityInfo &info) {
                HILOG_INFO("%{public}s, %{public}s", info.bundleName.c_str(), info.name.c_str());
                return info.bundleName == abilityInfo->bundleName && info.name == abilityInfo->name;
            };
            auto infoIter = std::find_if(
                hapModuleInfo->extensionInfos.begin(), hapModuleInfo->extensionInfos.end(), isExist);
            if (infoIter == hapModuleInfo->extensionInfos.end()) {
                HILOG_INFO("Get target fail.");
                return objValue;
            }
            object->SetProperty("extensionAbilityInfo", CreateJsExtensionAbilityInfo(engine, *infoIter));
        }
    }

    return objValue;
}

JSWallpaperExtensionConnection::JSWallpaperExtensionConnection(NativeEngine& engine) : engine_(engine) {}

JSWallpaperExtensionConnection::~JSWallpaperExtensionConnection() = default;

void JSWallpaperExtensionConnection::OnAbilityConnectDone(const AppExecFwk::ElementName &element,
    const sptr<IRemoteObject> &remoteObject, int resultCode)
{
    HILOG_INFO("OnAbilityConnectDone begin, resultCode:%{public}d", resultCode);
    if (handler_ == nullptr) {
        HILOG_INFO("handler_ nullptr");
        return;
    }
    wptr<JSWallpaperExtensionConnection> connection = this;
    auto task = [connection, element, remoteObject, resultCode]() {
        sptr<JSWallpaperExtensionConnection> connectionSptr = connection.promote();
        if (!connectionSptr) {
            HILOG_INFO("connectionSptr nullptr");
            return;
        }
        connectionSptr->HandleOnAbilityConnectDone(element, remoteObject, resultCode);
    };
    handler_->PostTask(task, "OnAbilityConnectDone");
}

void JSWallpaperExtensionConnection::HandleOnAbilityConnectDone(const AppExecFwk::ElementName &element,
    const sptr<IRemoteObject> &remoteObject, int resultCode)
{
    HILOG_INFO("HandleOnAbilityConnectDone begin, resultCode:%{public}d", resultCode);
    // wrap ElementName
    napi_value napiElementName = OHOS::AppExecFwk::WrapElementName(reinterpret_cast<napi_env>(&engine_), element);
    NativeValue* nativeElementName = reinterpret_cast<NativeValue*>(napiElementName);

    // wrap RemoteObject
    HILOG_INFO("OnAbilityConnectDone begin NAPI_ohos_rpc_CreateJsRemoteObject");
    napi_value napiRemoteObject = NAPI_ohos_rpc_CreateJsRemoteObject(
        reinterpret_cast<napi_env>(&engine_), remoteObject);
    NativeValue* nativeRemoteObject = reinterpret_cast<NativeValue*>(napiRemoteObject);
    NativeValue* argv[] = {nativeElementName, nativeRemoteObject};
    if (jsConnectionObject_ == nullptr) {
        HILOG_ERROR("jsConnectionObject_ nullptr");
        return;
    }
    NativeValue* value = jsConnectionObject_->Get();
    NativeObject* obj = ConvertNativeValueTo<NativeObject>(value);
    if (obj == nullptr) {
        HILOG_ERROR("Failed to get object");
        return;
    }
    NativeValue* methodOnConnect = obj->GetProperty("onConnect");
    if (methodOnConnect == nullptr) {
        HILOG_ERROR("Failed to get onConnect from object");
        return;
    }
    HILOG_INFO("JSWallpaperExtensionConnection::CallFunction onConnect, success");
    engine_.CallFunction(value, methodOnConnect, argv, ARGC_TWO);
    HILOG_INFO("OnAbilityConnectDone end");
}

void JSWallpaperExtensionConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode)
{
    HILOG_INFO("OnAbilityDisconnectDone begin, resultCode:%{public}d", resultCode);
    if (handler_ == nullptr) {
        HILOG_INFO("handler_ nullptr");
        return;
    }
    wptr<JSWallpaperExtensionConnection> connection = this;
    auto task = [connection, element, resultCode]() {
        sptr<JSWallpaperExtensionConnection> connectionSptr = connection.promote();
        if (!connectionSptr) {
            HILOG_INFO("connectionSptr nullptr");
            return;
        }
        connectionSptr->HandleOnAbilityDisconnectDone(element, resultCode);
    };
    handler_->PostTask(task, "OnAbilityDisconnectDone");
}

void JSWallpaperExtensionConnection::HandleOnAbilityDisconnectDone(const AppExecFwk::ElementName &element,
    int resultCode)
{
    HILOG_INFO("HandleOnAbilityDisconnectDone begin, resultCode:%{public}d", resultCode);
    napi_value napiElementName = OHOS::AppExecFwk::WrapElementName(reinterpret_cast<napi_env>(&engine_), element);
    NativeValue* nativeElementName = reinterpret_cast<NativeValue*>(napiElementName);
    NativeValue* argv[] = {nativeElementName};
    if (jsConnectionObject_ == nullptr) {
        HILOG_ERROR("jsConnectionObject_ nullptr");
        return;
    }
    NativeValue* value = jsConnectionObject_->Get();
    NativeObject* obj = ConvertNativeValueTo<NativeObject>(value);
    if (obj == nullptr) {
        HILOG_ERROR("Failed to get object");
        return;
    }

    NativeValue* method = obj->GetProperty("onDisconnect");
    if (method == nullptr) {
        HILOG_ERROR("Failed to get onDisconnect from object");
        return;
    }

    // release connect
    HILOG_INFO("OnAbilityDisconnectDone connects_.size:%{public}zu", connects_.size());
    std::string bundleName = element.GetBundleName();
    std::string abilityName = element.GetAbilityName();
    auto item = std::find_if(connects_.begin(),
        connects_.end(),
        [bundleName, abilityName](
            const std::map<ConnecttionKey, sptr<JSWallpaperExtensionConnection>>::value_type &obj) {
            return (bundleName == obj.first.want.GetBundle()) &&
                   (abilityName == obj.first.want.GetElement().GetAbilityName());
        });
    if (item != connects_.end()) {
        // match bundlename && abilityname
        connects_.erase(item);
        HILOG_INFO("OnAbilityDisconnectDone erase connects_.size:%{public}zu", connects_.size());
    }
    HILOG_INFO("OnAbilityDisconnectDone CallFunction success");
    engine_.CallFunction(value, method, argv, ARGC_ONE);
}

void JSWallpaperExtensionConnection::SetJsConnectionObject(NativeValue* jsConnectionObject)
{
    jsConnectionObject_ = std::unique_ptr<NativeReference>(engine_.CreateReference(jsConnectionObject, 1));
}

void JSWallpaperExtensionConnection::CallJsFailed(int32_t errorCode)
{
    HILOG_INFO("CallJsFailed begin");
    if (jsConnectionObject_ == nullptr) {
        HILOG_ERROR("jsConnectionObject_ nullptr");
        return;
    }
    NativeValue* value = jsConnectionObject_->Get();
    NativeObject* obj = ConvertNativeValueTo<NativeObject>(value);
    if (obj == nullptr) {
        HILOG_ERROR("Failed to get object");
        return;
    }

    NativeValue* method = obj->GetProperty("onFailed");
    if (method == nullptr) {
        HILOG_ERROR("Failed to get onFailed from object");
        return;
    }
    NativeValue* argv[] = {engine_.CreateNumber(errorCode)};
    HILOG_INFO("CallJsFailed CallFunction success");
    engine_.CallFunction(value, method, argv, ARGC_ONE);
    HILOG_INFO("CallJsFailed end");
}
}  // namespace AbilityRuntime
}  // namespace OHOS