#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include "scanner.h"

class ScannerTest : public::testing::Test {
    protected:
        const char* filename = "temp_test_input.tmp";

        void setInput(const std::string& input)
        {
            std::ofstream out(filename);
            out << input;
            out.close();

            std::freopen(filename,"r",stdin);
        }

        void TearDown() override{
            std::remove(filename);
        }

    
};

TEST_F(ScannerTest,TestKeywords){
    setInput("var val while if do def then else return true false Int Unit Double import");

    for (int i = 0; i < 14; i++ )
        EXPECT_EQ(scanner_get_next_token().type,Token_KW);
    
    
}

TEST_F(ScannerTest,TestOP){
    setInput("< > <= >= == + - != * / && || ! ");

    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    EXPECT_EQ(scanner_get_next_token().type,Token_OP);
    

}

TEST_F(ScannerTest,TestINT)
{
    setInput("1000 2500 6000 584321 1 0 99999999999999 1_985_452 ");

    EXPECT_EQ(scanner_get_next_token().type,Token_INT);
    EXPECT_EQ(scanner_get_next_token().type,Token_INT);
    EXPECT_EQ(scanner_get_next_token().type,Token_INT);
    EXPECT_EQ(scanner_get_next_token().type,Token_INT);
    EXPECT_EQ(scanner_get_next_token().type,Token_INT);
    EXPECT_EQ(scanner_get_next_token().type,Token_INT);
    EXPECT_EQ(scanner_get_next_token().type,Token_INT);
    EXPECT_EQ(scanner_get_next_token().type,Token_INT);

}

TEST_F(ScannerTest,TestDouble)
{
    setInput("3.14 .5 1e10 3.14d");

    EXPECT_EQ(scanner_get_next_token().type,Token_DOUBLE);
    EXPECT_EQ(scanner_get_next_token().type,Token_DOUBLE);
    EXPECT_EQ(scanner_get_next_token().type,Token_DOUBLE);
    EXPECT_EQ(scanner_get_next_token().type,Token_DOUBLE);

}

TEST_F(ScannerTest, TESTID)
{
    setInput("pos _axis VALUE test_ID x axis_ pos2");

    for(int i = 0; i < 6; i++)
        EXPECT_EQ(scanner_get_next_token().type,Token_ID);

}

TEST_F(ScannerTest, TestStrings) {
    setInput("\"ahoj svet\" \"s escape \\n sekvenciou\"");

    Token t1 = scanner_get_next_token();
    EXPECT_EQ(t1.type, TOKEN_STRING);
    EXPECT_STREQ(t1.value.str_val, "ahoj svet");

    Token t2 = scanner_get_next_token();
    EXPECT_EQ(t2.type, TOKEN_STRING);
    EXPECT_STREQ(t2.value.str_val, "s escape \n sekvenciou");
}
