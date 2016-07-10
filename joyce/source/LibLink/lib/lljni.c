/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002  John Elliott <seasip.webmaster@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifdef HAVE_JNI_H
#include <jni.h>
#include "uk_co_demon_seasip_liblink_LibLink.h"
#include "ll.h"

/* Convert error code to an exception */
static int check_error(JNIEnv *env, int error)
{
        jthrowable excep;
        jmethodID mid;
        jclass clazz;
        jstring str;

        if (error == LL_E_OK) return 0;

        clazz = (*env)->FindClass(env, "uk/co/demon/seasip/liblink/LibLinkException")
;
        if (clazz == NULL) return 0;

        mid = (*env)->GetMethodID(env, clazz, "<init>", "(Ljava/lang/String;I)V"
);
        if (mid == NULL) return 0;

        str = (*env)->NewStringUTF(env, ll_strerror(error));
        if (str == NULL) return 0;

        excep = (*env)->NewObject(env, clazz, mid, str, (jint)error);
        (*env)->Throw(env, excep);
        return 1;
}


/* Mapping between integer IDs and links... */
typedef struct ll_data *LL_PDEV;

static LL_PDEV *mapping;
int maplen;

static int check_mapping(JNIEnv *env)
{
        int n;
        if (!mapping)
        {
                mapping = malloc(16 * sizeof(LL_PDEV));
                if (!mapping) 
                {
                        maplen = 0;
                        check_error(env, LL_E_NOMEM);
                        return -1;
                }
                maplen = 16;
                for (n = 0; n < maplen; n++) mapping[n] = NULL;
        }
        return 0;
}

static jint add_map(JNIEnv *env, LL_PDEV ptr)
{
        int n;
        LL_PDEV *newmap;

        if (check_mapping(env)) return 0;
        for (n = 1; n < maplen; n++) if (mapping[n] == NULL)
        {
                mapping[n] = ptr;
                return n;
        }
        /* All slots used... */
        newmap = malloc(maplen * 2 * sizeof(LL_PDEV));
        if (!newmap)
        {
                check_error(env, LL_E_NOMEM);
                return 0;
        }
        for (n = 1; n < maplen; n++) newmap[n] = mapping[n];
        free(mapping);
        mapping[n = maplen] = ptr;
        maplen *= 2;
        return n;
}

/* Remove an integer <--> LL_PDEV mapping. If it was the last one, free
 * all the memory used by the mapping */

static void del_map(JNIEnv *env, jint index)
{
        int n;

        if (mapping)
        {
                mapping[index] = NULL;
                for (n = 0; n < maplen; n++) 
                {
                        if (mapping[n]) return;
                }
                free(mapping);
                mapping = NULL;
                maplen = 0;
        }
}

static jobject jlink_new(JNIEnv *env, LL_PDEV ptr)
{
        jclass clazz;
        jmethodID mid;
        jint llid;

        llid = add_map(env, ptr);
        if (!llid) return NULL;

        clazz = (*env)->FindClass(env, "uk/co/demon/seasip/liblink/LibLink");
        if (clazz == NULL) return 0;
        mid = (*env)->GetMethodID(env, clazz, "<init>", "(I)V");
        if (mid == NULL) return 0;
        return (*env)->NewObject(env, clazz, mid, llid);
}

static void jlink_delete(JNIEnv *env, jobject self)
{
        jint llid;
        jclass clazz;
        jfieldID fid;

        clazz = (*env)->FindClass(env, "uk/co/demon/seasip/liblink/LibLink");
        if (!clazz) { check_error(env, LL_E_BADPTR); return; }

        fid = (*env)->GetFieldID(env, clazz, "id", "I");
        if (!fid) { check_error(env, LL_E_BADPTR); return; }
        llid = (*env)->GetIntField(env, self, fid);

        del_map(env, llid);
        (*env)->SetIntField(env, self, fid, 0);
}

static LL_PDEV data_from_java(JNIEnv *env, jobject obj)
{
        jfieldID fid;
        jclass clazz;
        jint llid;

        clazz = (*env)->FindClass(env, "uk/co/demon/seasip/liblink/LibLink");
        if (!clazz) { check_error(env, LL_E_BADPTR); return NULL; }

        fid = (*env)->GetFieldID(env, clazz, "id", "I");
        if (!fid) { check_error(env, LL_E_BADPTR); return NULL; }
        llid = (*env)->GetIntField(env, obj, fid);

        if ((!mapping) || (llid >= maplen))
        {
                check_error(env, LL_E_BADPTR);
                return NULL;
        } 
        return mapping[llid];
}





/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    open
 * Signature: (Ljava/lang/String;)Luk/co/demon/seasip/liblink/LibLink;
 */
JNIEXPORT jobject JNICALL Java_uk_co_demon_seasip_liblink_LibLink_open
  (JNIEnv *env, jclass clazz, jstring fname, jstring dev)
{
	struct ll_data *self;
	const char *utfname, *utfdev;
	int err;

	utfname = (*env)->GetStringUTFChars(env, fname, NULL);
	utfdev  = (*env)->GetStringUTFChars(env, dev, NULL);
	
	err = ll_open(&self, utfname, utfdev);

	(*env)->ReleaseStringUTFChars(env, fname, utfname);
	(*env)->ReleaseStringUTFChars(env, dev,   utfdev);

	if (check_error(env, err)) return NULL; 
	
	return jlink_new(env, self);
}

/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    recvWait
 * Signature: ()B
 */
JNIEXPORT jbyte JNICALL Java_uk_co_demon_seasip_liblink_LibLink_recvWait
  (JNIEnv *env, jobject jself)
{
	int err;
	unsigned char c;

	LL_PDEV self = data_from_java(env, jself);
	err = ll_recv_wait(self, &c);

	if (check_error(env, err)) return 0;
	return c;
}



/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    recvPoll
 * Signature: ()B
 */
JNIEXPORT jbyte JNICALL Java_uk_co_demon_seasip_liblink_LibLink_recvPoll
  (JNIEnv *env, jobject jself)
{
	int err;
	unsigned char c;

	LL_PDEV self = data_from_java(env, jself);
	err = ll_recv_poll(self, &c);

	if (check_error(env, err)) return 0;
	return c;
}

/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    write
 * Signature: (B)V
 */
JNIEXPORT void JNICALL Java_uk_co_demon_seasip_liblink_LibLink_send
  (JNIEnv *env, jobject jself, jbyte v)
{
	int err;

	LL_PDEV self = data_from_java(env, jself);
	err = ll_send(self, v);

	if (check_error(env, err)) return;
}

/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    readControlWait
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_uk_co_demon_seasip_liblink_LibLink_readControlWait
  (JNIEnv *env, jobject jself)
{
	int err;
	unsigned c;	

	LL_PDEV self = data_from_java(env, jself);
	err = ll_rctl_wait(self, &c);

	if (check_error(env, err)) return 0;
	return c;
}



/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    readControlPoll
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_uk_co_demon_seasip_liblink_LibLink_readControlPoll
  (JNIEnv *env, jobject jself)
{
	int err;
	unsigned c;

	LL_PDEV self = data_from_java(env, jself);
	err = ll_rctl_poll(self, &c);

	if (check_error(env, err)) return 0;
	return c;
}

/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    setControl 
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_uk_co_demon_seasip_liblink_LibLink_setControl
  (JNIEnv *env, jobject jself, jint v)
{
	int err;

	LL_PDEV self = data_from_java(env, jself);
	err = ll_sctl(self, v);

	if (check_error(env, err)) return;
}


/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_uk_co_demon_seasip_liblink_LibLink_close
  (JNIEnv *env, jobject jself)
{
	LL_PDEV self = data_from_java(env, jself);
	ll_close(&self);
	
	jlink_delete(env, jself);	
}


#include <assert.h> 
JNIEXPORT void JNICALL Java_uk_co_demon_seasip_liblink_LibLink_sendPacket
  (JNIEnv *env, jobject jself, jbyteArray arr)
{
	int err;
	int len;
	jbyte *b;
	LL_PDEV self = data_from_java(env, jself);

	b   = (*env)->GetByteArrayElements(env, arr, NULL);
	len = (*env)->GetArrayLength(env, arr); 

	err = ll_sendpkt(self, b, len);

	(*env)->ReleaseByteArrayElements(env, arr, b, JNI_ABORT);

	if (check_error(env, err)) return;
}

/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    receivePacket
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_uk_co_demon_seasip_liblink_LibLink_receivePacket
  (JNIEnv *env, jobject jself)
{
	int err;
	unsigned int len;
	unsigned char packet[256];
	jbyteArray array;
	LL_PDEV self = data_from_java(env, jself);
	
	err = ll_recvpkt(self, packet, &len);

	if (check_error(env, err)) return NULL;
	array = (*env)->NewByteArray(env, len);	
	(*env)->SetByteArrayRegion(env, array, 0, len, packet);
	return array;
}

/*
 * Class:     uk_co_demon_seasip_liblink_LibLink
 * Method:    getVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_uk_co_demon_seasip_liblink_LibLink_getVersion
  (JNIEnv *env, jclass clazz)
{
        str = (*env)->NewStringUTF(env, VERSION);
	return str;
}


#endif // def HAVE_JNI_H

