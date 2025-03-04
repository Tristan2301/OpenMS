// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2022.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Hendrik Weisser $
// $Authors: Hendrik Weisser $
// --------------------------------------------------------------------------
//

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>

///////////////////////////

#include <OpenMS/CHEMISTRY/RibonucleotideDB.h>

using namespace OpenMS;
using namespace std;

///////////////////////////

START_TEST(RibonucleotideDB, "$Id$")

/////////////////////////////////////////////////////////////

RibonucleotideDB* ptr = nullptr;
RibonucleotideDB* null = nullptr;
START_SECTION(RibonucleotideDB* getInstance())
{
  ptr = RibonucleotideDB::getInstance();
  TEST_NOT_EQUAL(ptr, null);
}
END_SECTION

START_SECTION(virtual ~RibonucleotideDB())
  NOT_TESTABLE
END_SECTION

START_SECTION(void readFromJSON_(void const std::string& path))
  // Reading from the JSON gets tested as part of the constructor above.
  // We check the contents below in begin() and getRibonucleotide
  NOT_TESTABLE
END_SECTION

START_SECTION(void readFromFile_(void const std::string& path))
  // Reading from the TSV gets tested as part of the constructor above.
  // We check the contents below in getRibonucleotide and getRibonucleotideAlternatives
  NOT_TESTABLE
END_SECTION

START_SECTION(ConstIterator begin())
{
  //Loading of the JSON and TSV files gets tested during the 
  RibonucleotideDB::ConstIterator it = ptr->begin();
  TEST_STRING_EQUAL((*it)->getCode(), "io6A");
}
END_SECTION

START_SECTION(ConstIterator end())
{
  RibonucleotideDB::ConstIterator it = ptr->end();
  TEST_EQUAL(it != ptr->begin(), true);
}
END_SECTION

START_SECTION((const Ribonucleotide& getRibonucleotide(const String& code)))
{
  // These three load from the Modomics.json
  const Ribonucleotide * ribo = ptr->getRibonucleotide("Am");
  TEST_STRING_EQUAL(ribo->getCode(), "Am");
  TEST_STRING_EQUAL(ribo->getName(), "2'-O-methyladenosine");
  // This loads from Custom_RNA_modifications.tsv
  const Ribonucleotide * customRibo = ptr->getRibonucleotide("msU?");

  TEST_EXCEPTION(Exception::ElementNotFound,
                 ptr->getRibonucleotide("bla"));
}
END_SECTION

START_SECTION( (pair<RibonucleotideDB::ConstRibonucleotidePtr, RibonucleotideDB::ConstRibonucleotidePtr> RibonucleotideDB::getRibonucleotideAlternatives(const std::string& code)))
{
  // THis also tests that loading from the TSV went well
  const pair<RibonucleotideDB::ConstRibonucleotidePtr, RibonucleotideDB::ConstRibonucleotidePtr> alts = ptr->getRibonucleotideAlternatives("msU?");
  TEST_STRING_EQUAL(alts.first->getCode(), "m5s2U");
  TEST_STRING_EQUAL(alts.second->getCode(), "s2Um");
}
END_SECTION

START_SECTION((const Ribonucleotide& getRibonucleotidePrefix(const String& seq)))
{
  const Ribonucleotide* ribo = ptr->getRibonucleotidePrefix("m1AmCGU");
  TEST_STRING_EQUAL(ribo->getCode(), "m1Am");
  TEST_EXCEPTION(Exception::ElementNotFound,
                 ptr->getRibonucleotidePrefix("blam1A"));
}
END_SECTION

START_SECTION(EmpiricalFormula getBaselossFormula())
{
  const Ribonucleotide* dna = ptr->getRibonucleotide("dT");
  TEST_EQUAL(EmpiricalFormula("C5H10O4") == dna->getBaselossFormula(), true);
  const Ribonucleotide* rnam = ptr->getRibonucleotide("Um");
  TEST_EQUAL(EmpiricalFormula("C6H12O5") == rnam->getBaselossFormula(), true);
}
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
