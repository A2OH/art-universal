#include <jni.h>
#include <stdio.h>
#include <string.h>

static void OHB_logDebug(JNIEnv* e, jclass c, jstring t, jstring m) {
  const char*a=(*e)->GetStringUTFChars(e,t,0),*b=(*e)->GetStringUTFChars(e,m,0);
  printf("[D] %s: %s\n",a,b); (*e)->ReleaseStringUTFChars(e,m,b); (*e)->ReleaseStringUTFChars(e,t,a); }
static void OHB_logInfo(JNIEnv* e, jclass c, jstring t, jstring m) {
  const char*a=(*e)->GetStringUTFChars(e,t,0),*b=(*e)->GetStringUTFChars(e,m,0);
  printf("[I] %s: %s\n",a,b); (*e)->ReleaseStringUTFChars(e,m,b); (*e)->ReleaseStringUTFChars(e,t,a); }
static void OHB_logWarn(JNIEnv* e, jclass c, jstring t, jstring m) {
  const char*a=(*e)->GetStringUTFChars(e,t,0),*b=(*e)->GetStringUTFChars(e,m,0);
  printf("[W] %s: %s\n",a,b); (*e)->ReleaseStringUTFChars(e,m,b); (*e)->ReleaseStringUTFChars(e,t,a); }
static void OHB_logError(JNIEnv* e, jclass c, jstring t, jstring m) {
  const char*a=(*e)->GetStringUTFChars(e,t,0),*b=(*e)->GetStringUTFChars(e,m,0);
  printf("[E] %s: %s\n",a,b); (*e)->ReleaseStringUTFChars(e,m,b); (*e)->ReleaseStringUTFChars(e,t,a); }
static jfloat OHB_fontMeasureText(JNIEnv*e,jclass c,jlong f,jstring s){return s?(jfloat)((*e)->GetStringLength(e,s)*8):0;}
static jfloatArray OHB_fontGetMetrics(JNIEnv*e,jclass c,jlong f){jfloatArray a=(*e)->NewFloatArray(e,4);jfloat m[]={-12,3,0,15};(*e)->SetFloatArrayRegion(e,a,0,4,m);return a;}
static jint OHB_audioGetRingerMode(JNIEnv* env, jclass cls) { return 0; }
static jint OHB_audioGetStreamMaxVolume(JNIEnv* env, jclass cls, jint a0) { return 0; }
static jint OHB_audioGetStreamVolume(JNIEnv* env, jclass cls, jint a0) { return 0; }
static jboolean OHB_audioIsMusicActive(JNIEnv* env, jclass cls) { return JNI_FALSE; }
static void OHB_audioSetRingerMode(JNIEnv* env, jclass cls, jint a0) {  }
static void OHB_audioSetStreamVolume(JNIEnv* env, jclass cls, jint a0, jint a1, jint a2) {  }
static jint OHB_bitmapBlitToFb0(JNIEnv* env, jclass cls, jlong a0, jint a1) { return 0; }
static jlong OHB_bitmapCreate(JNIEnv* env, jclass cls, jint a0, jint a1, jint a2) { return 0; }
static void OHB_bitmapDestroy(JNIEnv* env, jclass cls, jlong a0) {  }
static jint OHB_bitmapGetHeight(JNIEnv* env, jclass cls, jlong a0) { return 0; }
static jint OHB_bitmapGetPixel(JNIEnv* env, jclass cls, jlong a0, jint a1, jint a2) { return 0; }
static jint OHB_bitmapGetWidth(JNIEnv* env, jclass cls, jlong a0) { return 0; }
static void OHB_bitmapSetPixel(JNIEnv* env, jclass cls, jlong a0, jint a1, jint a2, jint a3) {  }
static jint OHB_bitmapWriteToFile(JNIEnv* env, jclass cls, jlong a0, jstring a1) { return 0; }
static jlong OHB_brushCreate(JNIEnv* env, jclass cls) { return 0; }
static void OHB_brushDestroy(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_brushSetColor(JNIEnv* env, jclass cls, jlong a0, jint a1) {  }
static void OHB_canvasClipPath(JNIEnv* env, jclass cls, jlong a0, jlong a1) {  }
static void OHB_canvasClipRect(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4) {  }
static void OHB_canvasConcat(JNIEnv* env, jclass cls, jlong a0, jfloatArray a1) {  }
static jlong OHB_canvasCreate(JNIEnv* env, jclass cls, jlong a0) { return 0; }
static void OHB_canvasDestroy(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_canvasDrawArc(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jfloat a5, jfloat a6, jboolean a7, jlong a8, jlong a9) {  }
static void OHB_canvasDrawBitmap(JNIEnv* env, jclass cls, jlong a0, jlong a1, jfloat a2, jfloat a3) {  }
static void OHB_canvasDrawCircle(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jlong a4, jlong a5) {  }
static void OHB_canvasDrawColor(JNIEnv* env, jclass cls, jlong a0, jint a1) {  }
static void OHB_canvasDrawLine(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jlong a5) {  }
static void OHB_canvasDrawOval(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jlong a5, jlong a6) {  }
static void OHB_canvasDrawPath(JNIEnv* env, jclass cls, jlong a0, jlong a1, jlong a2, jlong a3) {  }
static void OHB_canvasDrawRect(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jlong a5, jlong a6) {  }
static void OHB_canvasDrawRoundRect(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jfloat a5, jfloat a6, jlong a7, jlong a8) {  }
static void OHB_canvasDrawText(JNIEnv* env, jclass cls, jlong a0, jstring a1, jfloat a2, jfloat a3, jlong a4, jlong a5, jlong a6) {  }
static void OHB_canvasRestore(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_canvasRotate(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3) {  }
static void OHB_canvasSave(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_canvasScale(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2) {  }
static void OHB_canvasTranslate(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2) {  }
static jint OHB_checkPermission(JNIEnv* env, jclass cls, jstring a0) { return 0; }
static jstring OHB_clipboardGet(JNIEnv* env, jclass cls) { return NULL; }
static void OHB_clipboardSet(JNIEnv* env, jclass cls, jstring a0) {  }
static void OHB_fontDestroy(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_fontSetSize(JNIEnv* env, jclass cls, jlong a0, jfloat a1) {  }
static jstring OHB_getDeviceBrand(JNIEnv* env, jclass cls) { return NULL; }
static jstring OHB_getDeviceModel(JNIEnv* env, jclass cls) { return NULL; }
static jint OHB_getNetworkType(JNIEnv* env, jclass cls) { return 0; }
static jstring OHB_getOSVersion(JNIEnv* env, jclass cls) { return NULL; }
static jstring OHB_httpRequest(JNIEnv* env, jclass cls, jstring a0, jstring a1, jstring a2, jstring a3) { return NULL; }
static jdoubleArray OHB_locationGetLast(JNIEnv* env, jclass cls) { return NULL; }
static jboolean OHB_locationIsEnabled(JNIEnv* env, jclass cls) { return JNI_FALSE; }
static jlong OHB_mediaPlayerCreate(JNIEnv* env, jclass cls) { return 0; }
static jint OHB_mediaPlayerGetCurrentPosition(JNIEnv* env, jclass cls, jlong a0) { return 0; }
static jint OHB_mediaPlayerGetDuration(JNIEnv* env, jclass cls, jlong a0) { return 0; }
static jboolean OHB_mediaPlayerIsPlaying(JNIEnv* env, jclass cls, jlong a0) { return JNI_FALSE; }
static void OHB_mediaPlayerPause(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_mediaPlayerPrepare(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_mediaPlayerRelease(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_mediaPlayerReset(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_mediaPlayerSeekTo(JNIEnv* env, jclass cls, jlong a0, jint a1) {  }
static void OHB_mediaPlayerSetDataSource(JNIEnv* env, jclass cls, jlong a0, jstring a1) {  }
static void OHB_mediaPlayerSetLooping(JNIEnv* env, jclass cls, jlong a0, jboolean a1) {  }
static void OHB_mediaPlayerSetVolume(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2) {  }
static void OHB_mediaPlayerStart(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_mediaPlayerStop(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_nodeAddChild(JNIEnv* env, jclass cls, jlong a0, jlong a1) {  }
static jlong OHB_nodeCreate(JNIEnv* env, jclass cls, jint a0) { return 0; }
static void OHB_nodeDispose(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_nodeInsertChildAt(JNIEnv* env, jclass cls, jlong a0, jlong a1, jint a2) {  }
static void OHB_nodeMarkDirty(JNIEnv* env, jclass cls, jlong a0, jint a1) {  }
static jint OHB_nodeRegisterEvent(JNIEnv* env, jclass cls, jlong a0, jint a1, jint a2) { return 0; }
static void OHB_nodeRemoveChild(JNIEnv* env, jclass cls, jlong a0, jlong a1) {  }
static jint OHB_nodeSetAttrColor(JNIEnv* env, jclass cls, jlong a0, jint a1, jint a2) { return 0; }
static jint OHB_nodeSetAttrFloat(JNIEnv* env, jclass cls, jlong a0, jint a1, jfloat a2, jfloat a3, jfloat a4, jfloat a5, jint a6) { return 0; }
static jint OHB_nodeSetAttrInt(JNIEnv* env, jclass cls, jlong a0, jint a1, jint a2) { return 0; }
static jint OHB_nodeSetAttrString(JNIEnv* env, jclass cls, jlong a0, jint a1, jstring a2) { return 0; }
static void OHB_nodeUnregisterEvent(JNIEnv* env, jclass cls, jlong a0, jint a1) {  }
static void OHB_notificationAddSlot(JNIEnv* env, jclass cls, jstring a0, jstring a1, jint a2) {  }
static void OHB_notificationCancel(JNIEnv* env, jclass cls, jint a0) {  }
static void OHB_notificationPublish(JNIEnv* env, jclass cls, jint a0, jstring a1, jstring a2, jstring a3, jint a4) {  }
static void OHB_pathAddCircle(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jint a4) {  }
static void OHB_pathAddRect(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jint a5) {  }
static void OHB_pathClose(JNIEnv* env, jclass cls, jlong a0) {  }
static jlong OHB_pathCreate(JNIEnv* env, jclass cls) { return 0; }
static void OHB_pathCubicTo(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jfloat a5, jfloat a6) {  }
static void OHB_pathDestroy(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_pathLineTo(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2) {  }
static void OHB_pathMoveTo(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2) {  }
static void OHB_pathQuadTo(JNIEnv* env, jclass cls, jlong a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4) {  }
static void OHB_pathReset(JNIEnv* env, jclass cls, jlong a0) {  }
static jlong OHB_penCreate(JNIEnv* env, jclass cls) { return 0; }
static void OHB_penDestroy(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_penSetAntiAlias(JNIEnv* env, jclass cls, jlong a0, jboolean a1) {  }
static void OHB_penSetCap(JNIEnv* env, jclass cls, jlong a0, jint a1) {  }
static void OHB_penSetColor(JNIEnv* env, jclass cls, jlong a0, jint a1) {  }
static void OHB_penSetJoin(JNIEnv* env, jclass cls, jlong a0, jint a1) {  }
static void OHB_penSetWidth(JNIEnv* env, jclass cls, jlong a0, jfloat a1) {  }
static void OHB_preferencesClear(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_preferencesClose(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_preferencesFlush(JNIEnv* env, jclass cls, jlong a0) {  }
static jboolean OHB_preferencesGetBoolean(JNIEnv* env, jclass cls, jlong a0, jstring a1, jboolean a2) { return JNI_FALSE; }
static jfloat OHB_preferencesGetFloat(JNIEnv* env, jclass cls, jlong a0, jstring a1, jfloat a2) { return 0.0; }
static jint OHB_preferencesGetInt(JNIEnv* env, jclass cls, jlong a0, jstring a1, jint a2) { return 0; }
static jlong OHB_preferencesGetLong(JNIEnv* env, jclass cls, jlong a0, jstring a1, jlong a2) { return 0; }
static jstring OHB_preferencesGetString(JNIEnv* env, jclass cls, jlong a0, jstring a1, jstring a2) { return NULL; }
static jlong OHB_preferencesOpen(JNIEnv* env, jclass cls, jstring a0) { return 0; }
static void OHB_preferencesPutBoolean(JNIEnv* env, jclass cls, jlong a0, jstring a1, jboolean a2) {  }
static void OHB_preferencesPutFloat(JNIEnv* env, jclass cls, jlong a0, jstring a1, jfloat a2) {  }
static void OHB_preferencesPutInt(JNIEnv* env, jclass cls, jlong a0, jstring a1, jint a2) {  }
static void OHB_preferencesPutLong(JNIEnv* env, jclass cls, jlong a0, jstring a1, jlong a2) {  }
static void OHB_preferencesPutString(JNIEnv* env, jclass cls, jlong a0, jstring a1, jstring a2) {  }
static void OHB_preferencesRemove(JNIEnv* env, jclass cls, jlong a0, jstring a1) {  }
static void OHB_rdbStoreBeginTransaction(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_rdbStoreClose(JNIEnv* env, jclass cls, jlong a0) {  }
static void OHB_rdbStoreCommit(JNIEnv* env, jclass cls, jlong a0) {  }
static jint OHB_rdbStoreDelete(JNIEnv* env, jclass cls, jlong a0, jstring a1, jstring a2, jobjectArray a3) { return 0; }
static void OHB_rdbStoreExecSQL(JNIEnv* env, jclass cls, jlong a0, jstring a1) {  }
static jlong OHB_rdbStoreInsert(JNIEnv* env, jclass cls, jlong a0, jstring a1, jstring a2) { return 0; }
static jlong OHB_rdbStoreOpen(JNIEnv* env, jclass cls, jstring a0, jint a1) { return 0; }
static jlong OHB_rdbStoreQuery(JNIEnv* env, jclass cls, jlong a0, jstring a1, jobjectArray a2) { return 0; }
static void OHB_rdbStoreRollback(JNIEnv* env, jclass cls, jlong a0) {  }
static jint OHB_rdbStoreUpdate(JNIEnv* env, jclass cls, jlong a0, jstring a1, jstring a2, jstring a3, jobjectArray a4) { return 0; }
static void OHB_reminderCancel(JNIEnv* env, jclass cls, jint a0) {  }
static jint OHB_reminderScheduleTimer(JNIEnv* env, jclass cls, jint a0, jstring a1, jstring a2, jstring a3, jstring a4) { return 0; }
static void OHB_resultSetClose(JNIEnv* env, jclass cls, jlong a0) {  }
static jbyteArray OHB_resultSetGetBlob(JNIEnv* env, jclass cls, jlong a0, jint a1) { return NULL; }
static jint OHB_resultSetGetColumnCount(JNIEnv* env, jclass cls, jlong a0) { return 0; }
static jint OHB_resultSetGetColumnIndex(JNIEnv* env, jclass cls, jlong a0, jstring a1) { return 0; }
static jstring OHB_resultSetGetColumnName(JNIEnv* env, jclass cls, jlong a0, jint a1) { return NULL; }
static jdouble OHB_resultSetGetDouble(JNIEnv* env, jclass cls, jlong a0, jint a1) { return 0.0; }
static jfloat OHB_resultSetGetFloat(JNIEnv* env, jclass cls, jlong a0, jint a1) { return 0.0; }
static jint OHB_resultSetGetInt(JNIEnv* env, jclass cls, jlong a0, jint a1) { return 0; }
static jlong OHB_resultSetGetLong(JNIEnv* env, jclass cls, jlong a0, jint a1) { return 0; }
static jint OHB_resultSetGetRowCount(JNIEnv* env, jclass cls, jlong a0) { return 0; }
static jstring OHB_resultSetGetString(JNIEnv* env, jclass cls, jlong a0, jint a1) { return NULL; }
static jboolean OHB_resultSetGoToFirstRow(JNIEnv* env, jclass cls, jlong a0) { return JNI_FALSE; }
static jboolean OHB_resultSetGoToNextRow(JNIEnv* env, jclass cls, jlong a0) { return JNI_FALSE; }
static jboolean OHB_resultSetIsNull(JNIEnv* env, jclass cls, jlong a0, jint a1) { return JNI_FALSE; }
static jfloatArray OHB_sensorGetData(JNIEnv* env, jclass cls, jint a0) { return NULL; }
static jboolean OHB_sensorIsAvailable(JNIEnv* env, jclass cls, jint a0) { return JNI_FALSE; }
static void OHB_showToast(JNIEnv* env, jclass cls, jstring a0, jint a1) {  }
static void OHB_startAbility(JNIEnv* env, jclass cls, jstring a0, jstring a1, jstring a2) {  }
static jlong OHB_surfaceCreate(JNIEnv* env, jclass cls, jlong a0, jint a1, jint a2) { return 0; }
static void OHB_surfaceDestroy(JNIEnv* env, jclass cls, jlong a0) {  }
static jint OHB_surfaceFlush(JNIEnv* env, jclass cls, jlong a0) { return 0; }
static jlong OHB_surfaceGetCanvas(JNIEnv* env, jclass cls, jlong a0) { return 0; }
static void OHB_surfaceResize(JNIEnv* env, jclass cls, jlong a0, jint a1, jint a2) {  }
static jstring OHB_telephonyGetDeviceId(JNIEnv* env, jclass cls) { return NULL; }
static jstring OHB_telephonyGetLine1Number(JNIEnv* env, jclass cls) { return NULL; }
static jstring OHB_telephonyGetNetworkOperatorName(JNIEnv* env, jclass cls) { return NULL; }
static jint OHB_telephonyGetNetworkType(JNIEnv* env, jclass cls) { return 0; }
static jint OHB_telephonyGetPhoneType(JNIEnv* env, jclass cls) { return 0; }
static jint OHB_telephonyGetSimState(JNIEnv* env, jclass cls) { return 0; }
static void OHB_terminateSelf(JNIEnv* env, jclass cls) {  }
static void OHB_vibratorCancel(JNIEnv* env, jclass cls) {  }
static jboolean OHB_vibratorHasVibrator(JNIEnv* env, jclass cls) { return JNI_FALSE; }
static void OHB_vibratorVibrate(JNIEnv* env, jclass cls, jlong a0) {  }
static jint OHB_wifiGetFrequency(JNIEnv* env, jclass cls) { return 0; }
static jint OHB_wifiGetLinkSpeed(JNIEnv* env, jclass cls) { return 0; }
static jint OHB_wifiGetRssi(JNIEnv* env, jclass cls) { return 0; }
static jstring OHB_wifiGetSSID(JNIEnv* env, jclass cls) { return NULL; }
static jint OHB_wifiGetState(JNIEnv* env, jclass cls) { return 0; }
static jboolean OHB_wifiIsEnabled(JNIEnv* env, jclass cls) { return JNI_FALSE; }
static jboolean OHB_wifiSetEnabled(JNIEnv* env, jclass cls, jboolean a0) { return JNI_FALSE; }

static jint OHB_arkuiInit(JNIEnv*e,jclass c){return 0;}
static jint OHB_arkuiInitAsync(JNIEnv*e,jclass c){return 0;}
JNIEXPORT jint JNICALL JNI_OnLoad_ohbridge(JavaVM*vm,void*r){
  fprintf(stderr, "[OHBridge] JNI_OnLoad_ohbridge CALLED\n");
  JNIEnv*e; if((*vm)->GetEnv(vm,(void**)&e,JNI_VERSION_1_6)!=JNI_OK)return-1;
  jclass c=(*e)->FindClass(e,"com/ohos/shim/bridge/OHBridge");
  if(!c){fprintf(stderr, "[OHBridge ARM64] class not found\n");return JNI_VERSION_1_6;}
  JNINativeMethod m[]={
    {"arkuiInit","()I",(void*)OHB_arkuiInit},
    {"audioGetRingerMode","()I",(void*)OHB_audioGetRingerMode},
    {"audioGetStreamMaxVolume","(I)I",(void*)OHB_audioGetStreamMaxVolume},
    {"audioGetStreamVolume","(I)I",(void*)OHB_audioGetStreamVolume},
    {"audioIsMusicActive","()Z",(void*)OHB_audioIsMusicActive},
    {"audioSetRingerMode","(I)V",(void*)OHB_audioSetRingerMode},
    {"audioSetStreamVolume","(III)V",(void*)OHB_audioSetStreamVolume},
    {"bitmapBlitToFb0","(JI)I",(void*)OHB_bitmapBlitToFb0},
    {"bitmapCreate","(III)J",(void*)OHB_bitmapCreate},
    {"bitmapDestroy","(J)V",(void*)OHB_bitmapDestroy},
    {"bitmapGetHeight","(J)I",(void*)OHB_bitmapGetHeight},
    {"bitmapGetPixel","(JII)I",(void*)OHB_bitmapGetPixel},
    {"bitmapGetWidth","(J)I",(void*)OHB_bitmapGetWidth},
    {"bitmapSetPixel","(JIII)V",(void*)OHB_bitmapSetPixel},
    {"bitmapWriteToFile","(JLjava/lang/String;)I",(void*)OHB_bitmapWriteToFile},
    {"brushCreate","()J",(void*)OHB_brushCreate},
    {"brushDestroy","(J)V",(void*)OHB_brushDestroy},
    {"brushSetColor","(JI)V",(void*)OHB_brushSetColor},
    {"canvasClipPath","(JJ)V",(void*)OHB_canvasClipPath},
    {"canvasClipRect","(JFFFF)V",(void*)OHB_canvasClipRect},
    {"canvasConcat","(J[F)V",(void*)OHB_canvasConcat},
    {"canvasCreate","(J)J",(void*)OHB_canvasCreate},
    {"canvasDestroy","(J)V",(void*)OHB_canvasDestroy},
    {"canvasDrawArc","(JFFFFFFZJJ)V",(void*)OHB_canvasDrawArc},
    {"canvasDrawBitmap","(JJFF)V",(void*)OHB_canvasDrawBitmap},
    {"canvasDrawCircle","(JFFFJJ)V",(void*)OHB_canvasDrawCircle},
    {"canvasDrawColor","(JI)V",(void*)OHB_canvasDrawColor},
    {"canvasDrawLine","(JFFFFJ)V",(void*)OHB_canvasDrawLine},
    {"canvasDrawOval","(JFFFFJJ)V",(void*)OHB_canvasDrawOval},
    {"canvasDrawPath","(JJJJ)V",(void*)OHB_canvasDrawPath},
    {"canvasDrawRect","(JFFFFJJ)V",(void*)OHB_canvasDrawRect},
    {"canvasDrawRoundRect","(JFFFFFFJJ)V",(void*)OHB_canvasDrawRoundRect},
    {"canvasDrawText","(JLjava/lang/String;FFJJJ)V",(void*)OHB_canvasDrawText},
    {"canvasRestore","(J)V",(void*)OHB_canvasRestore},
    {"canvasRotate","(JFFF)V",(void*)OHB_canvasRotate},
    {"canvasSave","(J)V",(void*)OHB_canvasSave},
    {"canvasScale","(JFF)V",(void*)OHB_canvasScale},
    {"canvasTranslate","(JFF)V",(void*)OHB_canvasTranslate},
    {"checkPermission","(Ljava/lang/String;)I",(void*)OHB_checkPermission},
    {"clipboardGet","()Ljava/lang/String;",(void*)OHB_clipboardGet},
    {"clipboardSet","(Ljava/lang/String;)V",(void*)OHB_clipboardSet},
    {"fontDestroy","(J)V",(void*)OHB_fontDestroy},
    {"fontGetMetrics","(J)[F",(void*)OHB_fontGetMetrics},
    {"fontMeasureText","(JLjava/lang/String;)F",(void*)OHB_fontMeasureText},
    {"fontSetSize","(JF)V",(void*)OHB_fontSetSize},
    {"getDeviceBrand","()Ljava/lang/String;",(void*)OHB_getDeviceBrand},
    {"getDeviceModel","()Ljava/lang/String;",(void*)OHB_getDeviceModel},
    {"getNetworkType","()I",(void*)OHB_getNetworkType},
    {"getOSVersion","()Ljava/lang/String;",(void*)OHB_getOSVersion},
    {"httpRequest","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",(void*)OHB_httpRequest},
    {"locationGetLast","()[D",(void*)OHB_locationGetLast},
    {"locationIsEnabled","()Z",(void*)OHB_locationIsEnabled},
    {"logDebug","(Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_logDebug},
    {"logError","(Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_logError},
    {"logInfo","(Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_logInfo},
    {"logWarn","(Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_logWarn},
    {"mediaPlayerCreate","()J",(void*)OHB_mediaPlayerCreate},
    {"mediaPlayerGetCurrentPosition","(J)I",(void*)OHB_mediaPlayerGetCurrentPosition},
    {"mediaPlayerGetDuration","(J)I",(void*)OHB_mediaPlayerGetDuration},
    {"mediaPlayerIsPlaying","(J)Z",(void*)OHB_mediaPlayerIsPlaying},
    {"mediaPlayerPause","(J)V",(void*)OHB_mediaPlayerPause},
    {"mediaPlayerPrepare","(J)V",(void*)OHB_mediaPlayerPrepare},
    {"mediaPlayerRelease","(J)V",(void*)OHB_mediaPlayerRelease},
    {"mediaPlayerReset","(J)V",(void*)OHB_mediaPlayerReset},
    {"mediaPlayerSeekTo","(JI)V",(void*)OHB_mediaPlayerSeekTo},
    {"mediaPlayerSetDataSource","(JLjava/lang/String;)V",(void*)OHB_mediaPlayerSetDataSource},
    {"mediaPlayerSetLooping","(JZ)V",(void*)OHB_mediaPlayerSetLooping},
    {"mediaPlayerSetVolume","(JFF)V",(void*)OHB_mediaPlayerSetVolume},
    {"mediaPlayerStart","(J)V",(void*)OHB_mediaPlayerStart},
    {"mediaPlayerStop","(J)V",(void*)OHB_mediaPlayerStop},
    {"nodeAddChild","(JJ)V",(void*)OHB_nodeAddChild},
    {"nodeCreate","(I)J",(void*)OHB_nodeCreate},
    {"nodeDispose","(J)V",(void*)OHB_nodeDispose},
    {"nodeInsertChildAt","(JJI)V",(void*)OHB_nodeInsertChildAt},
    {"nodeMarkDirty","(JI)V",(void*)OHB_nodeMarkDirty},
    {"nodeRegisterEvent","(JII)I",(void*)OHB_nodeRegisterEvent},
    {"nodeRemoveChild","(JJ)V",(void*)OHB_nodeRemoveChild},
    {"nodeSetAttrColor","(JII)I",(void*)OHB_nodeSetAttrColor},
    {"nodeSetAttrFloat","(JIFFFFI)I",(void*)OHB_nodeSetAttrFloat},
    {"nodeSetAttrInt","(JII)I",(void*)OHB_nodeSetAttrInt},
    {"nodeSetAttrString","(JILjava/lang/String;)I",(void*)OHB_nodeSetAttrString},
    {"nodeUnregisterEvent","(JI)V",(void*)OHB_nodeUnregisterEvent},
    {"notificationAddSlot","(Ljava/lang/String;Ljava/lang/String;I)V",(void*)OHB_notificationAddSlot},
    {"notificationCancel","(I)V",(void*)OHB_notificationCancel},
    {"notificationPublish","(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V",(void*)OHB_notificationPublish},
    {"pathAddCircle","(JFFFI)V",(void*)OHB_pathAddCircle},
    {"pathAddRect","(JFFFFI)V",(void*)OHB_pathAddRect},
    {"pathClose","(J)V",(void*)OHB_pathClose},
    {"pathCreate","()J",(void*)OHB_pathCreate},
    {"pathCubicTo","(JFFFFFF)V",(void*)OHB_pathCubicTo},
    {"pathDestroy","(J)V",(void*)OHB_pathDestroy},
    {"pathLineTo","(JFF)V",(void*)OHB_pathLineTo},
    {"pathMoveTo","(JFF)V",(void*)OHB_pathMoveTo},
    {"pathQuadTo","(JFFFF)V",(void*)OHB_pathQuadTo},
    {"pathReset","(J)V",(void*)OHB_pathReset},
    {"penCreate","()J",(void*)OHB_penCreate},
    {"penDestroy","(J)V",(void*)OHB_penDestroy},
    {"penSetAntiAlias","(JZ)V",(void*)OHB_penSetAntiAlias},
    {"penSetCap","(JI)V",(void*)OHB_penSetCap},
    {"penSetColor","(JI)V",(void*)OHB_penSetColor},
    {"penSetJoin","(JI)V",(void*)OHB_penSetJoin},
    {"penSetWidth","(JF)V",(void*)OHB_penSetWidth},
    {"preferencesClear","(J)V",(void*)OHB_preferencesClear},
    {"preferencesClose","(J)V",(void*)OHB_preferencesClose},
    {"preferencesFlush","(J)V",(void*)OHB_preferencesFlush},
    {"preferencesGetBoolean","(JLjava/lang/String;Z)Z",(void*)OHB_preferencesGetBoolean},
    {"preferencesGetFloat","(JLjava/lang/String;F)F",(void*)OHB_preferencesGetFloat},
    {"preferencesGetInt","(JLjava/lang/String;I)I",(void*)OHB_preferencesGetInt},
    {"preferencesGetLong","(JLjava/lang/String;J)J",(void*)OHB_preferencesGetLong},
    {"preferencesGetString","(JLjava/lang/String;Ljava/lang/String;)Ljava/lang/String;",(void*)OHB_preferencesGetString},
    {"preferencesOpen","(Ljava/lang/String;)J",(void*)OHB_preferencesOpen},
    {"preferencesPutBoolean","(JLjava/lang/String;Z)V",(void*)OHB_preferencesPutBoolean},
    {"preferencesPutFloat","(JLjava/lang/String;F)V",(void*)OHB_preferencesPutFloat},
    {"preferencesPutInt","(JLjava/lang/String;I)V",(void*)OHB_preferencesPutInt},
    {"preferencesPutLong","(JLjava/lang/String;J)V",(void*)OHB_preferencesPutLong},
    {"preferencesPutString","(JLjava/lang/String;Ljava/lang/String;)V",(void*)OHB_preferencesPutString},
    {"preferencesRemove","(JLjava/lang/String;)V",(void*)OHB_preferencesRemove},
    {"rdbStoreBeginTransaction","(J)V",(void*)OHB_rdbStoreBeginTransaction},
    {"rdbStoreClose","(J)V",(void*)OHB_rdbStoreClose},
    {"rdbStoreCommit","(J)V",(void*)OHB_rdbStoreCommit},
    {"rdbStoreDelete","(JLjava/lang/String;Ljava/lang/String;[Ljava/lang/String;)I",(void*)OHB_rdbStoreDelete},
    {"rdbStoreExecSQL","(JLjava/lang/String;)V",(void*)OHB_rdbStoreExecSQL},
    {"rdbStoreInsert","(JLjava/lang/String;Ljava/lang/String;)J",(void*)OHB_rdbStoreInsert},
    {"rdbStoreOpen","(Ljava/lang/String;I)J",(void*)OHB_rdbStoreOpen},
    {"rdbStoreQuery","(JLjava/lang/String;[Ljava/lang/String;)J",(void*)OHB_rdbStoreQuery},
    {"rdbStoreRollback","(J)V",(void*)OHB_rdbStoreRollback},
    {"rdbStoreUpdate","(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)I",(void*)OHB_rdbStoreUpdate},
    {"reminderCancel","(I)V",(void*)OHB_reminderCancel},
    {"reminderScheduleTimer","(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I",(void*)OHB_reminderScheduleTimer},
    {"resultSetClose","(J)V",(void*)OHB_resultSetClose},
    {"resultSetGetBlob","(JI)[B",(void*)OHB_resultSetGetBlob},
    {"resultSetGetColumnCount","(J)I",(void*)OHB_resultSetGetColumnCount},
    {"resultSetGetColumnIndex","(JLjava/lang/String;)I",(void*)OHB_resultSetGetColumnIndex},
    {"resultSetGetColumnName","(JI)Ljava/lang/String;",(void*)OHB_resultSetGetColumnName},
    {"resultSetGetDouble","(JI)D",(void*)OHB_resultSetGetDouble},
    {"resultSetGetFloat","(JI)F",(void*)OHB_resultSetGetFloat},
    {"resultSetGetInt","(JI)I",(void*)OHB_resultSetGetInt},
    {"resultSetGetLong","(JI)J",(void*)OHB_resultSetGetLong},
    {"resultSetGetRowCount","(J)I",(void*)OHB_resultSetGetRowCount},
    {"resultSetGetString","(JI)Ljava/lang/String;",(void*)OHB_resultSetGetString},
    {"resultSetGoToFirstRow","(J)Z",(void*)OHB_resultSetGoToFirstRow},
    {"resultSetGoToNextRow","(J)Z",(void*)OHB_resultSetGoToNextRow},
    {"resultSetIsNull","(JI)Z",(void*)OHB_resultSetIsNull},
    {"sensorGetData","(I)[F",(void*)OHB_sensorGetData},
    {"sensorIsAvailable","(I)Z",(void*)OHB_sensorIsAvailable},
    {"showToast","(Ljava/lang/String;I)V",(void*)OHB_showToast},
    {"startAbility","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",(void*)OHB_startAbility},
    {"surfaceCreate","(JII)J",(void*)OHB_surfaceCreate},
    {"surfaceDestroy","(J)V",(void*)OHB_surfaceDestroy},
    {"surfaceFlush","(J)I",(void*)OHB_surfaceFlush},
    {"surfaceGetCanvas","(J)J",(void*)OHB_surfaceGetCanvas},
    {"surfaceResize","(JII)V",(void*)OHB_surfaceResize},
    {"telephonyGetDeviceId","()Ljava/lang/String;",(void*)OHB_telephonyGetDeviceId},
    {"telephonyGetLine1Number","()Ljava/lang/String;",(void*)OHB_telephonyGetLine1Number},
    {"telephonyGetNetworkOperatorName","()Ljava/lang/String;",(void*)OHB_telephonyGetNetworkOperatorName},
    {"telephonyGetNetworkType","()I",(void*)OHB_telephonyGetNetworkType},
    {"telephonyGetPhoneType","()I",(void*)OHB_telephonyGetPhoneType},
    {"telephonyGetSimState","()I",(void*)OHB_telephonyGetSimState},
    {"terminateSelf","()V",(void*)OHB_terminateSelf},
    {"vibratorCancel","()V",(void*)OHB_vibratorCancel},
    {"vibratorHasVibrator","()Z",(void*)OHB_vibratorHasVibrator},
    {"vibratorVibrate","(J)V",(void*)OHB_vibratorVibrate},
    {"wifiGetFrequency","()I",(void*)OHB_wifiGetFrequency},
    {"wifiGetLinkSpeed","()I",(void*)OHB_wifiGetLinkSpeed},
    {"wifiGetRssi","()I",(void*)OHB_wifiGetRssi},
    {"wifiGetSSID","()Ljava/lang/String;",(void*)OHB_wifiGetSSID},
    {"wifiGetState","()I",(void*)OHB_wifiGetState},
    {"wifiIsEnabled","()Z",(void*)OHB_wifiIsEnabled},
    {"wifiSetEnabled","(Z)Z",(void*)OHB_wifiSetEnabled},
  };
  int n=sizeof(m)/sizeof(m[0]);
  if((*e)->RegisterNatives(e,c,m,n)!=0){(*e)->ExceptionClear(e);
    for(int i=0;i<n;i++){if((*e)->RegisterNatives(e,c,&m[i],1)!=0)(*e)->ExceptionClear(e);}}
  fprintf(stderr, "[OHBridge ARM64] %d methods\n",n); return JNI_VERSION_1_6;}
