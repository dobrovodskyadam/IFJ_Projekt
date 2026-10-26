#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include "scanner.h"


 std::ostream& operator<<(std::ostream& os, TokenType type)
{
    switch (type)
    {
        case TOKEN_ID: return os << "TOKEN_ID";
        case TOKEN_INT: return os << "TOKEN_INT";
        case TOKEN_DOUBLE: return os << "TOKEN_DOUBLE";
        case TOKEN_STRING: return os << "TOKEN_STRING";
        case TOKEN_PLUS: return os << "TOKEN_PLUS";
        case TOKEN_MINUS: return os << "TOKEN_MINUS";
        case TOKEN_IF: return os << "TOKEN_IF";
        case TOKEN_ELSE: return os << "TOKEN_ELSE";
        case TOKEN_WHILE: return os << "TOKEN_WHILE";
        case TOKEN_FOR: return os << "TOKEN_FOR";
        case TOKEN_AND: return os << "TOKEN_AND";
        case TOKEN_OR: return os << "TOKEN_OR";
        case TOKEN_EQ: return os << "TOKEN_EQ";
        case TOKEN_GR_EQ: return os << "TOKEN_GR_EQ";
        case TOKEN_LS_EQ: return os << "TOKEN_LS_EQ";
        case TOKEN_LS: return os << "TOKEN_LS";
        case TOKEN_GR: return os << "TOKEN_GR";
        case TOKEN_DEF: return os << "TOKEN_DEF";
        case TOKEN_MUL: return os << "TOKEN_MUL";
        case TOKEN_DIV: return os << "TOKEN_DIV";
        case TOKEN_NEQ: return os << "TOKEN_NEQ";
        case TOKEN_LPAREN: return os << "TOKEN_LPAREN";
        case TOKEN_RPAREN: return os << "TOKEN_RPAREN";
        case TOKEN_ASSIGN: return os << "TOKEN_ASSIGN";
        case TOKEN_NOT: return os << "TOKEN_NOT";
        case TOKEN_ERROR: return os << "TOKEN_ERROR";
        case TOKEN_ERROR_MEMORY: return os << "TOKEN_ERROR_MEMORY";
        case TOKEN_VAR: return os << "TOKEN_VAR";
        case TOKEN_VAL: return os << "TOKEN_VAL";
        case TOKEN_RETURN: return os << "TOKEN_RETURN";
        case TOKEN_UNIT: return os << "TOKEN_UNIT";
        case TOKEN_BOOLEAN: return os << "TOKEN_BOOLEAN";
        case TOKEN_CASE: return os << "TOKEN_CASE";
        case TOKEN_LIST: return os << "TOKEN_LIST";
        case TOKEN_DO: return os << "TOKEN_DO";
        case TOKEN_THEN: return os << "TOKEN_THEN";
        case TOKEN_IMPORT: return os << "TOKEN_IMPORT";
        case TOKEN_KW_STRING: return os << "TOKEN_KW_STRING";
        case TOKEN_KW_INT: return os << "TOKEN_KW_INT";
        case TOKEN_KW_DOUBLE: return os << "TOKEN_KW_DOUBLE";
        case TOKEN_INDENT: return os << "TOKEN_INDENT";
        case TOKEN_DEDENT: return os << "TOKEN_DEDENT";
        case TOKEN_EOL: return os << "TOKEN_EOL";
        case TOKEN_SEPARATOR: return os << "TOKEN_SEPARATOR";
        case TOKEN_COLON: return os << "TOKEN_COLON";
        case TOKEN_IFJ: return os << "TOKEN_IFJ";
        case TOKEN_MAIN: return os << "TOKEN_MAIN";
        case TOKEN_EOF: return os << "TOKEN_EOF";
        default: return os << "TOKEN_UNKNOWN(" << static_cast<int>(type) << ")";
    }
}


class ScannerTest : public::testing::Test {
    protected:
        const char* filename = "temp_test_input.tmp";
        ScannerData scanner_data;

        void setInput(const std::string& input)
        {
            std::ofstream out(filename);
            out << input;
            out.close();

            std::freopen(filename,"r",stdin);
        }

        void SetUp() override {
            initScannerData(&scanner_data);
        }

        void TearDown() override{
            std::remove(filename);
        }

       Token scanner_get_next_token(void)
       {
            Token t;
            getToken(&t,&scanner_data);
            return t;
       }

    
};

TEST_F(ScannerTest,TestKeywords){
    setInput("var val while if do def then else return Int Unit Double import String Boolean case List @main");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_VAR);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_VAL);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_WHILE);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_IF);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_DO);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_DEF);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_THEN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ELSE);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_RETURN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_KW_INT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_UNIT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_KW_DOUBLE);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_IMPORT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_KW_STRING);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_BOOLEAN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_CASE);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_LIST);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_MAIN);
    
}

TEST_F(ScannerTest,TestOP){
    setInput("< > <= >= == + - != * / && || ! ");

    EXPECT_EQ(scanner_get_next_token().type,TOKEN_LS);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_GR);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_LS_EQ);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_GR_EQ);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_EQ);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_PLUS);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_MINUS);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_NEQ);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_MUL);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_DIV);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_AND);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_OR);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_NOT);
  
}

TEST_F(ScannerTest, TestINT)
{
    setInput("1000 2500 6000 584321 1 0 999999 1_985_452 1_2 ");

    const int expected[] = {1000, 2500, 6000, 584321, 1, 0, 999999, 1985452, 12};
    const int count = sizeof(expected) / sizeof(expected[0]);

    for (int i = 0; i < count; i++) {
        Token t = scanner_get_next_token();
        EXPECT_EQ(t.type, TOKEN_INT) << "token #" << i;
        EXPECT_EQ(t.value.int_val, expected[i]) << "token #" << i;
    }

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_EOF);
}

TEST_F(ScannerTest, TestDouble)
{
    setInput("3.14 .5 1e10 3.14d");

    const double expected[] = {3.14, 0.5, 1e10, 3.14};
    const int count = sizeof(expected) / sizeof(expected[0]);

    for (int i = 0; i < count; i++) {
        Token t = scanner_get_next_token();
        EXPECT_EQ(t.type, TOKEN_DOUBLE) << "token #" << i;
        EXPECT_DOUBLE_EQ(t.value.double_val, expected[i]) << "token #" << i;
    }

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_EOF);
}

TEST_F(ScannerTest, TESTID)
{
    setInput("pos _axis VALUE test_ID x axis_ pos2");
    const char* expected[] = {"pos", "_axis", "VALUE", "test_ID", "x", "axis_", "pos2"};
    const int count = sizeof(expected) / sizeof(expected[0]);   

    for(int i = 0; i < count; i++)
    {
        Token t = scanner_get_next_token();
        EXPECT_EQ(t.type, TOKEN_ID) << "token #" << i;
        ASSERT_NE(t.value.str_val, nullptr) << "token #" << i;
        EXPECT_STREQ(t.value.str_val, expected[i]) << "token #" << i;
    }

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
TEST_F(ScannerTest, TestComments) {
    setInput("// toto je riadkovy komentar\nvar // dalsi komentar\n/* blokovy \n komentar */ val");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_VAR);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_EOL); 
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_VAL);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_EOF);
}

TEST_F(ScannerTest, TestComplexNumbers){
    setInput("10e2 1.25e-3 0.05 1e+10");

    EXPECT_EQ(scanner_get_next_token().type,TOKEN_DOUBLE);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_DOUBLE);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_DOUBLE);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_DOUBLE);
    EXPECT_EQ(scanner_get_next_token().type,TOKEN_EOF);
}

TEST_F(ScannerTest, TestOperatorLookahead) {
    setInput("===<=>=");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_EQ);     // ==
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ASSIGN); // =
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_LS_EQ);   // <=
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_GR_EQ);   // >=
}

TEST_F(ScannerTest, TestEXP){
    setInput("val = 42 + 1_2");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_VAL);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ASSIGN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_INT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_PLUS);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_INT);

    setInput("val = 42 * (1_2)");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_VAL);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ASSIGN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_INT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_MUL);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_LPAREN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_INT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_RPAREN);

}

TEST_F(ScannerTest, TestDeclarations)
{
    setInput("def factorial(n: Int): Int =");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_DEF);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ID);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_LPAREN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ID);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_COLON);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_KW_INT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_RPAREN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_COLON);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_KW_INT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ASSIGN);

}

TEST_F(ScannerTest, TestSpecialTokensAndSeparators) {
    setInput("( ) ; : @main ifj26.*");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_LPAREN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_RPAREN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_SEPARATOR);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_COLON);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_MAIN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_IFJ);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_EOF);
}

TEST_F(ScannerTest, TestStringEscapesAll) {
    setInput("\"\\\\ \\\" \\n \\r \\t\"");

    Token t = scanner_get_next_token();
    EXPECT_EQ(t.type, TOKEN_STRING);
    EXPECT_STREQ(t.value.str_val, "\\ \" \n \r \t");
    free(t.value.str_val);
}

TEST_F(ScannerTest, TestTripleQuoteStrings) {
    setInput("\"\"\"riadok 1\nriadok 2\"\"\"");

    Token t = scanner_get_next_token();
    EXPECT_EQ(t.type, TOKEN_STRING);
    EXPECT_STREQ(t.value.str_val, "riadok 1\nriadok 2");
    free(t.value.str_val);
}

TEST_F(ScannerTest, TestUnclosedTripleQuoteString) {
    setInput("\"\"\"riadok 1\nriadok 2\"\"");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ERROR);
}

TEST_F(ScannerTest, TestInvalidEscape) {
    setInput("\"neplatny \\z escape\"");
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ERROR);
}

TEST_F(ScannerTest, TestUnclosedString) {
    setInput("\"unclosed string\n");
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ERROR);
}

TEST_F(ScannerTest, TestUnclosedBlockComment) {
    setInput("/* otvoreny komentar bez konca");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ERROR);
}

TEST_F(ScannerTest, TestIndentDedentMultiLevel) {
    setInput("var a = 1\n        val b = 2\nval c = 3");

    EXPECT_EQ(scanner_get_next_token().type, TOKEN_VAR);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ID);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ASSIGN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_INT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_EOL);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_INDENT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_VAL);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ID);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ASSIGN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_INT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_EOL);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_DEDENT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_VAL);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ID);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_ASSIGN);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_INT);
    EXPECT_EQ(scanner_get_next_token().type, TOKEN_EOF);
}