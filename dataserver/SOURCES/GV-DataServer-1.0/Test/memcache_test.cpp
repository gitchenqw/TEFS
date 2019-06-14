//memcache client example
/*
 * File:memcache_test.cpp
 * DATA:2014-8-13 
 *
 Contains:
 *             create memcached 
 *             set value
 *             get value
 *             fetch muti-key values
 *             compare and set(memcached_cs)
 *             delete 
 *             prepend value
 *             increment value by offset
 * 
*/

#include <stdio.h>
#include <libmemcached/memcached.h>

void fetchmultikeys(memcached_st *memc,char**keys,size_t* key_length);
void SetKey(memcached_st *memc, const char* key, size_t key_length, const char*value, size_t value_length);

int main()
{
    const char *config_string= "--SERVER=127.0.0.1";
    memcached_st *memc;
    memcached_return_t rc;
    memc= memcached(config_string, strlen(config_string));
/*    
    //create and destroy memcache pool
    memcached_pool_st* pool= memcached_pool(config_string, strlen(config_string));
    memc= memcached_pool_pop(pool, false, &rc);
    memcached_pool_push(pool, memc);
    memcached_pool_destroy(pool);
*/
//set non_block
    rc = memcached_behavior_set(memc,MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
    if (rc != MEMCACHED_SUCCESS)
    {
        printf("[%s]Error:memcached_behavior_set MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED fail", __FUNCTION__);
        return -1;
    }  
    rc = memcached_behavior_set(memc,MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
    if (rc != MEMCACHED_SUCCESS)
    {
        printf("[%s]Error:memcached_behavior_set MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED fail", __FUNCTION__);
        return -1;
    }     
    
    
    //Set first value to the Server
    char *key= "hello";
    char *value= "kit";
    SetKey(memc,key,strlen(key),value,strlen(value));
    
    //Set second value to the Server
    key= "foo";
    value= "value";
    SetKey(memc,key,strlen(key),value,strlen(value));
    
    //Set third value
    key= "rank";
    value= "0";
    SetKey(memc,key,strlen(key),value,strlen(value));
    
    //get every single key value
    char *keys[]= {"hello", "foo", "rank"};
    size_t key_length[]= {5, 3, 4};

    char* return_value;
    unsigned int flags;
    size_t value_length;
    for(int i=0;i<3;i++)
    {
        if(return_value=memcached_get(memc,keys[i], strlen(keys[i]),&value_length,&flags,&rc))
        {
            printf("memcached get <%s,%s>\n",keys[i],return_value);
            free(return_value);
        }
    }
    
    //atomic overwrite,failed
    //memcached_result_st* result;
    //result=memcached_result_create(memc, NULL);
    //if(!result)
        //printf("create result_struct failed\n");
    //uint64_t cas = memcached_result_cas(result);
    //memcached_result_free(result);
    uint64_t cas = memc->result.item_cas;
    key= "rank";
    value= "2";
    rc=memcached_cas(memc, key,strlen(key),value,strlen(value),
                      (time_t) 0, (uint32_t) 0, cas);
    if(rc!= MEMCACHED_SUCCESS)
    {
         printf("memcached_cas overwrite <%s,%s> failed\n",key,value);
    }
    else
    {
        printf("memcached_cas overwrite  <%s,%s> success\n",key,value);
    }
    
    
    //Fetching multiple values
    fetchmultikeys(memc,keys,key_length);

    //delete a value
    if(memcached_delete(memc, keys[0], (size_t)5, (time_t)0) != MEMCACHED_SUCCESS)
    {
        printf("memcached delete key:<%s> failed\n",keys[0]);
    }
    else
    {
        printf("memcached delete key:<%s> success\n",keys[0]);
    }

    //prepend key:foo with pre,failed?,expection:pre_value
    //OPT_COMPRESSION
    rc=memcached_prepend(memc, keys[1],strlen(keys[1]), "pre", 3, (time_t) 0, (uint32_t) 0);
    if(rc != MEMCACHED_SUCCESS)
    {
        printf("memcached prehend <%s> with value:%s failed\n",memcached_strerror(memc,rc),"pre");
    }
    else
    {
        printf("memcached prehend <%s> with value:%s success\n",keys[1],"pre");
    }
    //Fetching multiple values after prehend
    fetchmultikeys(memc,keys,key_length);

    //increment key:<rank>'s value by offset,eg:valuse+=6
    unsigned int offset = 6;
    uint64_t in_value =64;
    rc = memcached_increment(memc, keys[2], strlen(keys[2]),offset, &in_value);
    if(rc != MEMCACHED_SUCCESS)
    {
        printf("memcached increment key:<%s> by offset:%d failed\n",keys[2],offset);
    }
    else
    {
        printf("memcached increment key:<%s> by offset:%d\n",keys[2],offset);
    }
    //memcached_return_t memcached_decrement(memcached_st *ptr, const char *key, 
    //                                       size_t key_length, uint32_t offset, uint64_t *value)
    if(return_value=memcached_get(memc,keys[2], strlen(keys[2]),&value_length,&flags,&rc))
    {
        printf("memcached get <%s,%s>\n",keys[2],return_value);
        free(return_value);
    }
    memcached_free(memc);
}


void fetchmultikeys(memcached_st *memc,char**keys,size_t* key_length)
{
    memcached_return_t rc;
    unsigned int x;
    uint32_t flags;
    char return_key[MEMCACHED_MAX_KEY];
    size_t return_key_length;
    char *return_value;
    size_t return_value_length;

    if((rc= memcached_mget(memc, keys, key_length, 3))==MEMCACHED_SUCCESS)
    {
        printf("memcached get mutiple key<%s,%s,%s> success\n",keys[0],keys[1],keys[2]);
    }
    else
    {
        printf("memcached get mutiple key<%s,%s,%s> failed\n",keys[0],keys[1],keys[2]);
    }
    x= 0;
    while ((return_value= memcached_fetch(memc, return_key, &return_key_length,
                                          &return_value_length, &flags, &rc)))
    {
        return_key[return_key_length]=0;
        printf("Muti-key fetch return:<%s,%s>\n",return_key,return_value);
        free(return_value);
        x++;
    }
}

void SetKey(memcached_st *memc, const char* key, size_t key_length, const char*value, size_t value_length)
{
    //If an object already exists it will overwrite what is in the server. 
    //If the object does not exist it will be written.
    memcached_return_t rc= memcached_set(memc, key, key_length, value, 
                                            value_length, (time_t)0, (uint32_t)0);
                                            
    // If the object is found on the server an error occurs, otherwise the value is stored                                       
    //memcached_return_t rc= memcached_add(memc, key, key_length, 
    //                                        value, value_length, (time_t)0, (uint32_t)0);
    if (rc != MEMCACHED_SUCCESS)
    {
        printf("Memcached set <%s:%s>failed\n",key,value);
    }
    else
    {
        printf("Memcached set <%s:%s>\n",key,value);
    }
}
    
