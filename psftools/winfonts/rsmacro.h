
#ifndef RSMACRO_H_INCLUDED

#define RSMACRO_H_INCLUDED 1

#define PEEK16(x, y) ((((int )((x)[(y)+1])) << 8) | \
                               (x)[(y)])
#define PEEK32(x, y) ((((long)((x)[(y)+3])) << 24) | \
                      (((long)((x)[(y)+2])) << 16) | \
                      (((long)((x)[(y)+1])) << 8)  | \
                               (x)[(y)])

#endif /* def RSMACRO_H_INCLUDED */

