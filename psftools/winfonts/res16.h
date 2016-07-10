
#ifndef RESSTRUCT16_H_INCLUDED
#define RESSTRUCT16_H_INCLUDED 1

/* Based on information in the MSDN library */

typedef struct
{
    wyde rnOffset;      /* Offset of resource data from file start */
    wyde rnLength;      /* Length of resource data */
    wyde rnFlags;       /* See Windows.h */
    wyde rnID;          /* Resource ID */
    wyde rnHandle;      /* Unused? */
    wyde rnUsage;       /* Unused? */
} NAMEINFO16;

typedef struct
{
    wyde        rtTypeID;               /* Resource type - see RSTYPES.H */
    wyde        rtResourceCount;        /* No. of resources of this type */
    dwyde       rtReserved;             /* Unused? */
} TYPEINFO16;

#endif

