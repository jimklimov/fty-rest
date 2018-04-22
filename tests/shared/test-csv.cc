/*
 *
 * Copyright (C) 2015 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*!
 * \file test-csv.cc
 * \author Michal Vyskocil <MichalVyskocil@Eaton.com>
 * \author Alena Chernikava <AlenaChernikava@Eaton.com>
 * \author Jiri Kukacka <JiriKukacka@Eaton.com>
 * \brief Tests for CSV module
 */
#include <catch.hpp>
#include <cxxtools/csvdeserializer.h>
#include <cxxtools/jsondeserializer.h>

#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

#include <fty_proto.h>

#include "shared/csv.h"
#include "src/db/inout.h"
#include "shared/tntmlm.h"
using namespace shared;

TEST_CASE("CSV map basic get test", "[csv]") {

    std::stringstream buf;

    buf << "Name, Type, Group.1, group.2,description\n";
    buf << "RACK-01,rack,GR-01,GR-02,\"just,my,dc\"\n";
    buf << "RACK-02,rack,GR-01,GR-02,\"just\tmy\nrack\"\n";

    std::vector<std::vector<std::string> > data;
    cxxtools::CsvDeserializer deserializer(buf);
    deserializer.delimiter(',');
    deserializer.readTitle(false);
    deserializer.deserialize(data);

    shared::CsvMap cm{data};
    cm.deserialize();

    // an access to headers
    REQUIRE(cm.get(0, "Name") == "Name");
    REQUIRE(cm.get(0, "name") == "Name");
    REQUIRE(cm.get(0, "nAMe") == "Name");

    // an access to data
    REQUIRE(cm.get(1, "Name") == "RACK-01");
    REQUIRE(cm.get(2, "Name") == "RACK-02");

    // an access to data
    REQUIRE(cm.get(1, "Type") == "rack");
    REQUIRE(cm.get(2, "tYpe") == "rack");

    // out of bound access
    REQUIRE_THROWS_AS(cm.get(42, ""), std::out_of_range);
    // unknown key
    REQUIRE_THROWS_AS(cm.get(0, ""), std::out_of_range);

    // test values with commas
    REQUIRE(cm.get(1, "description") == "just,my,dc");
    REQUIRE(cm.get(2, "description") == "just\tmy\nrack");

    auto titles = cm.getTitles();
    REQUIRE(titles.size() == 5);
    REQUIRE(titles.count("name") == 1);
    REQUIRE(titles.count("type") == 1);
    REQUIRE(titles.count("group.1") == 1);
    REQUIRE(titles.count("group.2") == 1);
    REQUIRE(titles.count("description") == 1);

}

TEST_CASE("CSV multiple field names", "[csv]") {

    std::stringstream buf;

    buf << "Name, name\n";

    std::vector<std::vector<std::string> > data;
    cxxtools::CsvDeserializer deserializer(buf);
    deserializer.delimiter(',');
    deserializer.readTitle(false);
    deserializer.deserialize(data);

    shared::CsvMap cm{data};
    REQUIRE_THROWS_AS(cm.deserialize(), std::invalid_argument);

}

inline std::string to_utf8(const cxxtools::String& ws) {
    return cxxtools::Utf8Codec::encode(ws);
}

/*
 * This tests output from MS Excel, type Unicode Text, which is UTF-16 LE with BOM
 * As we support utf-8 only, file has been converted using iconv
 * iconv -f UTF-16LE -t UTF-8 INPUT > OUTPUT
 *
 * The file still have BOM, but at least unix end of lines, which is close to
 * expected usage, where iconv will be involved!
 *
 */
TEST_CASE("CSV utf-8 input", "[csv]") {

    std::string path{__FILE__};
    path += ".csv";

    std::ifstream buf{path};
    skip_utf8_BOM(buf);

    std::vector<std::vector<cxxtools::String> > data;

    cxxtools::CsvDeserializer deserializer(buf);
    deserializer.delimiter('\t');
    deserializer.readTitle(false);
    deserializer.deserialize(data);

    shared::CsvMap cm{data};
    REQUIRE_NOTHROW(cm.deserialize());

    REQUIRE(cm.get(0, "field") == "Field");
    REQUIRE(cm.get(0, "Ananotherone") == "An another one");

    REQUIRE(cm.get(1, "field") == to_utf8(cxxtools::String(L"тест")));
    REQUIRE(cm.get(2, "field") == to_utf8(cxxtools::String(L"测试")));
    REQUIRE(cm.get(3, "field") == to_utf8(cxxtools::String(L"Test")));

}

TEST_CASE("CSV findDelimiter", "[csv]")
{
    {
    std::stringstream buf;
    buf << "Some;delimiter\n";
    auto delim = findDelimiter(buf);

    REQUIRE(buf.str().size() == 15);
    REQUIRE(delim == ';');
    REQUIRE(buf.str().size() == 15);
    }

    {
    std::stringstream buf;
    buf << "None delimiter\n";
    auto delim = findDelimiter(buf);

    REQUIRE(buf.str().size() == 15);
    REQUIRE(delim == '\x0');
    REQUIRE(buf.str().size() == 15);
    }

    {
    std::stringstream buf;
    buf << "None_delimiter\n";
    auto delim = findDelimiter(buf);

    REQUIRE(buf.str().size() == 15);
    REQUIRE(delim == '\x0');
    REQUIRE(buf.str().size() == 15);
    }

}

TEST_CASE("CSV from_serialization_info", "[csv][si]")
{

    cxxtools::SerializationInfo si;

    si.setTypeName("Object");
    si.addMember("name") <<= "DC-1";
    si.addMember("type") <<= "datacenter";

    cxxtools::SerializationInfo& esi = si.addMember("ext");

    esi.setTypeName("Object");
    esi.addMember("ext1") <<= "ext1";
    esi.addMember("ext2") <<= "ext2";

    CsvMap map = CsvMap_from_serialization_info(si);
    REQUIRE(map.cols() == 4);
    REQUIRE(map.rows() == 2);

    std::vector<std::vector<std::string>> EXP = {
        {"name", "type", "ext1", "ext2"},
        {"DC-1", "datacenter", "ext1", "ext2"}
    };

    int i = 0;
    for (const auto& title : EXP[0]) {
        REQUIRE(map.hasTitle(title));
        REQUIRE(map.get(1, title) == EXP[1][i++]);
    }

}

TEST_CASE("CSV from_json", "[csv][si]")
{

    const char* JSON = "{\"name\":\"dc_name_test\",\"type\":\"datacenter\",\"sub_type\":\"\",\"location\":\"\",\"status\":\"active\",\"priority\":\"P1\",\"ext\":{\"asset_tag\":\"A123B123\",\"address\":\"ASDF\"}}";

    std::stringstream json_s{JSON};

    cxxtools::SerializationInfo si;
    cxxtools::JsonDeserializer jsd{json_s};
    jsd.deserialize(si);

    CsvMap map = CsvMap_from_serialization_info(si);

    REQUIRE(map.cols() == 8);
    REQUIRE(map.rows() == 2);

    std::vector<std::vector<std::string>> EXP = {
        {"name", "type", "sub_type", "location", "status", "priority", "asset_tag", "address"},
        {"dc_name_test", "datacenter", "", "", "active", "P1", "A123B123", "ASDF"}
    };

    int i = 0;
    for (const auto& title : EXP[0]) {
        REQUIRE(map.hasTitle(title));
        REQUIRE(map.get(1, title) == EXP[1][i++]);
    }
}


TEST_CASE("CSV map apostrof", "[csv]") {

    std::stringstream buf;

    buf << "Name, Type, Group.1, group.2,description\n";
    buf << "RACK-01,rack,GR-01,GR-02,\"just,my',dc\"\n";
    buf << "RACK-02,rack,GR-01,GR-02,\"just\tmy\nrack\"\n";

    REQUIRE_THROWS ( shared::CsvMap cm = CsvMap_from_istream(buf));
}


TEST_CASE("CSV map apostrof2", "[csv]") {

    std::stringstream buf;

    buf << "Name, Type, Group.1, group.2,description\n";
    buf << "RACK'-01,rack,GR-01,GR-02,\"just,my',dc\"\n";
    buf << "RACK-02,rack,GR-01,GR-02,\"just\tmy\nrack\"\n";

    REQUIRE_THROWS ( shared::CsvMap cm = CsvMap_from_istream(buf));
}


class MlmClientTest: public MlmClient {
    public:
        MlmClientTest(int sendto_retval) : MlmClient(NULL) {
            sendto_rv = sendto_retval;
            reply_no = 0;
            reply.clear();
        };
        ~MlmClientTest() = default;
        virtual zmsg_t* recv (const std::string& uuid, uint32_t timeout) {
            zsys_debug("Running mocked recv with uuid '%s' and timeout '%" PRIu32 "'", uuid.c_str(), timeout);
            if (reply_no-1 < reply.size()) {
            } else {
                throw std::range_error("Too many recv called, not enough replies provided");
            }
            zsys_debug("We have %u replies in vector, loading '%u' of them", reply.size(), reply_no);
            zmsg_t *rv = zmsg_dup(reply[reply_no-1]);
            if (NULL == rv)
                throw std::bad_alloc();
            return rv;
        }
        virtual int sendto (const std::string& address, const std::string& subject, uint32_t timeout, zmsg_t **content_p) {
            zsys_debug("Running mocked sendto with address '%s', subject '%s' and timeout '%" PRIu32 "'",
                    address.c_str(), subject.c_str(), timeout);
            zmsg_destroy(content_p);
            *content_p = NULL;
            ++reply_no;
            return sendto_rv;
        }
        void set_replies(zmsg_t *msg, ...) {
            va_list va_lst;
            reply.push_back(msg);
            va_start(va_lst, msg);
            zmsg_t *va_msg = va_arg(va_lst, zmsg_t *);
            while (NULL != va_msg) {
                reply.push_back(va_msg);
                va_msg = va_arg(va_lst, zmsg_t *);
            }
            va_end(va_lst);
        }
        void clear_replies() {
            reply.clear();
        }
        void set_reply_no(size_t rn) {
            reply_no = rn;
        }
        void set_sendto_rv(int sr) {
            sendto_rv = sr;
        }
    private:
        int sendto_rv;
        std::vector<zmsg_t *> reply;
        size_t reply_no;
};


TEST_CASE("test for rackcontroller-0 detection", "[csv]") {
    auto void_fn = []() {return;};
    int rv;
    int row_number;
    zsys_debug("Running test for rackcontroller-0 detection");
    // prepare reply messages for testing
    zmsg_t *reply_info_ok = zmsg_new ();
    zmsg_t *reply_info_error = zmsg_new ();
    zmsg_t *reply_get_rackcontroller_ok_empty = zmsg_new ();
    zmsg_t *reply_get_rackcontroller_ok_single = zmsg_new ();
    zmsg_t *reply_get_rackcontroller_ok_many = zmsg_new ();
    zmsg_t *reply_get_rackcontroller_error = zmsg_new ();
    zmsg_t *reply_asset_detail_rackcontroller0_ok = NULL;
    zmsg_t *reply_asset_detail_rackcontroller0_error = zmsg_new ();
    // reply INFO info ok
    zmsg_addstr (reply_info_ok, "OK");      // we don't really need those, but we need to pushstr them
    zmsg_addstr (reply_info_ok, "useless1"); // to conform to original message
    zmsg_addstr (reply_info_ok, "useless2");
    zmsg_addstr (reply_info_ok, "useless3");
    zmsg_addstr (reply_info_ok, "useless4");
    zhash_t *map = zhash_new();
    zhash_insert(map, "serial", (char *)"S3R1ALN0");
    zhash_insert(map, "hostname", (char *)"myhname");
    zhash_insert(map, "ip.1", (char *)"98.76.54.30");
    zhash_insert(map, "ip.2", (char *)"98.76.54.31");
    //zhash_insert(map, "ip.3", (char *)"98.76.54.32");
    zframe_t * frame_infos = zhash_pack(map);
    zmsg_append (reply_info_ok, &frame_infos);
    zframe_destroy(&frame_infos);
    // reply INFO info error
    zmsg_addstr (reply_info_error, "ERROR");
    zmsg_addstr (reply_info_error, "just error");
    // reply GET rackcontroller ASSETS ok empty
    zmsg_addstr (reply_get_rackcontroller_ok_empty, "OK");
    // reply GET rackcontroller ASSETS ok single
    zmsg_addstr (reply_get_rackcontroller_ok_single, "OK");
    zmsg_addstr (reply_get_rackcontroller_ok_single, "rackcontroller-0");
    // reply GET rackcontroller ASSETS ok many
    zmsg_addstr (reply_get_rackcontroller_ok_many, "OK");
    zmsg_addstr (reply_get_rackcontroller_ok_many, "rackcontroller-0");
    zmsg_addstr (reply_get_rackcontroller_ok_many, "rackcontroller-10");
    zmsg_addstr (reply_get_rackcontroller_ok_many, "rackcontroller-99");
    // reply GET rackcontroller ASSETS ok many
    zmsg_addstr (reply_get_rackcontroller_error, "ERROR");
    zmsg_addstr (reply_get_rackcontroller_error, "just error");
    // reply ASSET_DETAIL rackcontroller-0 ASSET_DETAIL ok
    zhash_t *aux = zhash_new ();
    zhash_insert (aux, "dummy", (char *)"dummy");
    zhash_t *ext = zhash_new ();
    zhash_insert (ext, "uuid", (char *)"uuid-0000-ok");
    zhash_insert (ext, "serial_no", (char *)"S3R1ALN01");
    zhash_insert (ext, "hostname.1", (char *)"myhname1");
    zhash_insert (ext, "fqdn.1", (char *)"fqdn.network.my");
    zhash_insert (ext, "ip.1", (char *)"98.76.54.30");
    reply_asset_detail_rackcontroller0_ok = fty_proto_encode_asset (aux, "rackcontroller-0", "UPDATE", ext);
    zhash_destroy (&ext);
    zhash_destroy (&aux);
    // reply ASSET_DETAIL rackcontroller-0 ASSET_DETAIL error
    zmsg_addstr (reply_asset_detail_rackcontroller0_error, "ERROR");
    zmsg_addstr (reply_asset_detail_rackcontroller0_error, "BAD_COMMAND");
    // initialization finished
    
    zsys_debug("\tTesting: empty database, 0 RCs in csv - detect none");
    // empty database, 0 RCs in csv - detect none
    row_number = -1;
    MlmClientTest *mct = new MlmClientTest(0);
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_empty, NULL);
    std::string base_path{__FILE__};
    std::string csv = base_path + ".rc0_1.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    zsys_debug ("Loading files from %s", csv.c_str());
    std::ifstream csv_buf(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: empty database, 1 rc in csv - promote it - was on row number 7");
    // empty database, 1 rc in csv - promote it - was on row number:
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_error, reply_get_rackcontroller_ok_empty, NULL);
    row_number = 6;
    csv = base_path + ".rc0_2.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: empty database, 3 RC in csv - promote one of it - was on row number 9");
    // empty database, 3 RC in csv - promote one of it - was on row number:
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_error, reply_get_rackcontroller_ok_empty, NULL);
    row_number = 8;
    csv = base_path + ".rc0_3.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 1 RC in database, 0 RCs in csv - detect none");
    // 1 RC in database, 0 RCs in csv - detect none
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_single, NULL);
    row_number = -1;
    csv = base_path + ".rc0_1.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 1 RC in database, 1 RC in csv - promote it - was on row number 7");
    // 1 RC in database, 1 RC in csv - promote it - was on row number:
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_single, NULL);
    row_number = 6;
    csv = base_path + ".rc0_2a.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 1 RC in database, 3 RC in csv - promote one of it - was on row number 9");
    // 1 RC in database, 3 RC in csv - promote one of it - was on row number:
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_single, NULL);
    row_number = 8;
    csv = base_path + ".rc0_3.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 1 RC in database, 3 RC in csv - was none of it");
    // 1 RC in database, 3 RC in csv - was none of it
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_single, NULL);
    row_number = -1;
    csv = base_path + ".rc0_4.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 1 RC in database, 3 RC in csv - was different than rackcontroller-0");
    // 1 RC in database, 3 RC in csv - was different than rackcontroller-0
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_single, NULL);
    row_number = -1;
    csv = base_path + ".rc0_5.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 3 RC in database, 0 RCs in csv - detect none");
    // 3 RC in database, 0 RCs in csv - detect none
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_many, NULL);
    row_number = -1;
    csv = base_path + ".rc0_1.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 3 RC in database, 1 RC in csv - promote it - was on row number 7");
    // 3 RC in database, 1 RC in csv - promote it - was on row number:
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_many, NULL);
    row_number = 6;
    csv = base_path + ".rc0_2a.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 3 RC in database, 3 RC in csv - promote one of it - was on row number 9");
    // 3 RC in database, 3 RC in csv - promote one of it - was on row number:
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_many, NULL);
    row_number = 8;
    csv = base_path + ".rc0_3.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 3 RC in database, 3 RC in csv - was none of it");
    // 3 RC in database, 3 RC in csv - was none of it
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_many, NULL);
    row_number = -1;
    csv = base_path + ".rc0_4.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 3 RC in database, 3 RC in csv - was different than rackcontroller-0");
    // 3 RC in database, 3 RC in csv - was different than rackcontroller-0
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_many, NULL);
    row_number = -1;
    csv = base_path + ".rc0_5.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    zsys_debug("\tTesting: 3 RC in database, 3 RC in csv - with missing ids");
    // 3 RC in database, 3 RC in csv - with missing ids
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_many, NULL);
    row_number = 8;
    csv = base_path + ".rc0_6.csv";
    zsys_debug("Using file %s for this test", csv.c_str());
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn));
    REQUIRE(row_number == rv);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    /*
     * disabled due to workaround to speed up api call
    zsys_debug("\tTesting: getting assets from fty_assets fails");
    // getting assets from fty_assets fails
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_error, NULL);
    REQUIRE_THROWS_AS(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn), std::runtime_error);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();
    */

    /*
     * disabled due to changes to enable running without alreday having RC-0 in DB
    zsys_debug("\tTesting: getting asset detail from fty_assets fails");
    // getting asset detail from fty_assets fails
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_error, reply_get_rackcontroller_error, NULL);
    REQUIRE_THROWS_AS(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn), std::runtime_error);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();
    */

    zsys_debug("\tTesting: getting info from fty_info fails");
    // getting info from fty_info fails
    mct->set_replies(reply_info_error, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_empty, NULL);
    REQUIRE_THROWS_AS(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn), std::runtime_error);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    mct->set_sendto_rv(-1);
    zsys_debug("\tTesting: sendto fails fails");
    // sendto fails fails
    mct->set_replies(reply_info_ok, reply_asset_detail_rackcontroller0_ok, reply_get_rackcontroller_ok_empty, NULL);
    REQUIRE_THROWS_AS(rv = persist::promote_rc0(mct, shared::CsvMap_from_istream(csv_buf), void_fn), std::runtime_error);
    // reset mct for next test
    mct->set_reply_no(0);
    mct->clear_replies();

    // clean up all the stuff
    zmsg_destroy (&reply_info_ok);
    zmsg_destroy (&reply_info_error);
    zmsg_destroy (&reply_get_rackcontroller_ok_empty);
    zmsg_destroy (&reply_get_rackcontroller_ok_single);
    zmsg_destroy (&reply_get_rackcontroller_ok_many);
    zmsg_destroy (&reply_get_rackcontroller_error);
    zmsg_destroy (&reply_asset_detail_rackcontroller0_ok);
    zmsg_destroy (&reply_asset_detail_rackcontroller0_error);
}
