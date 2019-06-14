#include "atomic.h"

int atomic_read(atomic_t*  ref_count_)
{
    return ref_count_->load();
}
void atomic_set( atomic_t*  ref_count_,int i)
{
    ref_count_->store(i);
}
int atomic_dec( atomic_t*  ref_count_)
{
    ref_count_->operator --();
    return ref_count_->load();
}
int atomic_add_return(int i, atomic_t*  ref_count_)
{
    return ref_count_->fetch_add(i);
}

