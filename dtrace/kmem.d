#pragma D option quiet

inline uint64_t a = '$1';
inline int      l = '$2';

dtrace:::BEGIN
{
    tracemem(a,l);
    exit(0);
}

dtrace:::ERROR {
    exit(1);
}