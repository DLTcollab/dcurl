#include <jni.h>
#include "iri-pearldiver-exlib.h"
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

JNIEXPORT jboolean JNICALL Java_com_iota_iri_hash_PearlDiver_exlib_1init(JNIEnv *env, jclass clazz)
{
    dcurl_init(2, 1);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_iota_iri_hash_PearlDiver_exlib_1search(JNIEnv *env, jclass clazz, jintArray trits, jint mwm)
{
    /* Convert ***INT*** array (TRITS) to ***CHAR*** array (TRYTES) */                                                         
    jint *c_trits = (*env)->GetIntArrayElements(env, trits, NULL);
    char *char_trits = int_to_char_array(c_trits, 8019);
    
    Trits_t *arg_trits = initTrits(char_trits, 8019);
    Trytes_t *arg_trytes = trytes_from_trits(arg_trits);
    /****************************************************************/
    
    Trytes_t *result = dcurl_entry(arg_trytes, mwm);
    
    /* Convert ***CHAR*** array(TRYTES) to ***INT*** array (TRITS) */
    Trits_t *ret_trits = trits_from_trytes(result);
    jint *int_trits = char_to_int_array(ret_trits->data, 8019);
    /***************************************************************/
   
    /*
    jintArray returnJNIArray = (*env)->NewIntArray(env, 8019);
    (*env)->SetIntArrayRegion(env, returnJNIArray, 0, 8019, int_trits);
    */

    (*env)->SetIntArrayRegion(env, trits, 0, 8019, int_trits);

    /* Free */
    free(char_trits);
    free(int_trits);
    freeTrobject(result);
    freeTrobject(arg_trits);
    freeTrobject(arg_trytes);
    freeTrobject(ret_trits);
    
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_iota_iri_hash_PearlDiver_exlib_1cancel(JNIEnv *env, jclass clazz)
{
    /* Do nothing */
}

JNIEXPORT void JNICALL Java_com_iota_iri_hash_PearlDiver_exlib_1destroy(JNIEnv *env, jclass clazz)
{
    dcurl_destroy();
}
