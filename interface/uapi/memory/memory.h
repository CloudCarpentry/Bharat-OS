#ifndef BHARAT_UAPI_MEMORY_H
#define BHARAT_UAPI_MEMORY_H

#include <stdint.h>

/* Memory classes */
#define BH_MEM_CLASS_NORMAL     0
#define BH_MEM_CLASS_DMA        1
#define BH_MEM_CLASS_RT         2
#define BH_MEM_CLASS_SECURE     3
#define BH_MEM_CLASS_PACKET     4
#define BH_MEM_CLASS_LOWPOWER   5
#define BH_MEM_CLASS_PERSISTENT 6

/* Memory protection flags */
#define BH_PROT_READ    (1 << 0)
#define BH_PROT_WRITE   (1 << 1)
#define BH_PROT_EXEC    (1 << 2)

/* Memory mapping flags */
#define BH_MAP_PRIVATE  (1 << 0)
#define BH_MAP_SHARED   (1 << 1)
#define BH_MAP_ANONYMOUS (1 << 2)
#define BH_MAP_FIXED    (1 << 3)

#endif /* BHARAT_UAPI_MEMORY_H */
