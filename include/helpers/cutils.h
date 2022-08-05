#ifndef _CUTILS_H_
#define _CUTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int cu_err_t;

#define CU_OK                 (0)
#define CU_FAIL              (-1)
#define CU_ERR_INVALID_ARG   (-2)
#define CU_ERR_NO_MEM        (-3)
#define CU_ERR_SYNTAX_ERROR  (-4)
#define CU_ERR_NOT_FOUND     (-5)
#define CU_ERR_EMPTY_STRING  (-6)
#define CU_ERR_INVALID_CHAR  (-7)
#define CU_ERR_OUT_OF_BOUNDS (-8)

/**
 * @brief Universal handle constructor. User is responsible for freeing the resulting object
 * @param handle_type Handle type (ex: person_handle_t)
 * @param struct Struct (ex: struct person)
 * @param ... Struct attributes (ex: .id = 2, .name = strdup("John"))
 * @return Pointer to dynamically allocated struct
 */
#define cu_tctor(handle_type, struct, ...) \
    __extension__ ({ handle_type obj = calloc(1, sizeof(struct)); if(obj) { *obj = (struct){ __VA_ARGS__ }; } obj; })

/**
 * @brief Universal struct (type) constructor. User is responsible for freeing the resulting object
 * @param type Struct type name (ex: person_t)
 * @param ... Struct attributes (ex: .id = 2, .name = strdup("John"))
 * @return Pointer to dynamically allocated struct
 */
#define cu_ctor(type, ...) \
    cu_tctor(type*, type, __VA_ARGS__)

/**
 * @brief Free the list (array of pointers) with custom type for the length variable,
 *        and custom function for freeing the list item.
 * @param list Double pointer to list
 * @param len_type Type of list length variable
 * @param len Number of the items in the list
 * @param item_free_fnc Function for freeing the list item
 * @return void
 */
#define cu_list_tfreex(list, len_type, len, item_free_fnc) \
    __extension__ ({ if(list) { for(len_type i = 0; i < len; i++) { item_free_fnc(list[i]); list[i] = NULL; } free(list); list = NULL; len = 0; } })

/**
 * @brief Free the list (array of pointers) with custom type for the length variable.
 *        For the freeing the list items free() will be used
 * @param list Double pointer to list
 * @param len_type Type of the list length variable
 * @param len Number of the items in the list
 * @return void
 */
#define cu_list_tfree(list, len_type, len) \
    cu_list_tfreex(list, len_type, len, free)

/**
 * @brief Free the list (array of pointers). For the freeing the list items free() will be used
 * @param list Double pointer to list
 * @param len Number of the items in the list
 * @return void
 */
#define cu_list_free(list, len) \
    cu_list_tfree(list, int, len)

/**
 * @brief Free the list (array of pointers) with custom function for freeing the list items
 * @param list Double pointer to list
 * @param len Number of the items in the list
 * @param free_fnc Custom function for freeing list items
 * @return void
 */
#define cu_list_freex(list, len, free_fnc) \
    cu_list_tfreex(list, int, len, free_fnc)

#define cu_err_checkl(EXP, label) \
    if((err = (EXP)) != CU_OK) { goto label; }

#define cu_err_negative_check(EXP) \
    if((err = (EXP)) < 0) { goto _error; } else { err = CU_OK; }

#define cu_err_check(EXP) \
    cu_err_checkl(EXP, _error)

#define cu_err_checkr(EXP) \
    if((err = (EXP)) != CU_OK) { return err; }

#define cu_mem_check(EXP) \
    if(!(EXP)) { err = CU_ERR_NO_MEM; goto _error; }

#define cu_mem_checkr(EXP) \
    if(!(EXP)) { return CU_ERR_NO_MEM; }

#ifdef __cplusplus
}
#endif

#endif