#ifndef GLOBAL_DEBUG_H
#define GLOBAL_DEBUG_H

// Define this macro to enable debug printing
// 定义此宏以启用调试打印
#define DEBUG_PRINT

#ifdef DEBUG_PRINT
    #include <QDebug>
    #define DEBUG_MSG_(msg) qDebug() <<msg
#else
    #define DEBUG_MSG_(msg)
#endif

#endif // GLOBAL_DEBUG_H
