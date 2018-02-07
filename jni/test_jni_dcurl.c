#include <jni.h>
#include "TestJNIPow.h"

void dcurl_init(int max_cpu_thread, int max_gpu_thread);

int *dcurl_entry(int *trits, int mwm);

JNIEXPORT void JNICALL Java_test_1jni_1dcurl_dcurl_1init(JNIEnv *env, jobject obj, jint max_cpu_thread, jint max_gpu_thread)
{
    dcurl_init(max_cpu_thread, max_gpu_thread);
}

JNIEXPORT jintArray JNICALL Java_test_1jni_1dcurl_dcurl_1entry(JNIEnv *env, jobject obj, jintArray trits, jint mwm)
{
    jint *c_trits = (*env)->GetIntArrayElements(env, trits, NULL);

    jint *result = dcurl_entry(c_trits, mwm);

    jintArray returnJNIArray = (*env)->NewIntArray(env, 8019);
    (*env)->SetIntArrayRegion(env, returnJNIArray, 0, 8019, result);

    return returnJNIArray;
}

JNIEXPORT void JNICALL Java_test_1jni_1dcurl_dcurl_1destroy(JNIEnv *env, jobject obj)
{
    /* Do Nothing */
}
