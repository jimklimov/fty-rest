/*
Copyright (C) 2014 Eaton
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*! \file NetHistory.h
    \brief A class for manipulating with the database objects stored 
    in the table t_bios_net_history.

    \author Alena Chernikava <alenachernikava@eaton.com>
*/  
#ifndef NETHISTORY_H_
#define NETHISTORY_H_

#include <string>
#include <ctime>
#include <tntdb/connection.h>
#include <tntdb/connect.h>
#include "cidr.h"
#include "databasetimeobject.h"

namespace utils {

/*
 * \brief NetHistory is a class for representing a database entity
 * t_bios_device_type.
 */
class NetHistory : public DataBaseTimeObject
{
    public:
         
        /**
         * \brief Creates a new object and specifies a connection.
         *
         * Creates a new object for the specified url in state OS_NEW.
         *
         * \param url - a connection to the database.
         */
        NetHistory(std::string url);

        /**
         * \brief Returns an object to OS_NEW state with initial parameters.
         *
         * Returns an object to initial state. It doesn't manipulate with database.
         */
        void clear();

        /**
         * \brief Returns all fields as string.
         */
        std::string toString();

        ~NetHistory();

        /**
         * \brief Sets a new mac address for the object.
         *
         * If state is OS_DELETED then do nothing.
         * If newmac = oldmac then do nothing.
         * If state is OS_SELECTED/OS_INSERTED and newmac != oldmac.
         * than state is changed to OS_UPDATED.
         *
         * \param name - new name of the device type.
         */    
        void setMac(std::string mac);

        void setAddres(CIDRAddress address);

        void setCommand(char command);

        void setName(std::string name);

        std::string getMac();

        int getMask();

        std::string getIp();

        char getCommand();

        std::string getName();

        CIDRAddress getAddress();

        void setAddress(CIDRAddress address);

        unsigned int selectById(int id);

        /**
         * \brief Selects ids of the rows matching the template (Address(IP/mask),command)
         *
         * Don't modify the object.
         *
         * \return A vector of ids found.
         */
        std::vector<int> checkUnique();
        
        
    protected:
       
        bool check_command();
         
        void clear_this();

        /**
         * \brief Checks the name length, command
         *
         * TODO add more checks if needed
         */
        bool check();

        /**
         *  \brief inserts a row.
         *
         *  All necessary pre-actions are made in dbsave
         */
        unsigned int db_insert();
        
        /**
         *  \brief updates a row.
         *
         *  All necessary pre-actions are made in dbsave
         */
        unsigned int db_update();
        
        /**
         *  \brief deletes a row.
         *
         *  All necessary pre-actions are made in dbdelete
         */
        unsigned int db_delete();
        
        /**
         * \brief Selects a timestamp for stored ID.
         *
         * Internal method.
         */
        unsigned int db_select_timestamp();

    private:
        
        std::string _mac;

        CIDRAddress _address;

        char _command;

        std::string _name;

}; // end of the class

}  // end of namespace utils

#endif // NETHISTORY_H_
