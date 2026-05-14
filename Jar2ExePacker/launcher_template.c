/*
 * Jar2Exe Launcher Stub - WORKING VERSION
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
  #include <windows.h>
  typedef HMODULE dynlib_t;
  static dynlib_t lib_open(const char *p) { return LoadLibraryA(p); }
  static void *lib_sym(dynlib_t h, const char *s) { return (void*)GetProcAddress(h, s); }
#else
  #include <dlfcn.h>
  typedef void* dynlib_t;
  static dynlib_t lib_open(const char *p) { return dlopen(p, RTLD_LAZY); }
  static void *lib_sym(dynlib_t h, const char *s) { return dlsym(h, s); }
#endif

#include "jni.h"

typedef uint8_t u8;
typedef uint32_t u32;

/* XOR decrypt function */
static u8* xor_decrypt(const u8 *key, size_t key_len, const u8 *data, size_t data_len, size_t *out_len) {
    u8 *plain = (u8 *)malloc(data_len);
    if (!plain) return NULL;
    
    for (size_t i = 0; i < data_len; i++) {
        plain[i] = data[i] ^ key[i % key_len];
    }
    
    *out_len = data_len;
    return plain;
}

/* Payload parser */
static u32 read_u32(const u8 **p) {
    u32 v = ((u32)(*p)[0] << 24) | ((u32)(*p)[1] << 16)
          | ((u32)(*p)[2] <<  8) |  (u32)(*p)[3];
    *p += 4;
    return v;
}

static uint16_t read_u16(const u8 **p) {
    uint16_t v = ((uint16_t)(*p)[0] << 8) | (uint16_t)(*p)[1];
    *p += 2;
    return v;
}

static char *read_str(const u8 **p, int len) {
    char *s = (char *)malloc(len + 1);
    memcpy(s, *p, len);
    s[len] = '\0';
    *p += len;
    return s;
}

/* JVM helpers */
typedef jint (JNICALL *JNI_CreateJavaVM_fn)(JavaVM **, void **, void *);

static const char *find_jvm_lib(void) {
    static char buf[512];
    const char *jh = getenv("JAVA_HOME");
    
#ifdef _WIN32
    /* First try with JAVA_HOME */
    if (jh) {
        /* Try jre/bin/server path first (most common) */
        snprintf(buf, sizeof(buf), "%s\\jre\\bin\\server\\jvm.dll", jh);
        if (fopen(buf, "r")) return buf;
        
        /* Try bin/server path */
        snprintf(buf, sizeof(buf), "%s\\bin\\server\\jvm.dll", jh);
        if (fopen(buf, "r")) return buf;
    }
    
    /* Try common installation paths */
    const char* common_paths[] = {
        "C:\\Program Files\\Java\\jdk1.8.0_162\\jre\\bin\\server\\jvm.dll",
        "C:\\Program Files\\Java\\jdk1.8.0_162\\bin\\server\\jvm.dll",
        "C:\\Program Files\\Java\\jre1.8.0_162\\bin\\server\\jvm.dll",
        "C:\\Program Files (x86)\\Java\\jdk1.8.0_162\\jre\\bin\\server\\jvm.dll",
        NULL
    };
    
    for (int i = 0; common_paths[i] != NULL; i++) {
        if (fopen(common_paths[i], "r")) {
            strcpy(buf, common_paths[i]);
            return buf;
        }
    }
    
    /* Last resort - assume it's in PATH */
    return "jvm.dll";
#else
    if (jh) { 
        snprintf(buf, sizeof(buf), "%s/jre/lib/server/libjvm.so", jh);
        if (fopen(buf, "r")) return buf;
        snprintf(buf, sizeof(buf), "%s/lib/server/libjvm.so", jh);
        return buf;
    }
    return "libjvm.so";
#endif
}

static char *make_tmp_path(void) {
    static char buf[256];
#ifdef _WIN32
    char tmp[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp);
    snprintf(buf, sizeof(buf), "%s\\app_%lu.jar", tmp, (unsigned long)GetCurrentProcessId());
#else
    snprintf(buf, sizeof(buf), "/tmp/app_%d.jar", getpid());
#endif
    return buf;
}

/* Embedded data - these will be replaced by the Java packer */
static const u8 XOR_KEY[] = { %%KEY%% };
static const size_t KEY_LEN = %%KEY_LEN%%;
static const u8 ENCRYPTED_DATA[] = { %%DATA%% };
static const size_t DATA_LEN = %%DATA_LEN%%;
static const char *MAIN_CLASS = "%%MAIN_CLASS%%";

int main(int argc, char **argv) {
    printf("[*] Launcher started\n");
    printf("[*] Key size: %zu bytes\n", KEY_LEN);
    printf("[*] Encrypted data size: %zu bytes\n", DATA_LEN);
    
    /* 1. Decrypt */
    size_t plain_len = 0;
    u8 *plain = xor_decrypt(XOR_KEY, KEY_LEN, ENCRYPTED_DATA, DATA_LEN, &plain_len);
    if (!plain) { 
        fprintf(stderr, "[!] Decryption failed\n"); 
        return 1; 
    }
    
    printf("[*] Decryption successful, plaintext size: %zu bytes\n", plain_len);
    
    /* 2. Parse payload */
    const u8 *p = plain;
    u32 num_classes = read_u32(&p);
    printf("[*] Number of obfuscated classes: %u\n", num_classes);
    
    if (num_classes > 10000) {
        fprintf(stderr, "[!] Invalid data - too many classes\n");
        free(plain);
        return 1;
    }
    
/* Read class mappings (skip if num_classes is 0) */
for (u32 i = 0; i < num_classes; i++) {
    int ol = read_u16(&p);
    char *obf_name = read_str(&p, ol);
    int rl = read_u16(&p);
    char *real_name = read_str(&p, rl);
    printf("[*] Map: %s -> %s\n", obf_name, real_name);
    free(obf_name);
    free(real_name);
}
    
    /* Get JAR data */
    u32 jar_size = read_u32(&p);
    printf("[*] Embedded JAR size: %u bytes\n", jar_size);
    
    if (jar_size == 0 || jar_size > 100 * 1024 * 1024) {
        fprintf(stderr, "[!] Invalid JAR size\n");
        free(plain);
        return 1;
    }
    
    /* 3. Write JAR to temp file */
    char *tmp_jar = make_tmp_path();
    printf("[*] Creating temp JAR: %s\n", tmp_jar);
    
    FILE *f = fopen(tmp_jar, "wb");
    if (!f) {
        fprintf(stderr, "[!] Cannot create temp file: %s (error: %d)\n", tmp_jar, errno);
        free(plain);
        return 1;
    }
    
    size_t written = fwrite(p, 1, jar_size, f);
    fclose(f);
    
    if (written != jar_size) {
        fprintf(stderr, "[!] Only wrote %zu of %u bytes\n", written, jar_size);
        remove(tmp_jar);
        free(plain);
        return 1;
    }
    
    printf("[*] Temp JAR created successfully (%zu bytes)\n", written);
    
    /* Verify the JAR is valid */
    f = fopen(tmp_jar, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        fclose(f);
        printf("[*] Verified temp file size: %ld bytes\n", file_size);
    }
    
    /* 4. Load JVM */
    const char *jvm_path = find_jvm_lib();
    printf("[*] Loading JVM from: %s\n", jvm_path);
    
    dynlib_t jvm_lib = lib_open(jvm_path);
    if (!jvm_lib) {
        fprintf(stderr, "[!] Failed to load JVM library from: %s\n", jvm_path);
        fprintf(stderr, "[!] Please check JAVA_HOME environment variable\n");
        remove(tmp_jar);
        free(plain);
        return 1;
    }
    
    printf("[*] JVM library loaded successfully\n");
    
    JNI_CreateJavaVM_fn create_jvm = (JNI_CreateJavaVM_fn)lib_sym(jvm_lib, "JNI_CreateJavaVM");
    if (!create_jvm) {
        fprintf(stderr, "[!] JNI_CreateJavaVM not found\n");
        remove(tmp_jar);
        free(plain);
        return 1;
    }
    
    /* 5. Create JVM */
    char classpath[512];
    snprintf(classpath, sizeof(classpath), "-Djava.class.path=%s", tmp_jar);
    
    JavaVMOption options[] = {
        { (char *)classpath, NULL },
        { (char *)"-Xmx256m", NULL }
    };
    
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 2;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = JNI_TRUE;
    
    JavaVM *jvm;
    JNIEnv *env;
    jint create_result = create_jvm(&jvm, (void **)&env, &vm_args);
    
    if (create_result != JNI_OK) {
        fprintf(stderr, "[!] JVM creation failed with code: %d\n", create_result);
        remove(tmp_jar);
        free(plain);
        return 1;
    }
    
    printf("[*] JVM created successfully\n");
    
    /* 6. Run main class */
    printf("[*] Looking for main class: %s\n", MAIN_CLASS);
    jclass cls = (*env)->FindClass(env, MAIN_CLASS);
    
    if (!cls) {
        fprintf(stderr, "[!] Main class not found: %s\n", MAIN_CLASS);
        if ((*env)->ExceptionOccurred(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }
        (*jvm)->DestroyJavaVM(jvm);
        remove(tmp_jar);
        free(plain);
        return 1;
    }
    
    printf("[*] Main class found\n");
    
    jmethodID main_method = (*env)->GetStaticMethodID(env, cls, "main", "([Ljava/lang/String;)V");
    if (!main_method) {
        fprintf(stderr, "[!] main() method not found\n");
        if ((*env)->ExceptionOccurred(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }
        (*jvm)->DestroyJavaVM(jvm);
        remove(tmp_jar);
        free(plain);
        return 1;
    }
    
    /* Prepare command line arguments */
    jclass str_class = (*env)->FindClass(env, "java/lang/String");
    jobjectArray args_array = (*env)->NewObjectArray(env, argc - 1, str_class, NULL);
    
    for (int i = 1; i < argc; i++) {
        jstring arg = (*env)->NewStringUTF(env, argv[i]);
        (*env)->SetObjectArrayElement(env, args_array, i - 1, arg);
    }
    
    /* Execute main method */
    printf("[*] Invoking main method...\n");
    (*env)->CallStaticVoidMethod(env, cls, main_method, args_array);
    
    /* Check for exceptions */
    if ((*env)->ExceptionOccurred(env)) {
        fprintf(stderr, "[!] Exception during execution:\n");
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }
    
    /* Cleanup */
    printf("[*] Shutting down JVM...\n");
    (*jvm)->DestroyJavaVM(jvm);
    remove(tmp_jar);
    free(plain);
    
    printf("[*] Application finished\n");
    return 0;
}