/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef _key_value_h_
#define _key_value_h_

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif

/* The totally storage size for key-value store */
#define KV_TOTAL_SIZE   (8 * 1024)


/* The physical parition for key-value store */
#define KV_PTN              0x7D000

#define KV_DEFAULT_CUSTOM_PTN      0x7F000

#ifdef CONFIG_AOS_KV_DEBUG
#define KV_DBG_ENABLE 1
#endif

#ifdef DEFAULT_LOG_LEVEL_DEBUG
#define SUPPORT_KV_LIST_CMD
#endif

typedef enum _kv_get_type_e
{
    KV_GET_TYPE_STRING = 1,
    KV_GET_TYPE_BINARY,
    KV_GET_TYPE_INT,
    KV_GET_TYPE_FLOAT,
    KV_GET_TYPE_MAX
}kv_get_type_e;

/**
 * @brief init the kv module.
 *
 * @param[in] none.
 *
 * @note: the default KV size is @HASH_TABLE_MAX_SIZE, the path to store
 *        the kv file is @KVFILE_PATH.
 * @retval  0 on success, otherwise -1 will be returned
 */
int aos_kv_init(void);

/**
 * @brief deinit the kv module.
 *
 * @param[in] none.
 *
 * @note: all the KV in RAM will be released.
 * @retval none.
 */
void aos_kv_deinit(void);


int aos_kv_get(const char *key, void *buffer, int *buffer_len);
int aos_kv_set(const char *key, const void *val, int len, int sync);
int aos_kv_del(const char *key);
int aos_kv_del_all(void);
int kv_check_has_val();

int kv_default_factory_load();
int kv_default_custom_store();
int kv_default_custom_load();


#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif


