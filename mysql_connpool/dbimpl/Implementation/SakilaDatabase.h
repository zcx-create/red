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

#ifndef _SAKILADATABASE_H
#define _SAKILADATABASE_H

#include "MySQLConnection.h"

enum SakilaDatabaseStatements : uint32
{
    /*  Naming standard for defines:
        {DB}_{SEL/INS/UPD/DEL/REP}_{Summary of data changed}
        When updating more than one field, consider looking at the calling function
        name for a suiting suffix.
    */
    SAKILA_SEL_ACTOR_INFO,
    SAKILA_SEL_ACTOR_INFO_ASYNC,
    MAX_SAKILADATABASE_STATEMENTS
};

class TC_DATABASE_API SakilaDatabaseConnection : public MySQLConnection
{
public:
    typedef SakilaDatabaseStatements Statements;

    //- Constructors for sync and async connections
    SakilaDatabaseConnection(MySQLConnectionInfo& connInfo);
    SakilaDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* q, MySQLConnectionInfo& connInfo);
    ~SakilaDatabaseConnection();

    //- Loads database type specific prepared statements
    void DoPrepareStatements() override;
};

#endif
