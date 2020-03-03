/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "iri_pearldiver_exlib.h"
#include <jni.h>
#include <stdint.h>
#include "../src/dcurl.h"
#include "../src/trinary.h"

JNIEXPORT jboolean JNICALL
Java_com_iota_iri_crypto_PearlDiver_exlibInit(JNIEnv *env,
                                              jclass clazz,
                                              jbyteArray broker_host,
                                              jint broker_host_len)
{
    jboolean ret = JNI_TRUE;

    /* Get the Byte array from Java byte Array */
    jbyte *host = (*env)->GetByteArrayElements(env, broker_host, NULL);

    dcurl_config config;
    config.broker_host = malloc(broker_host_len + 1);
    if (!config.broker_host) {
        ret = JNI_FALSE;
        goto fail;
    }
    memcpy(config.broker_host, (char *) host, broker_host_len);
    config.broker_host[broker_host_len] = '\0';

    if (!dcurl_init(&config))
        ret = JNI_FALSE;

    free(config.broker_host);

fail:
    return ret;
}

JNIEXPORT jboolean JNICALL
Java_com_iota_iri_crypto_PearlDiver_exlibSearch(JNIEnv *env,
                                                jclass clazz,
                                                jbyteArray trits,
                                                jint mwm,
                                                jint threads)
{
    jboolean ret = JNI_TRUE;

    /* Get the Byte array from Java byte Array */
    jbyte *c_trits = (*env)->GetByteArrayElements(env, trits, NULL);

    trits_t *arg_trits = init_trits((int8_t *) c_trits, 8019);
    trytes_t *arg_trytes = trytes_from_trits(arg_trits);
    if (!arg_trytes) {
        ret = JNI_FALSE;
        goto fail_input;
    }

    int8_t *result = dcurl_entry(arg_trytes->data, mwm, threads);

    /* Write result back Java byte array */
    trytes_t *ret_trytes = init_trytes(result, 2673);
    trits_t *ret_trits = trits_from_trytes(ret_trytes);
    if (!ret_trits) {
        ret = JNI_FALSE;
        goto fail_output;
    }
    (*env)->SetByteArrayRegion(env, trits, 0, 8019, ret_trits->data);

fail_output:
    free(result);
    free_trinary_object(ret_trits);
    free_trinary_object(ret_trytes);
fail_input:
    free_trinary_object(arg_trits);
    free_trinary_object(arg_trytes);

    return ret;
}

JNIEXPORT void JNICALL
Java_com_iota_iri_crypto_PearlDiver_exlibCancel(JNIEnv *env, jclass clazz)
{
    /* Do nothing */
}

JNIEXPORT void JNICALL
Java_com_iota_iri_crypto_PearlDiver_exlibDestroy(JNIEnv *env, jclass clazz)
{
    dcurl_destroy();
}
