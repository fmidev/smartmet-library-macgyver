#include "PostgreSQLConnection.h"

void test1(Fmi::Database::PostgreSQLConnection& conn)
{
    conn.prepare("test", "SELECT * FROM foo WHERE x=$1");
    Fmi::Database::PostgreSQLConnection::PreparedSQL sql(
        conn, "test",  "SELECT * FROM foo WHERE bar=$1 and baz=$2");
    sql.exec(42, "something");
}

void test2(Fmi::Database::PostgreSQLConnection& conn)
{
    conn.exec_params("SELECT * FROM foo WHERE x=$1", 42);
}
