
/*
 *
    File:       mysql.cpp

    Contains:  excute the sql inqure string(select,insert,delete)
                
*/

#include "DSErrorLog.h"
#include <stdio.h>  
#include <stdlib.h>  
#include <syslog.h>  
#include <string.h> 
#include "DSSQL.h"

#define mysql_debug 0

sql::~sql() {mysql_end();}
int sql::mysql_connect(char*user,char* passwd,char*host,char* db)
{
    if(!mysql_real_connect(&my_connection, host, user, passwd, db, 0, NULL, CLIENT_MULTI_STATEMENTS))
    {   
        LogError(DebugVerbosity,"%s,user:%s,host:%s,dbname:%s",mysql_error(&my_connection),user,host,db);
        return -1;
    } 
    else 
    {
        return 0;
    }
}
 

MYSQL_ROW* sql::mysql_excu(char* inquire)
{
    if(rows)
    {
        delete []rows;
        rows = NULL;
    }
    if(my_res)
    {
        mysql_free_result(my_res);
        my_res = NULL;
    }
    int res;
    res = mysql_query(&my_connection,inquire);
    if (!res)
    {  
        my_res = mysql_store_result(&my_connection);
        if(my_res)
        {
            num_column = mysql_num_fields(my_res);
            num_rows = mysql_num_rows(my_res);
            rows = new MYSQL_ROW [num_rows];
            for(int i=0;i < num_rows;i++)
            {
                rows[i] = mysql_fetch_row(my_res);
                if(!rows[i])
                    break;
                //lengths = mysql_fetch_lengths(my_res);
            }
            fields = mysql_fetch_fields(my_res);
        }
        return rows;
    }
    else
    {
        LogError(DebugVerbosity,"fail to excute mysql inquire, error:%s", mysql_error(&my_connection));
        return NULL;
    }
            
}

void sql::mysql_end()
{
    if(rows)
    {
        delete []rows;
        rows = NULL;
    }
    if(my_res)
    {
        mysql_free_result(my_res);
        my_res = NULL;
    }
    mysql_close(&my_connection);
}


 //print results 
#if mysql_debug

static sql visit;
void sql::res_print(sql* visit)
{
    int i,j; 
    if(visit->fields) 
    {
        for (i = 0; i < visit->num_column; i++)  
        {   
            printf("%s", visit->fields[i].name);
            printf("\t");  
        } 
        printf("\n");
    }
    if(visit->rows)
    {
        for (j = 0; j < visit->num_rows; j++ )  //........................
        {  

            for (i = 0; i < visit->num_column; i++){  
                printf("%s", visit->rows[j][i] ? visit->rows[j][i] : "NULL");
                printf("\t");  
            }  
            printf("\n");  
        }
    } 
}

void testcore(char* user,char* password,char* ip,char* db)
{
    if(visit.mysql_connect(user,password,ip,db)<0)
        return;
    MYSQL_ROW* data;
    if(data=visit.mysql_excu(argv[1]))
    {
        visit.res_print(&visit);
    }
}

int main(int argc, char * argv[])  
{  
    if(argc != 2)
    {
        printf("please input inquire string\n");
        return -1;
    }
    testcore("root", "adv", "127.0.0.1","test");
    return EXIT_SUCCESS;  
}  

#endif 
