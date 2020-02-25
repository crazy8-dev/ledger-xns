/*******************************************************************************
*   (c) 2018 ZondaX GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "gtest/gtest.h"
#include <json/json_parser.h>
#include <json/tx_display.h>
#include <json/tx_parser.h>
#include <parser.h>
#include "util/common.h"

namespace {
    parser_error_t tx_traverse(int16_t root_token_index, uint8_t *numChunks) {
        uint16_t ret_value_token_index;
        parser_error_t err = tx_traverse_find(root_token_index, &ret_value_token_index);

        if (err != parser_ok)
            return err;

        return tx_getToken(ret_value_token_index,
                           parser_tx_obj.query.out_val, parser_tx_obj.query.out_val_len,
                           parser_tx_obj.query.chunk_index, numChunks);
    }

    TEST(TxParse, Tx_Traverse) {
        auto transaction = R"({"keyA":"123456", "keyB":"abcdefg", "keyC":""})";

        parser_tx_obj.tx = transaction;
        parser_tx_obj.cache_valid = false;
        parser_error_t err = JSON_PARSE(&parser_tx_obj.json, parser_tx_obj.tx);
        ASSERT_EQ(err, parser_ok);

        char key[100];
        char val[100];
        uint8_t numChunks;

        // Try second key - first chunk
        INIT_QUERY_CONTEXT(key, sizeof(key), val, sizeof(val), 0, 4)
        parser_tx_obj.query.item_index = 1;

        err = tx_traverse(0, &numChunks);
        EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
        EXPECT_EQ(numChunks, 1) << "Incorrect number of chunks";
        EXPECT_EQ_STR(key, "keyB", "Incorrect key")
        EXPECT_EQ_STR(val, "abcdefg", "Incorrect value")

        // Try second key - Second chunk
        INIT_QUERY_CONTEXT(key, sizeof(key), val, sizeof(val), 1, 4)
        parser_tx_obj.query.item_index = 1;
        err = tx_traverse(0, &numChunks);
        EXPECT_EQ(err, parser_display_page_out_of_range) << parser_getErrorDescription(err);
        EXPECT_EQ(numChunks, 1) << "Incorrect number of chunks";

        // Find first key
        INIT_QUERY_CONTEXT(key, sizeof(key), val, sizeof(val), 0, 4)
        parser_tx_obj.query.item_index = 0;
        err = tx_traverse(0, &numChunks);
        EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
        EXPECT_EQ(numChunks, 1) << "Incorrect number of chunks";
        EXPECT_EQ_STR(key, "keyA", "Incorrect key")
        EXPECT_EQ_STR(val, "123456", "Incorrect value")

        // Try the same again
        INIT_QUERY_CONTEXT(key, sizeof(key), val, sizeof(val), 0, 4)
        parser_tx_obj.query.item_index = 0;
        err = tx_traverse(0, &numChunks);
        EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
        EXPECT_EQ(numChunks, 1) << "Incorrect number of chunks";
        EXPECT_EQ_STR(key, "keyA", "Incorrect key")
        EXPECT_EQ_STR(val, "123456", "Incorrect value")

        // Try last key
        INIT_QUERY_CONTEXT(key, sizeof(key), val, sizeof(val), 0, 4)
        parser_tx_obj.query.item_index = 2;
        err = tx_traverse(0, &numChunks);
        EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
        EXPECT_EQ(numChunks, 1) << "Incorrect number of chunks";
        EXPECT_EQ_STR(key, "keyC", "Incorrect key")
        EXPECT_EQ_STR(val, "", "Incorrect value")
    }

    TEST(TxParse, OutOfBoundsSmall) {
        auto transaction = R"({"keyA":"123456", "keyB":"abcdefg"})";

        parser_tx_obj.tx = transaction;
        parser_tx_obj.cache_valid = false;
        parser_error_t err = JSON_PARSE(&parser_tx_obj.json, parser_tx_obj.tx);
        ASSERT_EQ(err, parser_ok);

        char key[1000];
        char val[1000];
        uint8_t numChunks;

        INIT_QUERY_CONTEXT(key, sizeof(key), val, sizeof(val), 5, 4)
        err = tx_traverse(0, &numChunks);
        EXPECT_EQ(err, parser_display_page_out_of_range) << "Item not found";

        // We should find it.. but later tx_display should fail
        INIT_QUERY_CONTEXT(key, sizeof(key), val, sizeof(val), 0, 4)
        err = tx_traverse(0, &numChunks);
        EXPECT_EQ(err, parser_ok);
        EXPECT_EQ(numChunks, 1) << "Item not found";
    }

    TEST(TxParse, Count_Minimal) {
        auto transaction = R"({"account_number":"0"})";

        parser_tx_obj.tx = transaction;
        parser_tx_obj.cache_valid = false;
        parser_error_t err = JSON_PARSE(&parser_tx_obj.json, parser_tx_obj.tx);
        EXPECT_EQ(err, parser_ok);

        auto num_pages = tx_display_numItems();

        EXPECT_EQ(0, num_pages) << "Wrong number of pages";
    }

    TEST(TxParse, Tx_Page_Count_Transfer) {
        auto transaction = R"({"jsonrpc":"2.0","id":1,"method":"contract.call","params":{"callSite":"member.transfer","callParams":{"amount":"20000000000","toMemberReference":"insolar:1AAEAATjWvxVC3DFEBqINu7JSKWxlcb_uJo7QdAHrcP8"},"reference":"insolar:1AAEAAY2zkW3pOTCIlYg6XqhWLR32AAeBpTKZ3vLdFhE","publicKey":"-----BEGIN PUBLIC KEY-----\nMFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEc+Vs4y+XWE77LR0QL1e1wCOFePFEHJIB\ndsPWPKMH5zGRhRWV1HCJXajENCV2bdG/YKKEOAzTdE5BGXNg2dRQpQ==\n-----END PUBLIC KEY-----","seed":"rOpiqgDHHDmr2PfI5UzZiWpjRyDMoFtBNFoOwyC+yJ8="}})";

        parser_tx_obj.tx = transaction;
        parser_tx_obj.cache_valid = false;
        parser_error_t err = JSON_PARSE(&parser_tx_obj.json, parser_tx_obj.tx);
        EXPECT_EQ(err, parser_ok);

        auto num_pages = tx_display_numItems();
        EXPECT_EQ(3, num_pages) << "Wrong number of pages";
    }

    TEST(TxParse, Page_Count_Create_Member) {
        auto transaction =
                R"({"jsonrpc":"2.0","id":1,"method":"contract.call","params":{"callSite":"member.migrationCreate","publicKey":"-----BEGIN PUBLIC KEY-----\nMFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEc+Vs4y+XWE77LR0QL1e1wCOFePFEHJIB\ndsPWPKMH5zGRhRWV1HCJXajENCV2bdG/YKKEOAzTdE5BGXNg2dRQpQ==\n-----END PUBLIC KEY-----","seed":"rOpiqgDHHDmr2PfI5UzZiWpjRyDMoFtBNFoOwyC+yJ8="}})";

        parser_tx_obj.tx = transaction;
        parser_tx_obj.cache_valid = false;
        parser_error_t err = JSON_PARSE(&parser_tx_obj.json, parser_tx_obj.tx);
        EXPECT_EQ(err, parser_ok);

        EXPECT_EQ(1, tx_display_numItems()) << "Wrong number of items";
    }
}
