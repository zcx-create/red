
#include "Log.h"

#include "DatabaseEnv.h"
#include "DatabaseLoader.h"
#include "Implementation/SakilaDatabase.h"
#include "MySQLThreading.h"

int main()
{
    MySQL::Library_Init();

    DatabaseLoader loader;
    loader.AddDatabase(SakilaDatabase, "192.168.232.1;3306;root;123456;sakila",
        8, 2);
    
    if (!loader.Load()) {
        TC_LOG_ERROR("", "SakilaDatabase connect error");
        return 1;
    }
    TC_LOG_INFO("", "SakilaDatabase connect success");
#if 0
    {
        // SakilaDatabase.DirectExecute("insert into actor(first_name, last_name) values ('mark', '0voice')");
        auto result = SakilaDatabase.Query("select actor_id, first_name, last_name, last_update from actor where actor_id = 201");
        if (!result) {
            TC_LOG_ERROR("", "select empty");
            return 1;
        }
        TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
            (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
    }

    {
        auto *stmt = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO);
        stmt->setUInt8(0, 1);
        auto result = SakilaDatabase.Query(stmt);
        TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
            (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
    }

    {
        auto result = SakilaDatabase.Query("select actor_id, first_name, last_name, last_update from actor where actor_id >= 196 and actor_id <= 201");
        if (result) {
            do {
                Field *field = result->Fetch();

                TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                    field[0].GetUInt8(), field[1].GetString(), field[2].GetString(),field[3].GetString());
            } while (result->NextRow());
        }
    }
#endif

#if 1
    std::thread thrd1([]() {
        auto result = SakilaDatabase.Query("select actor_id, first_name, last_name, last_update from actor where actor_id = 201");
        if (!result) {
            TC_LOG_ERROR("", "select empty");
            return;
        }
        TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
            (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
    });
    std::thread thrd2([]() {
        auto *stmt = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO);
        stmt->setUInt8(0, 202);
        auto result = SakilaDatabase.Query(stmt);
        TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
            (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
    });
    std::thread thrd3([]() {
        auto *stmt = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO);
        stmt->setUInt8(0, 203);
        auto result = SakilaDatabase.Query(stmt);
        TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
            (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
    });

    thrd1.join();
    thrd2.join();
    thrd3.join();
#endif
    SakilaDatabase.Close();
    MySQL::Library_End();
    return 0;
}