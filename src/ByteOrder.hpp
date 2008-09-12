#include <stdlib.h>
#include <endian.h>

static inline uint64_t swap64(uint64_t x){
  return((((uint64_t)x & 0x00000000000000ffULL) << 56) | 
	 (((uint64_t)x & 0x000000000000ff00ULL) << 40) | 
	 (((uint64_t)x & 0x0000000000ff0000ULL) << 24) | 
	 (((uint64_t)x & 0x00000000ff000000ULL) <<  8) |
	 (((uint64_t)x & 0x000000ff00000000ULL) >>  8) | 
	 (((uint64_t)x & 0x0000ff0000000000ULL) >> 24) | 
	 (((uint64_t)x & 0x00ff000000000000ULL) >> 40) | 
	 (((uint64_t)x & 0xff00000000000000ULL) >> 56));
}

static inline uint32_t swap32(uint32_t x){
  return((((uint32_t)x & 0x000000ffU) << 24) | 
	 (((uint32_t)x & 0x0000ff00U) <<  8) | 
	 (((uint32_t)x & 0x00ff0000U) >>  8) | 
	 (((uint32_t)x & 0xff000000U) >> 24));
}

static inline uint16_t swap16(uint16_t x){
  return((((uint16_t)x & 0x00ffU) <<  8) | 
	 (((uint16_t)x & 0xff00U) >>  8));
}

#if BYTE_ORDER == LITTLE_ENDIAN

static inline uint64_t be64_to_cpu(uint64_t x){
  return(swap64(x));
}

static inline uint32_t be32_to_cpu(uint32_t x){
  return(swap32(x));
}

static inline uint16_t be16_to_cpu(uint16_t x){
  return(swap16(x));
}

static inline uint64_t le64_to_cpu(uint64_t x){
  return(x);
}

static inline uint32_t le32_to_cpu(uint32_t x){
  return(x);
}

static inline uint16_t le16_to_cpu(uint16_t x){
  return(x);
}

#else

static inline uint64_t be64_to_cpu(uint64_t x){
  return(x);
}

static inline uint32_t be32_to_cpu(uint32_t x){
  return(x);
}

static inline uint16_t be16_to_cpu(uint16_t x){
  return(x);
}

static inline uint64_t le64_to_cpu(uint64_t x){
  return(swap64(x));
}

static inline uint32_t le32_to_cpu(uint32_t x){
  return(swap32(x));
}

static inline uint16_t le16_to_cpu(uint16_t x){
  return(swap16(x));
}


#endif

static inline uint64_t cpu_to_be64(uint64_t x){
  return(be64_to_cpu(x));
}

static inline uint32_t cpu_to_be32(uint32_t x){
  return(be32_to_cpu(x));
}

static inline uint16_t cpu_to_be16(uint16_t x){
  return(be16_to_cpu(x));
}

static inline uint64_t cpu_to_le64(uint64_t x){
  return(le64_to_cpu(x));
}

static inline uint32_t cpu_to_le32(uint32_t x){
  return(le32_to_cpu(x));
}

static inline uint16_t cpu_to_le16(uint16_t x){
  return(le16_to_cpu(x));
}

