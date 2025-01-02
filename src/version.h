/**
 * @file version.h
 *
 */
#define APP_NAME "precizer"
#define APP_VERSION 0002

/**
 * @brief Helper macro for stringification
 */
#define STRINGIFY(x) #x

/**
 * @brief Helper macro for double expansion to resolve macros
 */
#define TOSTRING(x) STRINGIFY(x)

/**
 * @brief Version as string constant
 */
#define APP_VERSION_STR TOSTRING(APP_VERSION)
