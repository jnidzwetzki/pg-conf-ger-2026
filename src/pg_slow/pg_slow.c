#include <unistd.h>
#include <fcntl.h>

#include "postgres.h"
#include "fmgr.h"
#include "storage/fd.h"
#include "utils/builtins.h"


PG_MODULE_MAGIC;

/*
 * Function prototypes 
 */
bool is_odd_cpu(int32);
bool is_odd_io(int32);
bool is_odd_cliff(int32);

/*
 * Fast version of is_odd 
 */
PG_FUNCTION_INFO_V1(is_odd_fast);

Datum is_odd_fast(PG_FUNCTION_ARGS)
{
    int32 val = PG_GETARG_INT32(0);
    bool result = abs(val % 2) == 1;
    PG_RETURN_BOOL(result);
}
   
/*
 * Slow version of is_odd 
 */
PG_FUNCTION_INFO_V1(is_odd_slow);

Datum is_odd_slow(PG_FUNCTION_ARGS)
{
    bool result;
    int32 val = PG_GETARG_INT32(0);

    switch (abs(val % 3))
    {
        case 0:
            result = is_odd_cpu(val);
            break;
        case 1:
            result = is_odd_io(val);
            break;
        case 2:
            result = is_odd_cliff(val);
            break;
        default:
            /* Should never be reached */
            pg_unreachable();
    }

    PG_RETURN_BOOL(result);
}
   
static char *decrementString(const char *s)
{
    long value;
    char *result;

    value = strtol(s, NULL, 10);
    value -= 2;

    result = (char *) palloc(32);
    snprintf(result, 32, "%ld", value);
    return result;
}

bool is_odd_cpu(int32 val)
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

    return (strcmp(current, "1") == 0);
}

bool is_odd_io(int32 val)
{
    File temp_file;
    const char *temp_path;
    int32 abs_val;
    int32 i;
    FILE *f;
    char buf[128];
    char last_line[128] = "";
    bool odd;

    /* Open in Postgres temporary file area */
    temp_file = OpenTemporaryFile(false);
    if (temp_file < 0)
        elog(ERROR, "could not open temporary file");

    temp_path = FilePathName(temp_file);

    abs_val = abs(val);
    f = fopen(temp_path, "a");

    for (i = 0; i <= abs_val; i++)
    {
        if (f == NULL)
            elog(ERROR, "could not open temp file %s", temp_path);

        odd = (i % 2) != 0;
        fprintf(f, "%d %s\n", i, (odd ? "odd" : "even"));
    }
    fclose(f);

    f = fopen(temp_path, "r");
    if (f == NULL)
        elog(ERROR, "could not open temp file %s for reading", temp_path);

    while (fgets(buf, sizeof(buf), f) != NULL)
        strncpy(last_line, buf, sizeof(last_line) - 1);
    fclose(f);

    FileClose(temp_file);
    return (strstr(last_line, " odd") != NULL);
}

bool is_odd_cliff(int32 val)
{
    if (val < 900)
        return abs(val % 2) == 1;
    else
        return is_odd_cpu(val);
}  

/*
 * Sum function 
 */
PG_FUNCTION_INFO_V1(int32_sum_trans);

/*
 * State transition function for mysum aggregate.
 * Based on PG's int4_sum function.
 */
Datum int32_sum_trans(PG_FUNCTION_ARGS)
{
    int64 state;
    int64 newval;

    /* If the state is null initialize it */
    if (PG_ARGISNULL(0))
    {
        /* If the first parameter is also null, the call
         * is a no-op.
         */
        if (PG_ARGISNULL(1))
            PG_RETURN_NULL();

        /* Initialize the state to the first parameter */
        state = (int64)PG_GETARG_INT32(1);
        PG_RETURN_INT64(state);
    }

    /* Get the current state */
    state = PG_GETARG_INT64(0);

    /* If the second parameter is null, the call is a no-op */
    if (PG_ARGISNULL(1))
        PG_RETURN_INT64(state);

    /* Perform the sum operation and return the new state */
    newval = state + (int64)PG_GETARG_INT32(1);
    PG_RETURN_INT64(newval);
}