#include "PostgreSQLConnection.h"

void test1(Fmi::Database::PostgreSQLConnection& conn)
{
    conn.prepare("test", "SELECT * FROM foo WHERE x=$1");
    Fmi::Database::PostgreSQLConnection::Transaction work(conn);
    work.executePrepared("test", 42);
}
