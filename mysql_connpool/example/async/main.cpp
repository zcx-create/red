
#include "DatabaseEnvFwd.h"
#include "Log.h"

#include "DatabaseEnv.h"
#include "DatabaseLoader.h"
#include "Implementation/SakilaDatabase.h"
#include "MySQLThreading.h"
#include "AsyncCallbackProcessor.h"
#include "QueryHolder.h"
#include <chrono>
#include <memory>
#include <thread>

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

    // {
    //     // SakilaDatabase.DirectExecute("insert into actor(first_name, last_name) values ('mark', '0voice')");
    //     auto result = SakilaDatabase.Query("select actor_id, first_name, last_name, last_update from actor where actor_id = 201");
    //     if (!result) {
    //         TC_LOG_ERROR("", "select empty");
    //         return 1;
    //     }
    //     TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
    //         (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
    // }
#if 0
    {
        auto *stmt = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO_ASYNC);
        stmt->setUInt8(0, 1);

        AsyncCallbackProcessor<QueryCallback> processor;

        processor.AddCallback(SakilaDatabase.AsyncQuery(stmt).WithPreparedCallback([](PreparedQueryResult result) {
            TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
        }));

        auto *stmt2 = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO_ASYNC);
        stmt2->setUInt8(0, 2);
        processor.AddCallback(SakilaDatabase.AsyncQuery(stmt2).WithPreparedCallback([](PreparedQueryResult result) {
            TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
        }));

        auto *stmt3 = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO_ASYNC);
        stmt3->setUInt8(0, 3);
        processor.AddCallback(SakilaDatabase.AsyncQuery(stmt3).WithPreparedCallback([](PreparedQueryResult result) {
            TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
        }));

        while (true) {
            processor.ProcessReadyCallbacks();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
#endif
#if 0
    {
        auto *stmt1 = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO_ASYNC);
        stmt1->setUInt8(0, 1);

        AsyncCallbackProcessor<QueryCallback> processor;
        processor.AddCallback(SakilaDatabase.AsyncQuery(stmt1)
            .WithChainingPreparedCallback([](QueryCallback &callback, PreparedQueryResult result){
                TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                    (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
                auto *stmt2 = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO_ASYNC);
                stmt2->setUInt8(0, 2);
                callback.SetNextQuery(SakilaDatabase.AsyncQuery(stmt2));
        })
            .WithChainingPreparedCallback([](QueryCallback &callback, PreparedQueryResult result){
                TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                    (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
                // auto *stmt3 = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO_ASYNC);
                // stmt3->setUInt8(0, 3);
                // callback.SetNextQuery(SakilaDatabase.AsyncQuery(stmt3));
        })
            .WithChainingPreparedCallback([](QueryCallback &callback, PreparedQueryResult result){
                TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                    (*result)[0].GetUInt8(), (*result)[1].GetString(), (*result)[2].GetString(),(*result)[3].GetString());
        }));

        while (true) {
            processor.ProcessReadyCallbacks();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
#endif
#if 0 
    {
        // 异步获取多个sql 的结果  delayholder
        class ActorInfoHolder : public SQLQueryHolder<SakilaDatabaseConnection>
        {
        public:
            enum ACTOR_INFO_IDX : uint8 {
                AI_1,
                AI_3,
                AI_5,
                AI_MAX
            };
        public:
            ActorInfoHolder() {
                SetSize(AI_MAX);
                auto *stmt1 = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO_ASYNC);
                stmt1->setUInt8(0, 1);
                SetPreparedQuery(AI_1, stmt1);

                auto *stmt3 = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO_ASYNC);
                stmt3->setUInt8(0, 3);
                SetPreparedQuery(AI_3, stmt3);

                auto *stmt5 = SakilaDatabase.GetPreparedStatement(SAKILA_SEL_ACTOR_INFO_ASYNC);
                stmt5->setUInt8(0, 5);
                SetPreparedQuery(AI_5, stmt5);
            }
        };

        AsyncCallbackProcessor<SQLQueryHolderCallback> processor;
        auto holder = std::make_shared<ActorInfoHolder>();
        processor.AddCallback(SakilaDatabase.DelayQueryHolder(holder)).AfterComplete([](const SQLQueryHolderBase &hdr){
            auto holder = static_cast<const ActorInfoHolder&>(hdr);
            auto result1 = holder.GetPreparedResult(ActorInfoHolder::AI_1);
            auto result3 = holder.GetPreparedResult(ActorInfoHolder::AI_3);
            auto result5 = holder.GetPreparedResult(ActorInfoHolder::AI_5);
            
            TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                (*result1)[0].GetUInt8(), (*result1)[1].GetString(), (*result1)[2].GetString(),(*result1)[3].GetString());

            TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                (*result3)[0].GetUInt8(), (*result3)[1].GetString(), (*result3)[2].GetString(),(*result3)[3].GetString());

            TC_LOG_INFO("", "actor_id=%u,first_name=%s,last_name=%s,last_update=%s",
                (*result5)[0].GetUInt8(), (*result5)[1].GetString(), (*result5)[2].GetString(),(*result5)[3].GetString());
             
        });

        while (true) {
            processor.ProcessReadyCallbacks();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
#endif
#if 0
    {
        // 事务处理只针对写操作，拿不到操作的结果
        auto trans = SakilaDatabase.BeginTransaction();
        
        trans->Append("insert into actor(first_name, last_name) values ('king', '0voice')");
        trans->Append("insert into actor(first_name, last_name) values ('darren', '0voice')");
        trans->Append("insert into country(country_id, country) values (100, 'china')");
        trans->Append("insert into city(city, country_id) values ('chang sha', '100')");

        AsyncCallbackProcessor<TransactionCallback> processor;
        processor.AddCallback(SakilaDatabase.AsyncCommitTransaction(trans))
            .AfterComplete([](bool success) {
                if (success) {
                    TC_LOG_INFO("", "transaction commit success");
                } else {
                    TC_LOG_INFO("", "transaction commit failed");
                }
            });


        while (true) {
            processor.ProcessReadyCallbacks();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
#endif
    SakilaDatabase.Close();
    MySQL::Library_End();
    return 0;
}