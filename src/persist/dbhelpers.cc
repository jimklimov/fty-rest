/*
Copyright (C) 2015 Eaton

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "dbhelpers.h"

#include <assert.h>

#include <czmq.h>
#include <tntdb/connect.h>
#include <tntdb/row.h>
#include <tntdb/error.h>

#include <fty_common_db_dbpath.h>
#include <fty_log.h>
#include "persist_error.h"
#include "shared/asset_types.h"

int convert_asset_to_monitor_safe(const char* url,
                a_elmnt_id_t asset_element_id, m_dvc_id_t *device_id)
{
    if ( device_id == NULL )
        return -5;
    try
    {
        *device_id = convert_asset_to_monitor_old(url, asset_element_id);
        return 0;
    }
    catch (const bios::NotFound &e){
        return -1;
    }
    catch (const bios::InternalDBError &e) {
        return -2;
    }
    catch (const bios::ElementIsNotDevice &e) {
        return -3;
    }
    catch (const bios::MonitorCounterpartNotFound &e) {
        return -4;
    }
}


m_dvc_id_t convert_asset_to_monitor_old(const char* url,
                a_elmnt_id_t asset_element_id)
{
    assert ( asset_element_id );
    m_dvc_id_t       device_discovered_id = 0;
    a_elmnt_tp_id_t  element_type_id      = 0;
    try{
        tntdb::Connection conn = tntdb::connectCached(url);

        tntdb::Statement st = conn.prepareCached(
            " SELECT"
            "   v.id_discovered_device, v1.id_type"
            " FROM"
            "   v_bios_monitor_asset_relation v"
            " LEFT JOIN t_bios_asset_element v1"
            "   ON (v1.id_asset_element = v.id_asset_element)"
            " WHERE v.id_asset_element = :id"
        );

        tntdb::Row row = st.set("id", asset_element_id).
                            selectRow();

        row[0].get(device_discovered_id);

        row[1].get(element_type_id);
        assert (element_type_id);
    }
    catch (const tntdb::NotFound &e){
        // apropriate asset element was not found
        log_info("end: asset element %" PRIu32 " notfound", asset_element_id);
        throw bios::NotFound();
    }
    catch (const std::exception &e) {
        log_warning("end: abnormal with '%s'", e.what());
        throw bios::InternalDBError(e.what());
    }
    if ( element_type_id != persist::asset_type::DEVICE )
    {
        log_info("end: specified element is not a device");
        throw bios::ElementIsNotDevice();
    }
    else if ( device_discovered_id == 0 )
    {
        log_warning("end: monitor counterpart for the %" PRIu32 " was not found",
                                                asset_element_id);
        throw bios::MonitorCounterpartNotFound ();
    }
    log_info("end: asset element %" PRIu32 " converted to %" PRIu32, asset_element_id, device_discovered_id);
    return device_discovered_id;
}

int convert_monitor_to_asset_safe(const char* url,
                    m_dvc_id_t discovered_device_id, a_elmnt_id_t *asset_element_id)
{
    if ( asset_element_id == NULL )
        return -5;
    try
    {
        *asset_element_id = convert_monitor_to_asset (url, discovered_device_id);
        return 0;
    }
    catch (const bios::NotFound &e){
        return -1;
    }
    catch (const bios::InternalDBError &e) {
        return -2;
    }
}


a_elmnt_id_t convert_monitor_to_asset(const char* url,
                    m_dvc_id_t discovered_device_id)
{
    log_info("start");
    assert ( discovered_device_id );
    a_elmnt_id_t asset_element_id = 0;
    try{
        tntdb::Connection conn = tntdb::connectCached(url);
        tntdb::Statement st = conn.prepareCached(
            " SELECT"
            " id_asset_element"
            " FROM"
            " v_bios_monitor_asset_relation"
            " WHERE id_discovered_device = :id"
        );

        tntdb::Value val = st.set("id", discovered_device_id).
                              selectValue();

        val.get(asset_element_id);
    }
    catch (const tntdb::NotFound &e){
        // apropriate asset element was not found
        log_warning("end: asset counterpart for the %" PRIu32 " was not found",
                                                discovered_device_id);
        throw bios::NotFound();
    }
    catch (const std::exception &e) {
        log_warning("end: abnormal with '%s'", e.what());
        throw bios::InternalDBError(e.what());
    }
    log_info("end: monitor device %" PRIu32 " converted to %" PRIu32, discovered_device_id, asset_element_id);
    return asset_element_id;
}

bool is_ok_element_type (a_elmnt_tp_id_t element_type_id)
{
    switch(element_type_id) {
        case persist::asset_type::DATACENTER:
        case persist::asset_type::ROOM:
        case persist::asset_type::ROW:
        case persist::asset_type::RACK:
        case persist::asset_type::GROUP:
        case persist::asset_type::DEVICE:
            return true;
        default:
            return false;
    }
}

bool is_ok_name (const char* name)
{
    size_t length = strlen (name);
    if ( length == 0)
        return false;

    // Bad characters _ % @
    if (strchr (name, '_') != NULL ||
        strchr (name, '%') != NULL ||
        strchr (name, '@') != NULL)
       return false;

    return true;
}

bool is_ok_keytag (const char *keytag)
{
    auto length = strlen(keytag);
    if ( ( length > 0 ) && ( length <= MAX_KEYTAG_LENGTH ) )
        return true;
    else
        return false;
}

bool is_ok_value (const char *value)
{
    auto length = strlen(value);
    if ( ( length > 0 ) && ( length <= MAX_VALUE_LENGTH ) )
        return true;
    else
        return false;
}

bool is_ok_link_type (a_lnk_tp_id_t link_type_id)
{
    // TODO: manage link types
    if ( link_type_id > 0 )
        return true;
    else
        return false;
}


std::string
sql_plac(
        size_t i,
        size_t j)
{
    return "item" + std::to_string(i) + "_" + std::to_string(j);
}

// for backward compatibility
std::string
    multi_insert_string(
        const std::string& sql_header,
        size_t tuple_len,
        size_t items_len
)
{
    return multi_insert_string(sql_header,tuple_len,items_len, "");
}


std::string
    multi_insert_string(
        const std::string& sql_header,
        size_t tuple_len,
        size_t items_len,
        const std::string& sql_postfix
)
{
    std::stringstream s{};

    s << sql_header;
    s << "\nVALUES ";
    for (size_t i = 0; i != items_len; i++) {
        s << "(";
        for (size_t j = 0; j != tuple_len; j++) {
            s << ":" << sql_plac(i, j);
            if (j < tuple_len -1)
                s << ", ";
        }
        if (i < items_len -1)
            s << "),\n";
        else
            s << ")\n";
    }
    s << sql_postfix;
    return s.str();
}


int
get_active_power_devices ()
{
    int count = 0;
    try
    {
        tntdb::Connection conn = tntdb::connectCached (DBConn::url);
        tntdb::Statement st = conn.prepareCached (
            "SELECT COUNT(*) AS CNT FROM t_bios_asset_element "
            "WHERE id_subtype IN "
                "(SELECT id_asset_device_type FROM t_bios_asset_device_type "
                "WHERE name IN ('epdu', 'sts', 'ups', 'pdu', 'genset')) "
            "AND status = 'active';"
        );

        tntdb::Row row = st.selectRow ();
        zsys_debug ("[get_active_power_devices]: were selected %" PRIu32 " rows", 1);

        row [0].get (count);
    }
    catch (const std::exception &e)
    {
        zsys_error ("exception caught %s when getting count of active power devices", e.what ());
        return 0;
    }

    return count;
}


std::string get_status_from_db (std::string &element_name) {
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached (DBConn::url);
    }
    catch ( const std::exception &e) {
        zsys_error ("DB: cannot connect, %s", e.what());
        return "unknown";
    }
    try{
        zsys_debug("get_status_from_db: getting status for asset %s", element_name.c_str());
        tntdb::Statement st = conn.prepareCached(
            " SELECT v.status "
            " FROM v_bios_asset_element v "
            " WHERE v.name=:vname ;"
            );

        tntdb::Row row = st.set ("vname", element_name).selectRow ();
        zsys_debug("get_status_from_db: [v_bios_asset_element]: were selected %zu rows", row.size());
        if (row.size() == 1) {
            std::string ret;
            row [0].get (ret);
            return ret;
        } else {
            return "unknown";
        }
    }
    catch (const tntdb::NotFound &e) {
        zsys_debug("get_status_from_db: [v_bios_asset_element]: %s asset not found", element_name.c_str ());
        return "unknown";
    }
    catch (const std::exception &e) {
        zsys_error ("get_status_from_db: [v_bios_asset_element]: error '%s'", e.what());
        return "unknown";
    }
}
