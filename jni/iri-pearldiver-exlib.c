#include "iri-pearldiver-exlib.h"
#include <jni.h>
#include "../src/dcurl.h"
#include "../src/trinary/trinary.h"
#include <stdint.h>

static int8_t *int_to_char_array(jint *arr, int size)
{
    int8_t *ret = malloc(size);
    if (!ret)
        return NULL;

    for (int i = 0; i < size; i++)
        ret[i] = arr[i];

    return ret;
}

static jint *char_to_int_array(int8_t *arr, int size)
{
    jint *ret = malloc(sizeof(jint) * size);
    if (!ret)
        return NULL;

    for (jint i = 0; i < size; i++)
        ret[i] = arr[i];

    return ret;
}

JNIEXPORT jboolean JNICALL
Java_com_iota_iri_hash_PearlDiver_exlib_1init(JNIEnv *env, jclass clazz)
{
    if (!dcurl_init(2, 1))
        return JNI_FALSE;
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_iota_iri_hash_PearlDiver_exlib_1search(JNIEnv *env,
                                                jclass clazz,
                                                jintArray trits,
                                                jint mwm)
{
    /* Convert ***INT*** array (TRITS) to ***CHAR*** array (TRYTES) */
    jint *c_trits = (*env)->GetIntArrayElements(env, trits, NULL);
    int8_t *char_trits = int_to_char_array(c_trits, 8019);
    if (!char_trits)
        return JNI_FALSE;

    Trits_t *arg_trits = initTrits(char_trits, 8019);
    Trytes_t *arg_trytes = trytes_from_trits(arg_trits);
    if (!arg_trytes)
        return JNI_FALSE;
    /****************************************************************/

    int8_t *result = dcurl_entry(arg_trytes->data, mwm);

    /* Convert ***CHAR*** array(TRYTES) to ***INT*** array (TRITS) */
    Trytes_t *ret_trytes = initTrytes(result, 2673);
    Trits_t *ret_trits = trits_from_trytes(ret_trytes);
    if (!ret_trits)
        return JNI_FALSE;

    jint *int_trits = char_to_int_array(ret_trits->data, 8019);
    if (!int_trits)
        return JNI_FALSE;
    /***************************************************************/

    (*env)->SetIntArrayRegion(env, trits, 0, 8019, int_trits);

    /* Free */
    free(char_trits);
    free(int_trits);
    free(result);
    freeTrobject(ret_trytes);
    freeTrobject(arg_trits);
    freeTrobject(arg_trytes);
    freeTrobject(ret_trits);

    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_iota_iri_hash_PearlDiver_exlib_1cancel(JNIEnv *env, jclass clazz)
{
    /* Do nothing */
}

JNIEXPORT void JNICALL
Java_com_iota_iri_hash_PearlDiver_exlib_1destroy(JNIEnv *env, jclass clazz)
{
    dcurl_destroy();
}
