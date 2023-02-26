const GET_NUMBER  = 0;
const GET_LETTER  = 1;
const MAX_CLIENTS = 5;

struct BAKERY
{
    int  op;
    int  num;
    int  pid;
    char letter;
};

program BAKERY_PROG
{
    version BAKERY_VER
    {
        struct BAKERY BAKERY_PROC(struct BAKERY) = 1; // удаленная процедура
    } = 1; /* Version number = 1 */
} = 0x20000001;