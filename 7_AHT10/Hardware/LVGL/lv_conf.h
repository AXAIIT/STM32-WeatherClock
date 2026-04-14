/**
 * @file lv_conf.h
 * 配置文件，适用于 v8.3.11
 */

/*
 * 将此文件复制为 `lv_conf.h`
 * 1. 简单地放置在 `lvgl` 文件夹旁边
 * 2. 或放置在其他任何位置，并
 *    - 定义 `LV_CONF_INCLUDE_SIMPLE`
 *    - 将路径添加为包含路径
 */

/* clang-format off */
#if 1 /* 将其设置为 "1" 以启用内容 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   颜色设置
 *====================*/

/*颜色深度：1（每像素1字节），8（RGB332），16（RGB565），32（ARGB8888）*/
#define LV_COLOR_DEPTH 16

/* 交换 RGB565 颜色的两个字节。如果显示器具有 8 位接口（例如 SPI），此选项非常有用 */
#define LV_COLOR_16_SWAP 0

/* 启用在透明背景上绘制的功能。
 * 如果使用了 opa 和 transform_* 样式属性，这是必需的。
 * 也可以在 UI 位于另一层（例如 OSD 菜单或视频播放器）之上时使用。*/
#define LV_COLOR_SCREEN_TRANSP 0

/* 调整颜色混合函数的舍入方式。GPU 可能以不同方式计算颜色混合（混合）。
 * 0: 向下舍入，64: 从 x.75 开始向上舍入，128: 从一半开始向上舍入，192: 从 x.25 开始向上舍入，254: 向上舍入 */
#define LV_COLOR_MIX_ROUND_OFS 0

/* 如果图像像素具有此颜色，并且启用了色度键功能，则这些像素将不会被绘制 */
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)         /* 纯绿色 */

/*=========================
   内存设置
 *=========================*/

/* 1: 使用自定义的 malloc/free，0: 使用内置的 `lv_mem_alloc()` 和 `lv_mem_free()` */
#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
    /* `lv_mem_alloc()` 可用内存的大小（以字节为单位，>= 2kB） */
    #define LV_MEM_SIZE (24U * 1024U)          /*[bytes]*/

    /* 为内存池设置一个地址，而不是将其分配为普通数组。也可以放在外部 SRAM 中。 */
    #define LV_MEM_ADR 0     /*0: unused*/
    /* 而不是提供一个地址，提供一个内存分配器，该分配器将被调用以为 LVGL 获取一个内存池。例如 my_malloc */
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif

#else       /*LV_MEM_CUSTOM*/
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /*Header for the dynamic memory function*/
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif     /*LV_MEM_CUSTOM*/

/* 中间内存缓冲区的数量，用于渲染和其他内部处理机制。
 * 如果缓冲区不足，您将看到一条错误日志消息。 */
#define LV_MEM_BUF_MAX_NUM 16

/* 使用标准的 `memcpy` 和 `memset` 替代 LVGL 自定义的函数。（可能会更快，也可能不会） */
#define LV_MEMCPY_MEMSET_STD 0

/*====================
   HAL 设置
 *====================*/

/* 默认显示刷新周期。LVGL 将以此周期重绘更改的区域 */
#define LV_DISP_DEF_REFR_PERIOD 5      /*[ms]*/

/*Input device read period in milliseconds*/
#define LV_INDEV_DEF_READ_PERIOD 5     /*[ms]*/

/* 使用自定义的时间源来提供以毫秒为单位的已用时间。
 * 这样就不需要手动使用 `lv_tick_inc()` 来更新时间了。*/
#define LV_TICK_CUSTOM 0
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"         /*Header for the system time function*/
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())    /*Expression evaluating to current system time in ms*/
    /*If using lvgl as ESP32 component*/
    // #define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
    // #define LV_TICK_CUSTOM_SYS_TIME_EXPR ((esp_timer_get_time() / 1000LL))
#endif   /*LV_TICK_CUSTOM*/

/* 默认每英寸点数（DPI）。用于初始化默认大小，例如小部件大小、样式填充。
 * （不是很重要，您可以调整它来修改默认大小和间距） */
#define LV_DPI_DEF 130     /*[px/inch]*/

/*=======================
 * 功能配置
 *=======================*/

/*-------------
 * 绘图
 *-----------*/

/* 启用复杂绘图引擎。
 * 需要启用此功能才能绘制阴影、渐变、圆角、圆形、弧线、倾斜线、图像变换或任何遮罩 */
#define LV_DRAW_COMPLEX 1
#if LV_DRAW_COMPLEX != 0

    /* 允许缓存一些阴影计算。
    * LV_SHADOW_CACHE_SIZE 是最大阴影大小的缓存值，其中阴影大小为 `shadow_width + radius`
    * 缓存会消耗 LV_SHADOW_CACHE_SIZE^2 的 RAM */
    #define LV_SHADOW_CACHE_SIZE 0

    /* 设置最大缓存的圆形数据数量。
    * 1/4 圆的周长数据会被保存，用于抗锯齿处理。
    * 每个圆的缓存会使用 radius * 4 字节（最常用的半径会被缓存）。
    * 0: 禁用缓存 */
    #define LV_CIRCLE_CACHE_SIZE 4
#endif /*LV_DRAW_COMPLEX*/

/**
 * “简单图层”用于当小部件的 `style_opa < 255` 时，将小部件缓存在图层中，
 * 并以给定的不透明度将其作为图像进行混合。
 * 请注意，`bg_opa`、`text_opa` 等不需要缓存在图层中。
 * 小部件可以分块缓存在较小的缓冲区中，以避免使用大缓冲区。
 *
 * - LV_LAYER_SIMPLE_BUF_SIZE: [字节] 最佳目标缓冲区大小。LVGL 将尝试分配它。
 * - LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE: [字节] 如果无法分配 `LV_LAYER_SIMPLE_BUF_SIZE`，则使用此值。
 *
 * 两个缓冲区大小均以字节为单位。
 * “变换图层”（使用 transform_angle/zoom 属性）使用更大的缓冲区，
 * 并且无法分块绘制。因此，这些设置仅影响具有不透明度的小部件。
 */
#define LV_LAYER_SIMPLE_BUF_SIZE          (24 * 1024)
#define LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE (3 * 1024)

/* 默认图像缓存大小。图像缓存会保持图像处于打开状态。
 * 如果仅使用内置的图像格式，缓存并没有实际优势。（例如，如果没有添加新的图像解码器）
 * 对于复杂的图像解码器（例如 PNG 或 JPG），缓存可以避免图像的持续打开/解码。
 * 然而，打开的图像可能会消耗额外的 RAM。
 * 0: 禁用缓存 */
#define LV_IMG_CACHE_DEF_SIZE 0

/* 允许每个渐变的停止点数量。增加此值以允许更多停止点。
 * 每增加一个停止点会增加 (sizeof(lv_color_t) + 1) 字节的内存消耗 */
#define LV_GRADIENT_MAX_STOPS 2

/* 默认渐变缓冲区大小。
 * 当 LVGL 计算渐变“映射”时，它可以将其保存到缓存中，以避免再次计算。
 * LV_GRAD_CACHE_DEF_SIZE 设置此缓存的大小（以字节为单位）。
 * 如果缓存太小，映射将仅在绘制时分配。
 * 0 表示不使用缓存。*/
#define LV_GRAD_CACHE_DEF_SIZE 0

/* 允许对渐变进行抖动（以在颜色深度有限的显示器上实现视觉上的平滑颜色渐变）。
 * LV_DITHER_GRADIENT 意味着需要为对象的渲染表面分配一行或两行额外的内存。
 * 如果使用误差扩散，内存消耗的增加为 (32 位 * 对象宽度) 加上 24 位 * 对象宽度。 */
#define LV_DITHER_GRADIENT 0
#if LV_DITHER_GRADIENT
    /* 添加对误差扩散抖动的支持。
    * 误差扩散抖动可以获得更好的视觉效果，但会增加绘图时的 CPU 消耗和内存使用。
    * 内存消耗的增加为 (24 位 * 对象宽度)。 */
    #define LV_DITHER_ERROR_DIFFUSION 0
#endif

/* 最大分配用于旋转的缓冲区大小。
 * 仅在显示驱动程序中启用了软件旋转时使用。 */
#define LV_DISP_ROT_MAX_BUF (10*1024)

/*-------------
 * GPU
 *-----------*/

/* 使用 Arm 的 2D 加速库 Arm-2D */
#define LV_USE_GPU_ARM2D 0

/* 使用 STM32 的 DMA2D（又称 Chrom Art）GPU */
#define LV_USE_GPU_STM32_DMA2D 0
#if LV_USE_GPU_STM32_DMA2D
/* 必须定义目标处理器的 CMSIS 头文件路径
   例如 "stm32f7xx.h" 或 "stm32f4xx.h" */
    #define LV_GPU_DMA2D_CMSIS_INCLUDE
#endif

/* 启用 RA6M3 G2D GPU */
#define LV_USE_GPU_RA6M3_G2D 0
#if LV_USE_GPU_RA6M3_G2D
    /* 定义目标处理器的包含路径
   例如 "hal_data.h" */
    #define LV_GPU_RA6M3_G2D_INCLUDE "hal_data.h"
#endif

/* 使用 SWM341 的 DMA2D GPU */
#define LV_USE_GPU_SWM341_DMA2D 0
#if LV_USE_GPU_SWM341_DMA2D
    #define LV_GPU_SWM341_DMA2D_INCLUDE "SWM341.h"
#endif

/* 使用 NXP 的 PXP GPU（适用于 iMX RTxxx 平台） */
#define LV_USE_GPU_NXP_PXP 0
#if LV_USE_GPU_NXP_PXP
    /* 1: 为 PXP 添加默认的裸机和 FreeRTOS 中断处理例程（lv_gpu_nxp_pxp_osa.c），
    *    并在 lv_init() 期间自动调用 lv_gpu_nxp_pxp_init()。
    *    注意：需要定义符号 SDK_OS_FREE_RTOS 才能使用 FreeRTOS OSA，否则将选择裸机实现。
    * 0: 必须在 lv_init() 之前手动调用 lv_gpu_nxp_pxp_init()。
    */
    #define LV_USE_GPU_NXP_PXP_AUTO_INIT 0
#endif

/*Use NXP's VG-Lite GPU iMX RTxxx platforms*/
#define LV_USE_GPU_NXP_VG_LITE 0

/*Use SDL renderer API*/
#define LV_USE_GPU_SDL 0
#if LV_USE_GPU_SDL
    #define LV_GPU_SDL_INCLUDE_PATH <SDL2/SDL.h>
    /*Texture cache size, 8MB by default*/
    #define LV_GPU_SDL_LRU_SIZE (1024 * 1024 * 8)
    /*Custom blend mode for mask drawing, disable if you need to link with older SDL2 lib*/
    #define LV_GPU_SDL_CUSTOM_BLEND_MODE (SDL_VERSION_ATLEAST(2, 0, 6))
#endif

/*-------------
 * 日志记录
 *-----------*/

/* 启用日志模块 */
#define LV_USE_LOG 0
#if LV_USE_LOG

    /* 如何记录日志的重要性：
        * LV_LOG_LEVEL_TRACE       记录大量日志以提供详细信息
        * LV_LOG_LEVEL_INFO        记录重要事件
        * LV_LOG_LEVEL_WARN        记录发生了不希望发生的事情但未引起问题
        * LV_LOG_LEVEL_ERROR       仅记录关键问题，当系统可能失败时
        * LV_LOG_LEVEL_USER        仅记录用户添加的日志
        * LV_LOG_LEVEL_NONE        不记录任何内容
    */
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /* 1: 使用 'printf' 打印日志；
    * 0: 用户需要通过 `lv_log_register_print_cb()` 注册一个回调函数 */
    #define LV_LOG_PRINTF 0

    /* 启用/禁用在生成大量日志的模块中使用 LV_LOG_TRACE */
    #define LV_LOG_TRACE_MEM        1
    #define LV_LOG_TRACE_TIMER      1
    #define LV_LOG_TRACE_INDEV      1
    #define LV_LOG_TRACE_DISP_REFR  1
    #define LV_LOG_TRACE_EVENT      1
    #define LV_LOG_TRACE_OBJ_CREATE 1
    #define LV_LOG_TRACE_LAYOUT     1
    #define LV_LOG_TRACE_ANIM       1

#endif  /*LV_USE_LOG*/

/*-------------
 * 断言
 *-----------*/

/* 如果某个操作失败或发现无效数据，则启用断言。
 * 如果启用了 LV_USE_LOG，则在失败时会打印一条错误消息。 */
#define LV_USE_ASSERT_NULL          1   /* 检查参数是否为 NULL。（非常快，推荐使用） */
#define LV_USE_ASSERT_MALLOC        1   /* 检查内存是否成功分配。（非常快，推荐使用） */
#define LV_USE_ASSERT_STYLE         1   /* 检查样式是否正确初始化。（非常快，推荐使用） */
#define LV_USE_ASSERT_MEM_INTEGRITY 0   /* 检查 `lv_mem` 在关键操作后的完整性。（较慢） */
#define LV_USE_ASSERT_OBJ           0   /* 检查对象的类型和存在性（例如，未被删除）。（较慢） */

/* 当断言发生时添加一个自定义处理程序，例如重启 MCU */
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);   /*Halt by default*/

/*-------------
 * 其他
 *-----------*/

/*1: 显示 CPU 使用率和 FPS 计数 */
#define LV_USE_PERF_MONITOR 0
#if LV_USE_PERF_MONITOR
    #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT
#endif

/*1: 显示已用内存和内存碎片化情况
 * 需要将 LV_MEM_CUSTOM 设置为 1 */
#define LV_USE_MEM_MONITOR 0
#if LV_USE_MEM_MONITOR
    #define LV_USE_MEM_MONITOR_POS LV_ALIGN_BOTTOM_LEFT
#endif

/*1: 绘制随机颜色的矩形以标记重绘区域*/
#define LV_USE_REFR_DEBUG 0

/* 更改内置的 (v)snprintf 函数 */
#define LV_SPRINTF_CUSTOM 0
#if LV_SPRINTF_CUSTOM
    #define LV_SPRINTF_INCLUDE <stdio.h>
    #define lv_snprintf  snprintf
    #define lv_vsnprintf vsnprintf
#else   /*LV_SPRINTF_CUSTOM*/
    #define LV_SPRINTF_USE_FLOAT 0
#endif  /*LV_SPRINTF_CUSTOM*/

#define LV_USE_USER_DATA 1

/* 垃圾回收器设置
 * 如果 LVGL 绑定到更高级别的语言，并且内存由该语言管理，则使用此设置 */
#define LV_ENABLE_GC 0
#if LV_ENABLE_GC != 0
    #define LV_GC_INCLUDE "gc.h"                           /* 包含与垃圾回收器相关的内容 */
#endif /*LV_ENABLE_GC*/

/*=====================
 * 编译器设置
 *====================*/

/* 对于大端系统设置为 1 */
#define LV_BIG_ENDIAN_SYSTEM 0

/* 定义 `lv_tick_inc` 函数的自定义属性 */
#define LV_ATTRIBUTE_TICK_INC

/* 为 `lv_timer_handler` 函数定义一个自定义属性 */
#define LV_ATTRIBUTE_TIMER_HANDLER

/* 为 `lv_disp_flush_ready` 函数定义一个自定义属性 */
#define LV_ATTRIBUTE_FLUSH_READY

/* 缓冲区所需的对齐大小 */
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1

/* 将在需要对齐内存的地方添加（在使用 -Os 时，数据可能默认未对齐到边界）。
 * 例如：__attribute__((aligned(4))) */
#define LV_ATTRIBUTE_MEM_ALIGN

/* 用于标记大型常量数组的属性，例如字体的位图 */
#define LV_ATTRIBUTE_LARGE_CONST

/* 编译器前缀，用于在 RAM 中声明大型数组 */
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/* 将性能关键的函数放入更快的内存中（例如 RAM） */
#define LV_ATTRIBUTE_FAST_MEM

/* 前缀变量用于 GPU 加速操作，这些变量通常需要放置在 DMA 可访问的 RAM 区域 */
#define LV_ATTRIBUTE_DMA

/* 导出整数常量到绑定。这一宏用于 LV_<CONST> 形式的常量，
 * 这些常量也应该出现在 LVGL 的绑定 API（例如 Micropython）中。 */
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning /* 默认值仅用于防止 GCC 警告 */

/* 将默认的 -32k..32k 坐标范围扩展为 -4M..4M，通过使用 int32_t 替代 int16_t 表示坐标 */
#define LV_USE_LARGE_COORD 0

/*==================
 *   字体使用
 *==================*/

/* Montserrat 字体，包含 ASCII 范围和一些符号，使用 bpp = 4
 * https://fonts.google.com/specimen/Montserrat */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* 演示特殊功能 */
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /*bpp = 3*/
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /* 希伯来文、阿拉伯文、波斯文字符及其所有形式 */
#define LV_FONT_SIMSUN_16_CJK            1  /* 1000 个最常用的 CJK 偏旁部首 */

/* 像素完美的等宽字体 */
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

/* 可选地在此处声明自定义字体。
 * 您也可以将这些字体用作默认字体，并且它们将在全局范围内可用。
 * 例如：#define LV_FONT_CUSTOM_DECLARE   LV_FONT_DECLARE(my_font_1) LV_FONT_DECLARE(my_font_2) */
#define LV_FONT_CUSTOM_DECLARE

/* 始终设置一个默认字体 */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* 启用处理大字体和/或包含大量字符的字体。
 * 字体的限制取决于字体大小、字体样式和 bpp。
 * 如果字体需要此功能，将触发编译器错误。 */
#define LV_FONT_FMT_TXT_LARGE 0

/* 启用/禁用对压缩字体的支持。 */
#define LV_USE_FONT_COMPRESSED 0

/* 启用子像素渲染 */
#define LV_USE_FONT_SUBPX 0
#if LV_USE_FONT_SUBPX
    /*Set the pixel order of the display. Physical order of RGB channels. Doesn't matter with "normal" fonts.*/
    #define LV_FONT_SUBPX_BGR 0  /*0: RGB; 1:BGR order*/
#endif

/* 启用在未找到字形描述时绘制占位符 */
#define LV_USE_FONT_PLACEHOLDER 1

/*=================
 *  文本设置
 *=================*/

/**
 * 选择字符串的字符编码。
 * 您的 IDE 或编辑器应使用相同的字符编码。
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/* 可在这些字符上换行（折行） */
#define LV_TXT_BREAK_CHARS " ,.;:-_"

/* 如果一个单词至少有这么长，将在“最合适”的地方断行
 * 若要禁用，请设置为 <= 0 的值 */
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/* 长单词中在换行前放置的最少字符数。
 * 取决于 LV_TXT_LINE_BREAK_LONG_LEN。 */
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/* 长单词中在换行后放置的最少字符数。
 * 取决于 LV_TXT_LINE_BREAK_LONG_LEN。 */
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/* 用于信号文本重新着色的控制字符。 */
#define LV_TXT_COLOR_CMD "#"

/* 支持双向文本。允许混合从左到右和从右到左的文本。
 * 方向将根据 Unicode 双向算法处理：
 * https://www.w3.org/International/articles/inline-bidi-markup/uba-basics */
#define LV_USE_BIDI 0
#if LV_USE_BIDI
    /* 设置默认方向。支持的值：
    * `LV_BASE_DIR_LTR` 从左到右
    * `LV_BASE_DIR_RTL` 从右到左
    * `LV_BASE_DIR_AUTO` 自动检测文本的基础方向 */
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/* 启用阿拉伯语/波斯语处理
 * 在这些语言中，字符应根据其在文本中的位置替换为其他形式 */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
 *  小部件使用
 *================*/

/* 小部件的文档： https://docs.lvgl.io/latest/en/html/widgets/index.html */
#define LV_USE_ARC        0

#define LV_USE_BAR        0

#define LV_USE_BTN        1

#define LV_USE_BTNMATRIX  1

#define LV_USE_CANVAS     0

#define LV_USE_CHECKBOX   0

#define LV_USE_DROPDOWN   0   /*Requires: lv_label*/

#define LV_USE_IMG        1   /*Requires: lv_label*/

#define LV_USE_LABEL      1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1 /* 启用标签文本的选择功能 */
    #define LV_LABEL_LONG_TXT_HINT 1  /* 在标签中存储一些额外的信息，以加速绘制非常长的文本 */
#endif

#define LV_USE_LINE       0

#define LV_USE_ROLLER     0   /*Requires: lv_label*/
#if LV_USE_ROLLER
    #define LV_ROLLER_INF_PAGES 7 /*Number of extra "pages" when the roller is infinite*/
#endif

#define LV_USE_SLIDER     0   /*Requires: lv_bar*/

#define LV_USE_SWITCH     0

#define LV_USE_TEXTAREA   0   /*Requires: lv_label*/
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500    /*ms*/
#endif

#define LV_USE_TABLE      0

/*==================
 * 额外组件
 *==================*/

/*-----------
 * 小部件
 *----------*/
#define LV_USE_ANIMIMG    0

#define LV_USE_CALENDAR   1
#if LV_USE_CALENDAR
    #define LV_CALENDAR_WEEK_STARTS_MONDAY 0
    #if LV_CALENDAR_WEEK_STARTS_MONDAY
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
    #else
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
    #endif

    #define LV_CALENDAR_DEFAULT_MONTH_NAMES {"January", "February", "March",  "April", "May",  "June", "July", "August", "September", "October", "November", "December"}
    #define LV_USE_CALENDAR_HEADER_ARROW 1
    #define LV_USE_CALENDAR_HEADER_DROPDOWN 0
#endif  /*LV_USE_CALENDAR*/

#define LV_USE_CHART      0

#define LV_USE_COLORWHEEL 0

#define LV_USE_IMGBTN     0

#define LV_USE_KEYBOARD   0

#define LV_USE_LED        0

#define LV_USE_LIST       0

#define LV_USE_MENU       0

#define LV_USE_METER      0

#define LV_USE_MSGBOX     0

#define LV_USE_SPAN       1
#if LV_USE_SPAN
    /*A line text can contain maximum num of span descriptor */
    #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif

#define LV_USE_SPINBOX    0

#define LV_USE_SPINNER    0

#define LV_USE_TABVIEW    0

#define LV_USE_TILEVIEW   0

#define LV_USE_WIN        0

/*-----------
 * 主题
 *----------*/

/* 一个简单、令人印象深刻且非常完整的主题 */
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT

    /* 0: 亮色模式；1: 暗色模式 */
    #define LV_THEME_DEFAULT_DARK 0

    /* 1: 启用按下时的放大效果 */
    #define LV_THEME_DEFAULT_GROW 1

    /* 默认过渡时间，以毫秒为单位 */
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif /*LV_USE_THEME_DEFAULT*/

/* 一个非常简单的主题，是自定义主题的良好起点 */
#define LV_USE_THEME_BASIC 0

/* 一个为单色显示器设计的主题 */
#define LV_USE_THEME_MONO 0

/*-----------
 * 布局
 *----------*/

/* 一个类似于 CSS 中 Flexbox 的布局。 */
#define LV_USE_FLEX 1

/* 一个类似于 CSS 中 Grid 的布局。 */
#define LV_USE_GRID 0

/*---------------------
 * 第三方库
 *--------------------*/

/* 文件系统接口，用于常见的 API */

/* 用于 fopen、fread 等的 API */
#define LV_USE_FS_STDIO 0
#if LV_USE_FS_STDIO
    #define LV_FS_STDIO_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
    #define LV_FS_STDIO_PATH ""         /*Set the working directory. File/directory paths will be appended to it.*/
    #define LV_FS_STDIO_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif

/*API for open, read, etc*/
#define LV_USE_FS_POSIX 0
#if LV_USE_FS_POSIX
    #define LV_FS_POSIX_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
    #define LV_FS_POSIX_PATH ""         /*Set the working directory. File/directory paths will be appended to it.*/
    #define LV_FS_POSIX_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif

/* 用于 CreateFile、ReadFile 等的 API */
#define LV_USE_FS_WIN32 0
#if LV_USE_FS_WIN32
    #define LV_FS_WIN32_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
    #define LV_FS_WIN32_PATH ""         /*Set the working directory. File/directory paths will be appended to it.*/
    #define LV_FS_WIN32_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif

/* 用于 FATFS 的 API（需要单独添加）。使用 f_open、f_read 等 */
#define LV_USE_FS_FATFS 0
#if LV_USE_FS_FATFS
    #define LV_FS_FATFS_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
    #define LV_FS_FATFS_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif

/* 用于 LittleFS 的 API（需要单独添加）。使用 lfs_file_open、lfs_file_read 等 */
#define LV_USE_FS_LITTLEFS 0
#if LV_USE_FS_LITTLEFS
    #define LV_FS_LITTLEFS_LETTER '\0'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
    #define LV_FS_LITTLEFS_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif

/*PNG decoder library*/
#define LV_USE_PNG 0

/*BMP decoder library*/
#define LV_USE_BMP 0

/* JPG + split JPG decoder library.
 * Split JPG is a custom format optimized for embedded systems. */
#define LV_USE_SJPG 0

/*GIF decoder library*/
#define LV_USE_GIF 0

/*QR code library*/
#define LV_USE_QRCODE 0

/*FreeType library*/
#define LV_USE_FREETYPE 0
#if LV_USE_FREETYPE
    /*Memory used by FreeType to cache characters [bytes] (-1: no caching)*/
    #define LV_FREETYPE_CACHE_SIZE (16 * 1024)
    #if LV_FREETYPE_CACHE_SIZE >= 0
        /* 1: bitmap cache use the sbit cache, 0:bitmap cache use the image cache. */
        /* sbit cache:it is much more memory efficient for small bitmaps(font size < 256) */
        /* if font size >= 256, must be configured as image cache */
        #define LV_FREETYPE_SBIT_CACHE 0
        /* Maximum number of opened FT_Face/FT_Size objects managed by this cache instance. */
        /* (0:use system defaults) */
        #define LV_FREETYPE_CACHE_FT_FACES 0
        #define LV_FREETYPE_CACHE_FT_SIZES 0
    #endif
#endif

/*Tiny TTF library*/
#define LV_USE_TINY_TTF 0
#if LV_USE_TINY_TTF
    /*Load TTF data from files*/
    #define LV_TINY_TTF_FILE_SUPPORT 0
#endif

/*Rlottie library*/
#define LV_USE_RLOTTIE 0

/*FFmpeg library for image decoding and playing videos
 *Supports all major image formats so do not enable other image decoder with it*/
#define LV_USE_FFMPEG 0
#if LV_USE_FFMPEG
    /*Dump input information to stderr*/
    #define LV_FFMPEG_DUMP_FORMAT 0
#endif

/*-----------
 * 其他
 *----------*/

/*1: 启用对象快照功能的 API */
#define LV_USE_SNAPSHOT 0

/*1: 启用 Monkey 测试 */
#define LV_USE_MONKEY 0

/*1: 启用网格导航 */
#define LV_USE_GRIDNAV 0

/*1: 启用 lv_obj 片段功能 */
#define LV_USE_FRAGMENT 0

/*1: 支持在标签或 span 小部件中使用图像作为字体 */
#define LV_USE_IMGFONT 0

/*1: 启用基于发布-订阅的消息系统 */
#define LV_USE_MSG 0

/*1: 启用拼音输入法*/
/*需要：lv_keyboard*/
#define LV_USE_IME_PINYIN 0
#if LV_USE_IME_PINYIN
    /*1: Use default thesaurus*/
    /*If you do not use the default thesaurus, be sure to use `lv_ime_pinyin` after setting the thesauruss*/
    #define LV_IME_PINYIN_USE_DEFAULT_DICT 1
    /*Set the maximum number of candidate panels that can be displayed*/
    /*This needs to be adjusted according to the size of the screen*/
    #define LV_IME_PINYIN_CAND_TEXT_NUM 6

    /*Use 9 key input(k9)*/
    #define LV_IME_PINYIN_USE_K9_MODE      1
    #if LV_IME_PINYIN_USE_K9_MODE == 1
        #define LV_IME_PINYIN_K9_CAND_TEXT_NUM 3
    #endif // LV_IME_PINYIN_USE_K9_MODE
#endif

/*==================
 * 示例
 *==================*/

/* 启用示例与库一起构建 */
#define LV_BUILD_EXAMPLES 0

/*===================
 * DEMO 使用
 ====================*/

/* 显示一些小部件。可能需要增加 `LV_MEM_SIZE` */
#define LV_USE_DEMO_WIDGETS 0
#if LV_USE_DEMO_WIDGETS
#define LV_DEMO_WIDGETS_SLIDESHOW 0
#endif

/* 演示编码器和键盘的使用 */
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0

/* 基准测试你的系统 */
#define LV_USE_DEMO_BENCHMARK 0
#if LV_USE_DEMO_BENCHMARK
/*Use RGB565A8 images with 16 bit color depth instead of ARGB8565*/
#define LV_DEMO_BENCHMARK_RGB565A8 0
#endif

/* 对 LVGL 进行压力测试 */
#define LV_USE_DEMO_STRESS 0

/* 音乐播放器演示 */
#define LV_USE_DEMO_MUSIC 0
#if LV_USE_DEMO_MUSIC
    #define LV_DEMO_MUSIC_SQUARE    0
    #define LV_DEMO_MUSIC_LANDSCAPE 0
    #define LV_DEMO_MUSIC_ROUND     0
    #define LV_DEMO_MUSIC_LARGE     0
    #define LV_DEMO_MUSIC_AUTO_PLAY 0
#endif

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_H*/

#endif /*End of "Content enable"*/
