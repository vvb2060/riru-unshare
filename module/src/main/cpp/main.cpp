#include <jni.h>
#include <sys/types.h>
#include <riru.h>
#include <malloc.h>

#include "logging.h"
#include "nativehelper/scoped_utf_chars.h"
#include "android_filesystem_config.h"

static int *allow_unload = nullptr;

static int shouldSkipUid(int uid) {
    int appid = uid % AID_USER_OFFSET;
    if (appid >= AID_APP_START && appid <= AID_APP_END) return false;
    return !(appid >= AID_ISOLATED_START && appid <= AID_ISOLATED_END);
}

static void doUnshare(JNIEnv *env, jint *uid, jint *mountExternal, jstring *niceName) {
    if (shouldSkipUid(*uid)) return;
    if (*mountExternal == 0) {
        *mountExternal = 1;
        ScopedUtfChars name(env, *niceName);
        LOGI("unshare uid=%d name=%s", *uid, name.c_str());
    }
    if (allow_unload) *allow_unload = 1;
}

static void forkAndSpecializePre(
        JNIEnv *env, jclass clazz, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jintArray *fdsToClose, jintArray *fdsToIgnore, jboolean *is_child_zygote,
        jstring *instructionSet, jstring *appDataDir, jboolean *isTopApp,
        jobjectArray *pkgDataInfoList,
        jobjectArray *whitelistedDataInfoList, jboolean *bindMountAppDataDirs,
        jboolean *bindMountAppStorageDirs) {
    doUnshare(env, uid, mountExternal, niceName);
}


static void specializeAppProcessPre(
        JNIEnv *env, jclass clazz, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
        jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
        jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs) {
    doUnshare(env, uid, mountExternal, niceName);
}

extern "C" {
static RiruVersionedModuleInfo module = {
        .moduleApiVersion = RIRU_MODULE_API_VERSION,
        .moduleInfo = {
                .supportHide = true,
                .version = RIRU_MODULE_VERSION,
                .versionName = RIRU_MODULE_VERSION_NAME,
                .onModuleLoaded = nullptr,
                .forkAndSpecializePre = forkAndSpecializePre,
                .forkAndSpecializePost = nullptr,
                .forkSystemServerPre = nullptr,
                .forkSystemServerPost = nullptr,
                .specializeAppProcessPre = specializeAppProcessPre,
                .specializeAppProcessPost = nullptr,
        }
};

RIRU_EXPORT RiruVersionedModuleInfo *init(Riru *riru) {
    if (riru->riruApiVersion < RIRU_MODULE_API_VERSION) return nullptr;
    allow_unload = riru->allowUnload;
    return &module;
}

}
