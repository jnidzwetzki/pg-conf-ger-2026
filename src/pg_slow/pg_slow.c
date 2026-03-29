#include <unistd.h>
#include <fcntl.h>

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "storage/fd.h"
#include "storage/latch.h"
#include "utils/builtins.h"
#include "utils/wait_event.h"

PG_MODULE_MAGIC;

/*
 * Function prototypes 
 */
bool is_odd_1(int32);
bool is_odd_2(int32);
bool is_odd_3(int32);

/*
 * Fast version of is_odd 
 */
PG_FUNCTION_INFO_V1(is_odd_fast);

Datum is_odd_fast(PG_FUNCTION_ARGS)
{
    int32 val = PG_GETARG_INT32(0);
    bool result = (val & 1) != 0;
    PG_RETURN_BOOL(result);
}
   
/*
 * Slow version of is_odd 
 */
#define BLACK_BLOCK_DECIDER(x) (abs((x) % 3))

PG_FUNCTION_INFO_V1(is_odd_slow);

Datum is_odd_slow(PG_FUNCTION_ARGS)
{
    bool result;
    int32 val = PG_GETARG_INT32(0);

    switch (BLACK_BLOCK_DECIDER(val))
    {
        case 0:
            result = is_odd_1(val);
            break;
        case 1:
            result = is_odd_2(val);
            break;
        case 2:
            result = is_odd_3(val);
            break;
        default:
            /* Should never be reached */
            pg_unreachable();
    }

    PG_RETURN_BOOL(result);
}
 
/*
 * Is Odd using a CPU-intensive method
 */
char *decrementString(const char *s)
{
    long value;
    char *result;

    value = strtol(s, NULL, 10);
    value -= 2;

    result = (char *) palloc(32);
    snprintf(result, 32, "%ld", value);
    return result;
}

bool is_odd_1(int32 val)
{
    long current_val = val;
    char *current;

    /* Handle negative values */
    if (current_val < 0)
        current_val = -current_val;

    /* Convert to string for processing */
    current = (char *) palloc(32);
    snprintf(current, 32, "%ld", current_val);

    /* Process the string until we reach 0 or 1 */
    while (strcmp(current, "0") != 0 && strcmp(current, "1") != 0)
    {
        char *next = decrementString(current);
        pfree(current);
        current = next;
    }

    bool is_odd = (strcmp(current, "1") == 0);
    pfree(current);
    return is_odd;
}

/* 
 * Is Odd using a CPU-intensive method with a conditional branch
 */
bool is_odd_2(int32 val)
{
    int i;
    if (val > 40000)
    {
        for(i = 0; i < 2000000; i++)
        {
            (void) (i * i);
        }
    }

    return (val & 1) != 0;
}  

/*
 * Is Odd using a latch
 */
bool is_odd_3(int32 val)
{
    if (val > 0 && val < 10)
    {
        (void) WaitLatch(MyLatch,
                    WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH,
                    10000,
                    WAIT_EVENT_PG_SLEEP);
		ResetLatch(MyLatch);
    }
        
    return (val & 1) != 0;
}
