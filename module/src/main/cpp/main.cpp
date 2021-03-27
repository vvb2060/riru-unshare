#include <jni.h>
#include <sys/types.h>
#include <riru.h>
#include <malloc.h>

#include "logging.h"
#include "nativehelper/scoped_utf_chars.h"
#include "android_filesystem_config.h"

static int shouldSkipUid(int uid) {
    int appid = uid % AID_USER_OFFSET;
    if (appid >= AID_APP_START && appid <= AID_APP_END) return false;
    if (appid >= AID_ISOLATED_START && appid <= AID_ISOLATED_END) return false;
    return true;
}

static void doUnshare(JNIEnv *env, jint *uid, jint *mountExternal, jstring *niceName) {
    if (shouldSkipUid(*uid)) return;
    if (*mountExternal == 0) {
        *mountExternal = 1;
        ScopedUtfChars name(env, *niceName);
        LOGI("unshare uid=%d name=%s", *uid, name.c_str());
    }
}

static void forkAndSpecializePre(
        JNIEnv *env, jclass clazz, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jintArray *fdsToClose, jintArray *fdsToIgnore, jboolean *is_child_zygote,
        jstring *instructionSet, jstring *appDataDir, jboolean *isTopApp, jobjectArray *pkgDataInfoList,
        jobjectArray *whitelistedDataInfoList, jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs) {
    doUnshare(env, uid, mountExternal, niceName);
}

static void forkAndSpecializePost(JNIEnv *env, jclass clazz, jint res) {
    if (res == 0) {
        riru_set_unload_allowed(true);
    }
}

static void specializeAppProcessPre(
        JNIEnv *env, jclass clazz, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
        jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
        jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs) {
    doUnshare(env, uid, mountExternal, niceName);
}

static void specializeAppProcessPost(JNIEnv *env, jclass clazz) {
    riru_set_unload_allowed(true);
}

extern "C" {

int riru_api_version;
const char *riru_magisk_module_path = nullptr;
int *riru_allow_unload = nullptr;

static auto module = RiruVersionedModuleInfo{
        .moduleApiVersion = RIRU_MODULE_API_VERSION,
        .moduleInfo= RiruModuleInfo{
                .supportHide = true,
                .version = RIRU_MODULE_VERSION,
                .versionName = RIRU_MODULE_VERSION_NAME,
                .onModuleLoaded = nullptr,
                .forkAndSpecializePre = forkAndSpecializePre,
                .forkAndSpecializePost = forkAndSpecializePost,
                .forkSystemServerPre = nullptr,
                .forkSystemServerPost = nullptr,
                .specializeAppProcessPre = specializeAppProcessPre,
                .specializeAppProcessPost = specializeAppProcessPost
        }
};

#ifndef RIRU_MODULE_LEGACY_INIT
RiruVersionedModuleInfo *init(Riru *riru) {
    auto core_max_api_version = riru->riruApiVersion;
    riru_api_version = core_max_api_version <= RIRU_MODULE_API_VERSION ? core_max_api_version : RIRU_MODULE_API_VERSION;
    module.moduleApiVersion = riru_api_version;

    riru_magisk_module_path = strdup(riru->magiskModulePath);
    if (riru_api_version >= 25) {
        riru_allow_unload = riru->allowUnload;
    }
    return &module;
}
#else
RiruVersionedModuleInfo *init(Riru *riru) {
    static int step = 0;
    step += 1;

    switch (step) {
        case 1: {
            auto core_max_api_version = riru->riruApiVersion;
            riru_api_version = core_max_api_version <= RIRU_MODULE_API_VERSION ? core_max_api_version : RIRU_MODULE_API_VERSION;
            if (riru_api_version < 25) {
                module.moduleInfo.unused = (void *) shouldSkipUid;
            } else {
                riru_allow_unload = riru->allowUnload;
            }
            if (riru_api_version >= 24) {
                module.moduleApiVersion = riru_api_version;
                riru_magisk_module_path = strdup(riru->magiskModulePath);
                return &module;
            } else {
                return (RiruVersionedModuleInfo *) &riru_api_version;
            }
        }
        case 2: {
            return (RiruVersionedModuleInfo *) &module.moduleInfo;
        }
        case 3:
        default: {
            return nullptr;
        }
    }
}
#endif
}
