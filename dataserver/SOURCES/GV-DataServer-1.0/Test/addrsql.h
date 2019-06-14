#ifndef __ADDSQL_H__
#define __ADDSQL_H__


#include "mysql.h"

class sql
{
public:
    sql():fields(NULL),num_column(0),num_rows(0),rows(NULL),my_res(NULL)
    {
        mysql_init(&my_connection);
    }
    ~sql();   
    
    MYSQL_ROW*   mysql_excu(char* inquire);
    void        res_print(sql* visit);
    int         mysql_connect(char*user,char* passwd,char*host,char* db);
    int         set_connect_timeout(int timeout);
    int         set_reconnect(my_bool judge);
public:
    MYSQL_FIELD* fields;
    int          num_column ;
    int          num_rows;
    MYSQL_ROW*   rows ;
    
private:
    MYSQL        my_connection;
    MYSQL_RES*   my_res;
    
protected:
    void        mysql_end();
};
#endif
