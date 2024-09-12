/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DATABASEWORKERPOOL_H
#define _DATABASEWORKERPOOL_H

#include "Define.h"
#include "DatabaseEnvFwd.h"
#include "StringFormat.h"
#include <array>
#include <string>
#include <vector>

template <typename T>
class ProducerConsumerQueue;

class SQLOperation;
struct MySQLConnectionInfo;

template <class T>
class DatabaseWorkerPool
{
    private:
        enum InternalIndex
        {
            IDX_ASYNC, // 异步
            IDX_SYNCH, // 同步
            IDX_SIZE
        };

    public:
        /* Activity state */
        DatabaseWorkerPool();

        ~DatabaseWorkerPool();

        // 设置数据库连接信息，包括数据库连接字符串和异步、同步线程的数量
        void SetConnectionInfo(std::string const& infoString, uint8 const asyncThreads, uint8 const synchThreads);

        uint32 Open(); // 打开连接池中的数据库连接

        void Close(); // 关闭连接

        //! Prepares all prepared statements
        bool PrepareStatements(); // 预处理

        // 获取数据库连接的详细信息
        inline MySQLConnectionInfo const* GetConnectionInfo() const
        {
            return _connectionInfo.get();
        }

        /**
            Delayed one-way statement methods. 异步操作
        */

        //! Enqueues a one-way SQL operation in string format that will be executed asynchronously.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        void Execute(char const* sql); // 将一个 SQL 字符串任务加入异步队列，交由异步线程执行

        //! Enqueues a one-way SQL operation in string format -with variable args- that will be executed asynchronously.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        template<typename Format, typename... Args>
        void PExecute(Format&& sql, Args&&... args) // 格式化 SQL 字符串并将其加入异步队列。这种格式化方式允许动态生成 SQL 语句
        {
            if (Trinity::IsFormatEmptyOrNull(sql))
                return;

            Execute(Trinity::StringFormat(std::forward<Format>(sql), std::forward<Args>(args)...).c_str());
        }

        //! Enqueues a one-way SQL operation in prepared statement format that will be executed asynchronously.
        //! Statement must be prepared with CONNECTION_ASYNC flag.
        void Execute(PreparedStatement<T>* stmt); // 将预处理好的 SQL 语句加入异步队列执行

        /**
            Direct synchronous one-way statement methods. 同步操作
        */

        //! Directly executes a one-way SQL operation in string format, that will block the calling thread until finished.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        void DirectExecute(char const* sql); // 直接执行一个 SQL 操作，调用线程会被阻塞直到查询完成

        //! Directly executes a one-way SQL operation in string format -with variable args-, that will block the calling thread until finished.
        //! This method should only be used for queries that are only executed once, e.g during startup.
        template<typename Format, typename... Args>
        void DirectPExecute(Format&& sql, Args&&... args) // 格式化并直接执行 SQL 语句，调用线程会被阻塞
        {
            if (Trinity::IsFormatEmptyOrNull(sql))
                return;

            DirectExecute(Trinity::StringFormat(std::forward<Format>(sql), std::forward<Args>(args)...).c_str());
        }

        //! Directly executes a one-way SQL operation in prepared statement format, that will block the calling thread until finished.
        //! Statement must be prepared with the CONNECTION_SYNCH flag.
        void DirectExecute(PreparedStatement<T>* stmt); // 直接执行一个预处理好的 SQL 操作，阻塞调用线程

        /**
            Synchronous query (with resultset) methods. 同步查询
        */

        //! Directly executes an SQL query in string format that will block the calling thread until finished.
        //! Returns reference counted auto pointer, no need for manual memory management in upper level code.
        QueryResult Query(char const* sql, T* connection = nullptr); // 执行一个 SQL 查询，返回查询结果，并且阻塞线程直到结果返回

        //! Directly executes an SQL query in string format -with variable args- that will block the calling thread until finished.
        //! Returns reference counted auto pointer, no need for manual memory management in upper level code.
        template<typename Format, typename... Args>
        QueryResult PQuery(Format&& sql, T* conn, Args&&... args) // 格式化并执行一个 SQL 查询，阻塞线程并返回查询结果
        {
            if (Trinity::IsFormatEmptyOrNull(sql))
                return QueryResult(nullptr);

            return Query(Trinity::StringFormat(std::forward<Format>(sql), std::forward<Args>(args)...).c_str(), conn);
        }

        //! Directly executes an SQL query in string format -with variable args- that will block the calling thread until finished.
        //! Returns reference counted auto pointer, no need for manual memory management in upper level code.
        template<typename Format, typename... Args>
        QueryResult PQuery(Format&& sql, Args&&... args)
        {
            if (Trinity::IsFormatEmptyOrNull(sql))
                return QueryResult(nullptr);

            return Query(Trinity::StringFormat(std::forward<Format>(sql), std::forward<Args>(args)...).c_str());
        }

        //! Directly executes an SQL query in prepared format that will block the calling thread until finished.
        //! Returns reference counted auto pointer, no need for manual memory management in upper level code.
        //! Statement must be prepared with CONNECTION_SYNCH flag.
        PreparedQueryResult Query(PreparedStatement<T>* stmt);

        /**
            Asynchronous query (with resultset) methods. 异步查询
        */

        //! Enqueues a query in string format that will set the value of the QueryResultFuture return object as soon as the query is executed.
        //! The return value is then processed in ProcessQueryCallback methods.
        QueryCallback AsyncQuery(char const* sql); // 将一个查询加入异步队列，查询结果通过回调函数处理

        //! Enqueues a query in prepared format that will set the value of the PreparedQueryResultFuture return object as soon as the query is executed.
        //! The return value is then processed in ProcessQueryCallback methods.
        //! Statement must be prepared with CONNECTION_ASYNC flag.
        QueryCallback AsyncQuery(PreparedStatement<T>* stmt); // 将一个预处理的查询加入异步队列，查询结果通过回调处理

        //! Enqueues a vector of SQL operations (can be both adhoc and prepared) that will set the value of the QueryResultHolderFuture
        //! return object as soon as the query is executed.
        //! The return value is then processed in ProcessQueryCallback methods.
        //! Any prepared statements added to this holder need to be prepared with the CONNECTION_ASYNC flag.
        SQLQueryHolderCallback DelayQueryHolder(std::shared_ptr<SQLQueryHolder<T>> holder);

        /**
            Transaction context methods.
        */

        //! Begins an automanaged transaction pointer that will automatically rollback if not commited. (Autocommit=0)
        SQLTransaction<T> BeginTransaction(); // 开始一个数据库事务，事务会在未提交时自动回滚

        //! Enqueues a collection of one-way SQL operations (can be both adhoc and prepared). The order in which these operations
        //! were appended to the transaction will be respected during execution.
        void CommitTransaction(SQLTransaction<T> transaction); // 提交一个事务，确保按顺序执行 SQL 操作

        //! Enqueues a collection of one-way SQL operations (can be both adhoc and prepared). The order in which these operations
        //! were appended to the transaction will be respected during execution.
        TransactionCallback AsyncCommitTransaction(SQLTransaction<T> transaction); // 将事务提交至异步队列执行

        //! Directly executes a collection of one-way SQL operations (can be both adhoc and prepared). The order in which these operations
        //! were appended to the transaction will be respected during execution.
        void DirectCommitTransaction(SQLTransaction<T>& transaction);

        //! Method used to execute ad-hoc statements in a diverse context.
        //! Will be wrapped in a transaction if valid object is present, otherwise executed standalone.
        void ExecuteOrAppend(SQLTransaction<T>& trans, char const* sql);

        //! Method used to execute prepared statements in a diverse context.
        //! Will be wrapped in a transaction if valid object is present, otherwise executed standalone.
        void ExecuteOrAppend(SQLTransaction<T>& trans, PreparedStatement<T>* stmt);

        /**
            Other
        */

        typedef typename T::Statements PreparedStatementIndex;

        //! Automanaged (internally) pointer to a prepared statement object for usage in upper level code.
        //! Pointer is deleted in this->DirectExecute(PreparedStatement*), this->Query(PreparedStatement*) or PreparedStatementTask::~PreparedStatementTask.
        //! This object is not tied to the prepared statement on the MySQL context yet until execution.
        PreparedStatement<T>* GetPreparedStatement(PreparedStatementIndex index);

        //! Apply escape string'ing for current collation. (utf8)
        void EscapeString(std::string& str); // 处理字符串的转义字符，防止 SQL 注入问题

        //! Keeps all our MySQL connections alive, prevent the server from disconnecting us.
        void KeepAlive(); // 保持数据库连接活跃，防止服务器断开连接

        void WarnAboutSyncQueries([[maybe_unused]] bool warn)
        {
#ifdef TRINITY_DEBUG
            _warnSyncQueries = warn;
#endif
        }

        size_t QueueSize() const;

    private:
        uint32 OpenConnections(InternalIndex type, uint8 numConnections);

        unsigned long EscapeString(char* to, char const* from, unsigned long length);

        void Enqueue(SQLOperation* op);

        //! Gets a free connection in the synchronous connection pool.
        //! Caller MUST call t->Unlock() after touching the MySQL context to prevent deadlocks.
        T* GetFreeConnection(); // 获取一个空闲的同步数据库连接，确保连接资源的合理利用

        char const* GetDatabaseName() const;

        //! Queue shared by async worker threads.
        std::unique_ptr<ProducerConsumerQueue<SQLOperation*>> _queue; // 异步 SQL 操作的队列，使用生产者-消费者模型管理异步任务
        std::array<std::vector<std::unique_ptr<T>>, IDX_SIZE> _connections;
        std::unique_ptr<MySQLConnectionInfo> _connectionInfo;
        std::vector<uint8> _preparedStatementSize;
        uint8 _async_threads, _synch_threads;
#ifdef TRINITY_DEBUG
        static inline thread_local bool _warnSyncQueries = false;
#endif
};

#endif
