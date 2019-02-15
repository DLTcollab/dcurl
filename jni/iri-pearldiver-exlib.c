#include "iri-pearldiver-exlib.h"
#include <jni.h>
#include <stdint.h>
#include "../src/dcurl.h"
#include "../src/trinary.h"

JNIEXPORT jboolean JNICALL
Java_com_iota_iri_hash_PearlDiver_exlib_1init(JNIEnv *env, jclass clazz)
{
    if (!dcurl_init())
        return JNI_FALSE;
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_iota_iri_hash_PearlDiver_exlib_1search(JNIEnv *env,
                                                jclass clazz,
                                                jbyteArray trits,
                                                jint mwm,
                                                jint threads)
{
    /*********** Get the Byte array from Java byte Array *************/
    jbyte *c_trits = (*env)->GetByteArrayElements(env, trits, NULL);

    Trits_t *arg_trits = initTrits((int8_t *) c_trits, 8019);
    Trytes_t *arg_trytes = trytes_from_trits(arg_trits);
    if (!arg_trytes)
        return JNI_FALSE;
    /****************************************************************/

    int8_t *result = dcurl_entry(arg_trytes->data, mwm, threads);

    /************ Write result back Java byte array *****************/
    Trytes_t *ret_trytes = initTrytes(result, 2673);
    Trits_t *ret_trits = trits_from_trytes(ret_trytes);
    if (!ret_trits)
        return JNI_FALSE;
    (*env)->SetByteArrayRegion(env, trits, 0, 8019, ret_trits->data);
    /****************************************************************/

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
