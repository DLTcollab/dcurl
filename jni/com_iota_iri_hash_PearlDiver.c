#include <jni.h>
#include "com_iota_iri_hash_PearlDiver.h"
#include "../src/dcurl.h"
#include "../src/trinary/trinary.h"

static char *int_to_char_array(jint *arr, int size)
{
    char *ret = malloc(size);

    for (int i = 0; i < size; i++)
        ret[i] = arr[i];

    return ret;
}

static jint *char_to_int_array(char *arr, int size)
{
    jint *ret = malloc(sizeof(jint) * size);

    for (jint i = 0; i < size; i++)
        ret[i] = arr[i];

    return ret;
}

JNIEXPORT void JNICALL Java_com_iota_iri_hash_PearlDiver_dcurl_1init(JNIEnv *env, jobject obj, jint max_cpu_thread, jint max_gpu_thread)
{
    dcurl_init(max_cpu_thread, max_gpu_thread);
}

JNIEXPORT jintArray JNICALL Java_com_iota_iri_hash_PearlDiver_dcurl_1entry(JNIEnv *env, jobject obj, jintArray trits, jint mwm)
{
    /* Convert ***INT*** array (TRITS) to ***CHAR*** array (TRYTES) */                                                         
    jint *c_trits = (*env)->GetIntArrayElements(env, trits, NULL);
    char *char_trits = int_to_char_array(c_trits, 8019);
    
    Trits_t *arg_trits = initTrits(char_trits, 8019);
    Trytes_t *arg_trytes = trytes_from_trits(arg_trits);
    /****************************************************************/
    
    char *result = dcurl_entry(arg_trytes->data, mwm);
    
    /* Convert ***CHAR*** array(TRYTES) to ***INT*** array (TRITS) */
    Trytes_t *ret_trytes = initTrytes(result, 81);
    Trits_t *ret_trits = trits_from_trytes(ret_trytes);
    jint *int_trits = char_to_int_array(ret_trits->data, 243);
    /***************************************************************/
    
    jintArray returnJNIArray = (*env)->NewIntArray(env, 243);
    (*env)->SetIntArrayRegion(env, returnJNIArray, 0, 243, int_trits);

    /* Free */
    free(result);
    free(char_trits);
    free(int_trits);
    freeTrobject(arg_trits);
    freeTrobject(arg_trytes);
    freeTrobject(ret_trytes);
    freeTrobject(ret_trits);
    
    return returnJNIArray;
}

JNIEXPORT void JNICALL Java_com_iota_iri_hash_PearlDiver_dcurl_1destroy(JNIEnv *env, jobject obj)
{
    dcurl_destroy();
}
