/*==============================================================================
 * FILE:       ParserTest.cc
 * OVERVIEW:   Provides the implementation for the ParserTest class, which
 *              tests the sslparser.y etc
 *============================================================================*/
/*
 * $Revision: 1.2 $
 *
 * 13 May 02 - Mike: Created
 */

#ifndef BOOMDIR
#error Must define BOOMDIR
#endif

#define SPARC_SSL		BOOMDIR "/frontend/machine/sparc/sparc.ssl"

#include "ParserTest.h"
#include "sslparser.h"

#include <sstream>

/*==============================================================================
 * FUNCTION:        ParserTest::registerTests
 * OVERVIEW:        Register the test functions in the given suite
 * PARAMETERS:      Pointer to the test suite
 * RETURNS:         <nothing>
 *============================================================================*/
#define MYTEST(name) \
suite->addTest(new CppUnit::TestCaller<ParserTest> ("ParserTest", \
    &ParserTest::name, *this))

void ParserTest::registerTests(CppUnit::TestSuite* suite) {
    MYTEST(testRead);
    MYTEST(testExp);
}

int ParserTest::countTestCases () const
{ return 2; }   // ? What's this for?

/*==============================================================================
 * FUNCTION:        ParserTest::setUp
 * OVERVIEW:        Set up some expressions for use with all the tests
 * NOTE:            Called before any tests
 * PARAMETERS:      <none>
 * RETURNS:         <nothing>
 *============================================================================*/
void ParserTest::setUp () {
}

/*==============================================================================
 * FUNCTION:        ParserTest::tearDown
 * OVERVIEW:        Delete expressions created in setUp
 * NOTE:            Called after all tests
 * PARAMETERS:      <none>
 * RETURNS:         <nothing>
 *============================================================================*/
void ParserTest::tearDown () {
}

/*==============================================================================
 * FUNCTION:        ParserTest::testRead
 * OVERVIEW:        Test reading the SSL file
 *============================================================================*/
void ParserTest::testRead () {
    RTLInstDict d;
    CPPUNIT_ASSERT(d.readSSLFile(SPARC_SSL));
}

/*==============================================================================
 * FUNCTION:        ParserTest::testExp
 * OVERVIEW:        Test parsing an expression
 *============================================================================*/
void ParserTest::testExp () {
    std::string s("*32* r0 := 5 + 6");
    Exp *e = SSLParser::parseExp(s.c_str());
    CPPUNIT_ASSERT(e);
    std::ostringstream ost;
    e->print(ost);
    CPPUNIT_ASSERT_EQUAL (s, std::string(ost.str()));
    std::string s2 = "*32* r[0] := 5 + 6";
    e = SSLParser::parseExp(s2.c_str());
    CPPUNIT_ASSERT(e);
    std::ostringstream ost2;
    e->print(ost2);
    // Still should print to string s, not s2
    CPPUNIT_ASSERT_EQUAL (s, std::string(ost2.str()));
}

