/*
 * Copyright (C) 1997-2001, The University of Queensland
 * Copyright (C) 2000-2001, Sun Microsystems, Inc
 * Copyright (C) 2002, Trent Waddington
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 */

/*==============================================================================
 * FILE:       basicblock.cc
 * OVERVIEW:   Implementation of the BasicBlock class.
 *============================================================================*/

/*
 * $Revision: 1.53 $
 * Dec 97 - created by Mike
 * 18 Apr 02 - Mike: Changes for boomerang
 * 04 Dec 02 - Mike: Added isJmpZ
 * 09 Jan 02 - Mike: Untabbed, reformatted
 * 17 Jun 03 - Mike: Fixed an apparent error in generateCode (getCond)
*/


/*==============================================================================
 * Dependencies.
 *============================================================================*/

#include <assert.h>
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma warning(disable:4786)
#endif 

#include "types.h"
#include "statement.h"
#include "exp.h"
#include "cfg.h"
#include "register.h"
#include "rtl.h"
#include "hllcode.h"
#include "proc.h"
#include "prog.h"
#include "util.h"
#include "boomerang.h"

/**********************************
 * BasicBlock methods
 **********************************/

/*==============================================================================
 * FUNCTION:        BasicBlock::BasicBlock
 * OVERVIEW:        Constructor.
 * PARAMETERS:      <none>
 * RETURNS:         <nothing>
 *============================================================================*/
BasicBlock::BasicBlock()
    :
        m_DFTfirst(0),
        m_structType(NONE), m_loopCondType(NONE),
        m_loopHead(NULL), m_caseHead(NULL),
        m_condFollow(NULL), m_loopFollow(NULL),
        m_latchNode(NULL),
        m_nodeType(INVALID),
        m_pRtls(NULL),
        m_iLabelNum(0),
        m_labelneeded(false),
        m_bIncomplete(true),
        m_bJumpReqd(false),
        m_iNumInEdges(0),
        m_iNumOutEdges(0),
        m_iTraversed(false),
        m_returnVal(NULL),
// From Doug's code
ord(-1), revOrd(-1), inEdgesVisited(0), numForwardInEdges(-1), 
traversed(UNTRAVERSED), hllLabel(false),
indentLevel(0), immPDom(NULL), loopHead(NULL), caseHead(NULL), 
condFollow(NULL), loopFollow(NULL), latchNode(NULL), sType(Seq), 
usType(Structured) 
{
}

/*==============================================================================
 * FUNCTION:        BasicBlock::~BasicBlock
 * OVERVIEW:        Destructor.
 * PARAMETERS:      <none>
 * RETURNS:         <nothing>
 *============================================================================*/
BasicBlock::~BasicBlock() {
    if (m_pRtls)
    {
        // Delete the RTLs
        for (std::list<RTL*>::iterator it = m_pRtls->begin(); it != m_pRtls->end(); it++)
        {
            if (*it)
            {
                delete *it;
            }
        }

        // and delete the list
        delete m_pRtls;
        m_pRtls = NULL;
    }
    if (m_returnVal) delete m_returnVal;
}


/*==============================================================================
 * FUNCTION:        BasicBlock::BasicBlock
 * OVERVIEW:        Copy constructor.    
 * PARAMETERS:      bb - the BB to copy from
 * RETURNS:         <nothing>
 *============================================================================*/
BasicBlock::BasicBlock(const BasicBlock& bb)
    :   m_DFTfirst(0),
        m_structType(bb.m_structType), m_loopCondType(bb.m_loopCondType),
        m_loopHead(bb.m_loopHead), m_caseHead(bb.m_caseHead),
        m_condFollow(bb.m_condFollow), m_loopFollow(bb.m_loopFollow),
        m_latchNode(bb.m_latchNode),
        m_nodeType(bb.m_nodeType),
        m_pRtls(NULL),
        m_iLabelNum(bb.m_iLabelNum),
        m_labelneeded(false),
        m_bIncomplete(bb.m_bIncomplete),
        m_bJumpReqd(bb.m_bJumpReqd),
        m_InEdges(bb.m_InEdges),
        m_OutEdges(bb.m_OutEdges),
        m_iNumInEdges(bb.m_iNumInEdges),
        m_iNumOutEdges(bb.m_iNumOutEdges),
        m_iTraversed(false),
        m_returnVal(bb.m_returnVal),
// From Doug's code
ord(bb.ord), revOrd(bb.revOrd), inEdgesVisited(bb.inEdgesVisited), 
numForwardInEdges(bb.numForwardInEdges), traversed(bb.traversed), 
hllLabel(bb.hllLabel), indentLevel(bb.indentLevel), 
immPDom(bb.immPDom), loopHead(bb.loopHead), caseHead(bb.caseHead), 
condFollow(bb.condFollow), loopFollow(bb.loopFollow), 
latchNode(bb.latchNode), sType(bb.sType), usType(bb.usType) 
{
    setRTLs(bb.m_pRtls);
}

/*==============================================================================
 * FUNCTION:        BasicBlock::BasicBlock
 * OVERVIEW:        Private constructor.
 * PARAMETERS:      pRtls -
 *                  bbType -
 *                  iNumOutEdges -
 * RETURNS:         <nothing>
 *============================================================================*/
BasicBlock::BasicBlock(std::list<RTL*>* pRtls, BBTYPE bbType, int iNumOutEdges)
    :   m_DFTfirst(0),
        m_structType(NONE), m_loopCondType(NONE),
        m_loopHead(NULL), m_caseHead(NULL),
        m_condFollow(NULL), m_loopFollow(NULL),
        m_latchNode(NULL),  
        m_nodeType(bbType),
        m_pRtls(NULL),
        m_iLabelNum(0),
        m_labelneeded(false),
        m_bIncomplete(false),
        m_bJumpReqd(false),
        m_iNumInEdges(0),
        m_iNumOutEdges(iNumOutEdges),
        m_iTraversed(false),
        m_returnVal(NULL),
// From Doug's code
ord(-1), revOrd(-1), inEdgesVisited(0), numForwardInEdges(-1), 
traversed(UNTRAVERSED), hllLabel(false),
indentLevel(0), immPDom(NULL), loopHead(NULL), caseHead(NULL), 
condFollow(NULL), loopFollow(NULL), latchNode(NULL), sType(Seq), 
usType(Structured) 
{
    m_OutEdges.reserve(iNumOutEdges);               // Reserve the space;
                                                    // values added with
                                                    // AddOutEdge()

    // Set the RTLs
    setRTLs(pRtls);

}

/*==============================================================================
 * FUNCTION:        BasicBlock::getLabel
 * OVERVIEW:        Returns nonzero if this BB has a label, in the sense that a
 *                  label is required in the translated source code
 * PARAMETERS:      <none>
 * RETURNS:         An integer unique to this BB, or zero
 *============================================================================*/
int BasicBlock::getLabel() {
    return m_iLabelNum;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::isTraversed
 * OVERVIEW:        Returns nonzero if this BB has been traversed
 * PARAMETERS:      <none>
 * RETURNS:         True if traversed
 *============================================================================*/
bool BasicBlock::isTraversed() {
    return m_iTraversed;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::setTraversed
 * OVERVIEW:        Sets the traversed flag
 * PARAMETERS:      bTraversed: true to set this BB to traversed
 * RETURNS:         <nothing>
 *============================================================================*/
void BasicBlock::setTraversed(bool bTraversed) {
    m_iTraversed = bTraversed;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::setRTLs
 * OVERVIEW:        Sets the RTLs for a basic block. This is the only place that
 *                  the RTLs for a block must be set as we need to add the back
 *                  link for a call instruction to its enclosing BB.
 * PARAMETERS:      rtls - a list of RTLs
 * RETURNS:         <nothing>
 *============================================================================*/
void BasicBlock::setRTLs(std::list<RTL*>* rtls) {
    // should we delete old ones here?  breaks some things - trent
    m_pRtls = rtls;

    // Set the link between the last instruction (a call) and this BB
    // if this is a call BB
    CallStatement* call = (CallStatement*)*(m_pRtls->rbegin());
    if (call->getKind() == STMT_CALL)
        call->setBB(this);
}

void BasicBlock::setReturnVal(Exp *e) {
    if (m_returnVal) delete m_returnVal; 
    m_returnVal = e; 
}

/*==============================================================================
 * FUNCTION:        BasicBlock::getType
 * OVERVIEW:        Return the type of the basic block.
 * PARAMETERS:      <none>
 * RETURNS:         the type of the basic block
 *============================================================================*/
BBTYPE BasicBlock::getType() {
    return m_nodeType;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::updateType
 * OVERVIEW:        Update the type and number of out edges. Used for example
 *                  where a COMPJUMP type is updated to an NWAY when a switch
 *                  idiom is discovered.
 * PARAMETERS:      bbType - the new type
 *                  iNumOutEdges - new number of inedges
 * RETURNS:         <nothing>
 *============================================================================*/
void BasicBlock::updateType(BBTYPE bbType, int iNumOutEdges) {
    m_nodeType = bbType;
    m_iNumOutEdges = iNumOutEdges;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::setJumpReqd
 * OVERVIEW:        Sets the "jump required" bit. This means that this BB is
 *                  an orphan (not generated from input code), and that the
 *                  "fall through" out edge needs to be implemented as a jump
 * PARAMETERS:      <none>
 * RETURNS:         <nothing>
 *============================================================================*/
void BasicBlock::setJumpReqd() {
    m_bJumpReqd = true;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::isJumpReqd
 * OVERVIEW:        Returns the "jump required" bit. See above for details
 * PARAMETERS:      <none>
 * RETURNS:         True if a jump is required
 *============================================================================*/
bool BasicBlock::isJumpReqd() {
    return m_bJumpReqd;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::prints
 * OVERVIEW:        Print to a static string (for debugging) 
 * PARAMETERS:      <none>
 * RETURNS:         Address of the static buffer
 *============================================================================*/
static char debug_buffer[1000];
char* BasicBlock::prints() {   
    std::ostringstream ost; 
    print(ost);       
    // Static buffer may overflow; that's OK, we just copy and print the first
    // 999 bytes
    strncpy(debug_buffer, ost.str().c_str(), 999);
    debug_buffer[999] = '\0';
    return debug_buffer; 
}

/*==============================================================================
 * FUNCTION:        BasicBlock::print()
 * OVERVIEW:        Display the whole BB to the given stream
 *                  Used for "-R" option, and handy for debugging
 * PARAMETERS:      os - stream to output to
 * RETURNS:         <nothing>
 *============================================================================*/
void BasicBlock::print(std::ostream& os, bool withDF) {
    if (m_iLabelNum) os << "L" << std::dec << m_iLabelNum << ": ";
    switch(m_nodeType) {
        case ONEWAY:    os << "Oneway BB"; break;
        case TWOWAY:    os << "Twoway BB"; break;
        case NWAY:      os << "Nway BB"; break;
        case CALL:      os << "Call BB"; break;
        case RET:       os << "Ret BB"; break;
        case FALL:      os << "Fall BB"; break;
        case COMPJUMP:  os << "Computed jump BB"; break;
        case COMPCALL:  os << "Computed call BB"; break;
        case INVALID:   os << "Invalid BB"; break;
    }
    os << ":\n";
    if (m_pRtls) {                  // Can be zero if e.g. INVALID
        std::list<RTL*>::iterator rit;
        for (rit = m_pRtls->begin(); rit != m_pRtls->end(); rit++) {
            (*rit)->print(os, withDF);
        }
    }
    if (m_bJumpReqd) {
        os << "Synthetic out edge(s) to ";
        for (int i=0; i < m_iNumOutEdges; i++) {
            PBB outEdge = m_OutEdges[i];
            if (outEdge && outEdge->m_iLabelNum)
                os << "L" << std::dec << outEdge->m_iLabelNum << " ";
        }
        os << std::endl;
    }
}

/*==============================================================================
 * FUNCTION:        BasicBlock::getLowAddr
 * OVERVIEW:        Get the lowest real address associated with this BB. Note
 *                  that although this is usually the address of the first RTL,
 *                  it is not always so. For example, if the BB contains just a
 *                  delayed branch, and the delay instruction for the branch
 *                  does not affect the branch, so the delay instruction is
 *                  copied in  front of the branch instruction. It's address
 *                  will be UpdateAddress()d to 0, since it is "not really
 *                  there", so the low address for this BB will be the address
 *                  of the branch.
 * PARAMETERS:      <none>
 * RETURNS:         the lowest real address associated with this BB
 *============================================================================*/
ADDRESS BasicBlock::getLowAddr() {
    assert(m_pRtls != NULL);
    ADDRESS a = m_pRtls->front()->getAddress();
    if ((a == 0) && (m_pRtls->size() > 1)) {
        std::list<RTL*>::iterator it = m_pRtls->begin();
        ADDRESS add2 = (*++it)->getAddress();
        // This is a bit of a hack for 286 programs, whose main actually starts
        // at offset 0. A better solution would be to change orphan BBs'
        // addresses to NO_ADDRESS, but I suspect that this will cause many
        // problems. MVE
        if (add2 < 0x10)
            // Assume that 0 is the real address
            return 0;
        return add2;
    }
    return a;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::getHiAddr
 * OVERVIEW:        Get the highest address associated with this BB. This is
 *                  always the address associated with the last RTL.
 * PARAMETERS:      <none>
 * RETURNS:         <nothing>
 *============================================================================*/
ADDRESS BasicBlock::getHiAddr() {
    assert(m_pRtls != NULL);
    return m_pRtls->back()->getAddress();
}

/*==============================================================================
 * FUNCTION:        BasicBlock::getRTLs
 * OVERVIEW:        Get pointer to the list of RTL*.
 * PARAMETERS:      <none>
 * RETURNS:         <nothing>
 *============================================================================*/
std::list<RTL*>* BasicBlock::getRTLs() {
    return m_pRtls;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::getInEdges
 * OVERVIEW:        Get a constant reference to the vector of in edges.
 * PARAMETERS:      <none>
 * RETURNS:         a constant reference to the vector of in edges
 *============================================================================*/
std::vector<PBB>& BasicBlock::getInEdges() {
    return m_InEdges;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::getOutEdges
 * OVERVIEW:        Get a constant reference to the vector of out edges.
 * PARAMETERS:      <none>
 * RETURNS:         a constant reference to the vector of out edges
 *============================================================================*/
std::vector<PBB>& BasicBlock::getOutEdges() {
    return m_OutEdges;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::setInEdge
 * OVERVIEW:        Change the given in-edge (0 is first) to the given value
 *                  Needed for example when duplicating BBs
 * PARAMETERS:      i: index (0 based) of in-edge to change
 *                  pNewInEdge: pointer to BB that will be a new parent
 * RETURNS:         <nothing>
 *============================================================================*/
void BasicBlock::setInEdge(int i, PBB pNewInEdge) {
    m_InEdges[i] = pNewInEdge;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::setOutEdge
 * OVERVIEW:        Change the given out-edge (0 is first) to the given value
 *                  Needed for example when duplicating BBs
 * NOTE:            Cannot add an additional out-edge with this function; use
 *                  addOutEdge for this rare case
 * PARAMETERS:      i: index (0 based) of out-edge to change
 *                  pNewOutEdge: pointer to BB that will be the new successor
 * RETURNS:         <nothing>
 *============================================================================*/
void BasicBlock::setOutEdge(int i, PBB pNewOutEdge) {
    if (m_OutEdges.size() == 0) {
        assert(i == 0);
        m_OutEdges.push_back(pNewOutEdge);
    } else {
        assert(i < (int)m_OutEdges.size());
        m_OutEdges[i] = pNewOutEdge;
    }
}

/*==============================================================================
 * FUNCTION:        BasicBlock::getOutEdge
 * OVERVIEW:        Returns the i-th out edge of this BB; counting starts at 0
 * NOTE:            
 * PARAMETERS:      i: index (0 based) of the desired out edge
 * RETURNS:         the i-th out edge; 0 if there is no such out edge
 *============================================================================*/
PBB BasicBlock::getOutEdge(unsigned int i) {
    if (i < m_OutEdges.size()) return m_OutEdges[i];
    else return 0;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::getCorrectOutEdge
 * OVERVIEW:        given an address this method returns the corresponding
 *                  out edge
 * NOTE:            
 * PARAMETERS:      a: the address
 * RETURNS:         
 *============================================================================*/

PBB BasicBlock::getCorrectOutEdge(ADDRESS a) {
    std::vector<PBB>::iterator it;
    for (it = m_OutEdges.begin(); it != m_OutEdges.end(); it++) {
        if ((*it)->getLowAddr() == a) return *it; 
    }
    return 0;
}


/*==============================================================================
 * FUNCTION:        BasicBlock::addInEdge
 * OVERVIEW:        Add the given in-edge
 *                  Needed for example when duplicating BBs
 * PARAMETERS:      pNewInEdge: pointer to BB that will be a new parent
 * RETURNS:         <nothing>
 *============================================================================*/
void BasicBlock::addInEdge(PBB pNewInEdge) {
    m_InEdges.push_back(pNewInEdge);
    m_iNumInEdges++;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::deleteInEdge
 * OVERVIEW:        Delete the in-edge from the given BB
 *                  Needed for example when duplicating BBs
 * PARAMETERS:      it: iterator to BB that will no longer be a parent
 * SIDE EFFECTS:    The iterator argument is incremented.
 * USAGE:           It should be used like this:
 *                    if (pred) deleteInEdge(it) else it++;
 * RETURNS:         <nothing>
 *============================================================================*/
void BasicBlock::deleteInEdge(std::vector<PBB>::iterator& it) {
    it = m_InEdges.erase(it);
    m_iNumInEdges--;
}

void BasicBlock::deleteInEdge(PBB edge) {
    for (std::vector<PBB>::iterator it = m_InEdges.begin(); 
         it != m_InEdges.end(); it++) {
        if (*it == edge) {
            deleteInEdge(it);
            break;
        }
    }
}

void BasicBlock::deleteEdge(PBB edge) {
    edge->deleteInEdge(this);
    for (std::vector<PBB>::iterator it = m_OutEdges.begin(); 
         it != m_OutEdges.end(); it++) {
        if (*it == edge) {
            m_OutEdges.erase(it);
            m_iNumOutEdges--;
            break;
        }
    }
}

/*==============================================================================
 * FUNCTION:        BasicBlock::DFTOrder
 * OVERVIEW:        Traverse this node and recurse on its children in a depth
 *                  first manner. Records the times at which this node was first
 *                  visited and last visited
 * PARAMETERS:      first - the number of nodes that have been visited
 *                  last - the number of nodes that have been visited for the
 *                    last time during this traversal
 * RETURNS:         the number of nodes (including this one) that were traversed
 *                  from this node
 *============================================================================*/
unsigned BasicBlock::DFTOrder(int& first, int& last) {
    first++;
    m_DFTfirst = first;

    unsigned numTraversed = 1;
    m_iTraversed = true;

    for (std::vector<PBB>::iterator it = m_OutEdges.begin();
      it != m_OutEdges.end(); it++) {

        PBB child = *it;
        if (child->m_iTraversed == false)
            numTraversed = numTraversed + child->DFTOrder(first,last);
    }

    last++;
    m_DFTlast = last;

    return numTraversed;
}

/*==============================================================================
 * FUNCTION:      BasicBlock::RevDFTOrder
 * OVERVIEW:      Traverse this node and recurse on its parents in a reverse 
 *                  depth first manner. Records the times at which this node was 
 *                  first visited and last visited
 * PARAMETERS:    first - the number of nodes that have been visited
 *                last - the number of nodes that have been visited for the
 *                  last time during this traversal
 * RETURNS:       the number of nodes (including this one) that were traversed
 *                from this node
 *============================================================================*/
unsigned BasicBlock::RevDFTOrder(int& first, int& last) {
    first++;
    m_DFTrevfirst = first;

    unsigned numTraversed = 1;
    m_iTraversed = true;

    for (std::vector<PBB>::iterator it = m_InEdges.begin(); it != m_InEdges.end();
         it++) {

        PBB parent = *it;
        if (parent->m_iTraversed == false)
            numTraversed = numTraversed + parent->RevDFTOrder(first,last);
    }

    last++;
    m_DFTrevlast = last;

    return numTraversed;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::lessAddress
 * OVERVIEW:        Static comparison function that returns true if the first BB
 *                  has an address less than the second BB.
 * PARAMETERS:      bb1 - first BB
 *                  bb2 - last BB
 * RETURNS:         bb1.address < bb2.address
 *============================================================================*/
bool BasicBlock::lessAddress(PBB bb1, PBB bb2) {
    return bb1->getLowAddr() < bb2->getLowAddr();
}

/*==============================================================================
 * FUNCTION:        BasicBlock::lessFirstDFT
 * OVERVIEW:        Static comparison function that returns true if the first BB
 *                  has DFT first order less than the second BB.
 * PARAMETERS:      bb1 - first BB
 *                  bb2 - last BB
 * RETURNS:         bb1.first_DFS < bb2.first_DFS
 *============================================================================*/
bool BasicBlock::lessFirstDFT(PBB bb1, PBB bb2) {
    return bb1->m_DFTfirst < bb2->m_DFTfirst;
}


/*==============================================================================
 * FUNCTION:        BasicBlock::lessLastDFT
 * OVERVIEW:        Static comparison function that returns true if the first BB
 *                  has DFT first order less than the second BB.
 * PARAMETERS:      bb1 - first BB
 *                  bb2 - last BB
 * RETURNS:         bb1.last_DFS < bb2.last_DFS
 *============================================================================*/
bool BasicBlock::lessLastDFT(PBB bb1, PBB bb2) {
    return bb1->m_DFTlast < bb2->m_DFTlast;
}

/*==============================================================================
 * FUNCTION:        BasicBlock::getCallDest
 * OVERVIEW:        Get the destination of the call, if this is a CALL BB with
 *                    a fixed dest. Otherwise, return -1
 * PARAMETERS:      <none>
 * RETURNS:         Native destination of the call, or -1
 *============================================================================*/
ADDRESS BasicBlock::getCallDest() {
    if (m_nodeType != CALL)
        return (ADDRESS)-1;
    if (m_pRtls->size() == 0)
        return (ADDRESS)-1;
    RTL* lastRtl = m_pRtls->back();
    std::list<Statement*>::reverse_iterator it;
    std::list<Statement*>& sl = lastRtl->getList();
    for (it = sl.rbegin(); it != sl.rend(); it++) {
        if ((*it)->getKind() == STMT_CALL)
            return ((CallStatement*)(*it))->getFixedDest();
    }
    return (ADDRESS)-1;
}

// serialize this BB
bool BasicBlock::serialize(std::ostream &ouf, int &len) {
    std::streampos st = ouf.tellp();

    saveFID(ouf, FID_BB_TYPE);
    saveValue(ouf, (char)m_nodeType);

    int ncount = 0;
    for (std::vector<PBB>::iterator it = m_OutEdges.begin(); it != m_OutEdges.end(); it++) 
        ncount++;
    if (ncount) {
        saveFID(ouf, FID_BB_OUTEDGES);
        saveLen(ouf, ncount * 4);
        saveValue(ouf, ncount, false);
        for (
#ifndef WIN32
            std::vector<PBB>::iterator 
#endif
            it = m_OutEdges.begin(); it != m_OutEdges.end(); it++) {
            saveValue(ouf, (*it)->m_nindex, false);
        }
    }

    if (m_pRtls) {
        for (std::list<RTL*>::iterator it = m_pRtls->begin(); it != m_pRtls->end(); it++) {
            saveFID(ouf, FID_BB_RTL);
            std::streampos pos = ouf.tellp();
            int len = -1;
            saveLen(ouf, -1, true);
            std::streampos posa = ouf.tellp();

            assert((*it)->serialize(ouf, len));

            std::streampos now = ouf.tellp();
            assert((int)(now - posa) == len);
            ouf.seekp(pos);
            saveLen(ouf, len, true);
            ouf.seekp(now);
        }
    }

    saveFID(ouf, FID_BB_END);
    saveLen(ouf, 0);

    len = ouf.tellp() - st;
    return true;
}

// deserialize a BB
bool BasicBlock::deserialize(std::istream &inf) {
    int fid;
    int i, l, j;
    char ch;

    m_nOutEdges.clear();

    while ((fid = loadFID(inf)) != -1 && fid != FID_BB_END) {
        switch (fid) {
            case FID_BB_TYPE:
                loadValue(inf, ch);
                assert(ch <= INVALID);
                m_nodeType = (BBTYPE)ch;
                break;
            case FID_BB_OUTEDGES:
                l = loadLen(inf);
                loadValue(inf, i, false);
                assert(l == i * 4);
                for (j = 0; j < i; j++) {
                    int o;
                    loadValue(inf, o, false);
                    m_nOutEdges.push_back(o);
                }
                break;
            case FID_BB_RTL:
                {
                    int len = loadLen(inf);
                    std::streampos pos = inf.tellg();
                    RTL *rtl = RTL::deserialize(inf);
                    if (rtl) {
                        assert((int)(inf.tellg() - pos) == len);
                        if (m_pRtls == NULL) m_pRtls = new std::list<RTL*>();
                        assert(m_pRtls);
                        m_pRtls->push_back(rtl);
                    } else {
                        // unknown rtl type, skip it
                        inf.seekg(pos + (std::streamoff)len);
                    }
                }
                break;
    /*
    TODO:

    std::list<RTL*>*     m_pRtls;        // Ptr to list of RTLs
    int             m_iLabelNum;    // Nonzero if start of BB needs label
    bool            m_bIncomplete:1;// True if not yet complete
    bool            m_bJumpReqd:1;  // True if jump required for "fall through"
    
    std::vector<PBB>     m_InEdges;      // Vector of in-edges
    std::vector<PBB>     m_OutEdges;     // Vector of out-edges
    int             m_iNumInEdges;  // We need these two because GCC doesn't

    bool            m_iTraversed;   // traversal marker

    Exp* returnLoc;

    */
            default:
                skipFID(inf, fid);
        }
    }
    assert(loadLen(inf) == 0);

    return true;
}

//
// Get First/Next Statement in a BB
//
Statement* BasicBlock::getFirstStmt(rtlit& rit, stmtlistIt& sit) {
    if (m_pRtls == NULL) return NULL;
    rit = m_pRtls->begin();
    while (rit != m_pRtls->end()) {
        RTL* rtl = *rit;
        sit = rtl->getList().begin();
        if (sit != rtl->getList().end())
            return *sit;
        rit++;
    }
    return NULL;
}

Statement* BasicBlock::getNextStmt(rtlit& rit, stmtlistIt& sit) {
    do {
        if (++sit != (*rit)->getList().end())
            return *sit;
        if (++rit == m_pRtls->end())
            break;
        sit = (*rit)->getList().begin();
        if (sit != (*rit)->getList().end())
            return *sit;
    } while (1);
    return NULL;
}

/*
 * Structuring and code generation.
 *
 * This code is whole heartly based on AST by Doug Simon.  Portions may be
 * copyright to him and are available under a BSD style license.  
 *
 * Adapted for Boomerang by Trent Waddington, 20 June 2002.
 *
 */

/* Get the condition */
Exp *BasicBlock::getCond() {
    // the condition will be in the last rtl
    assert(m_pRtls);
    RTL *last = m_pRtls->back();
    // it should contain a BranchStatement
    BranchStatement* bs = (BranchStatement*)last->getHlStmt();
    assert(bs->getKind() == STMT_BRANCH);
    return bs->getCondExpr();
}

void BasicBlock::setCond(Exp *e) {
    // the condition will be in the last rtl
    assert(m_pRtls);
    RTL *last = m_pRtls->back();
    // it should contain a BranchStatement
    std::list<Statement*>& sl = last->getList();
    std::list<Statement*>::reverse_iterator it;
    assert(sl.size());
    for (it = sl.rbegin(); it != sl.rend(); it++) {
        if ((*it)->getKind() == STMT_BRANCH) {
            ((BranchStatement*)(*it))->setCondExpr(e);
            return;
        }
    }
    assert(0);
}

/* Check for branch if equal relation */
bool BasicBlock::isJmpZ(PBB dest) {
    // The condition will be in the last rtl
    assert(m_pRtls);
    RTL *last = m_pRtls->back();
    // it should contain a BranchStatement
    std::list<Statement*>& sl = last->getList();
    std::list<Statement*>::reverse_iterator it;
    assert(sl.size());
    for (it = sl.rbegin(); it != sl.rend(); it++) {
        if ((*it)->getKind() == STMT_BRANCH) {
            BRANCH_TYPE jt = ((BranchStatement*)(*it))->getCond();
            if ((jt != BRANCH_JE) && (jt != BRANCH_JNE)) return false;
            PBB trueEdge = m_OutEdges[0];
            if (jt == BRANCH_JE)
            return dest == trueEdge;
            else {
                PBB falseEdge = m_OutEdges[1];
                return dest == falseEdge;
            }
        }
    }
    assert(0);
    return false;
}

/* Get the loop body */
BasicBlock *BasicBlock::getLoopBody() {
    assert(m_structType == PRETESTLOOP || m_structType == POSTTESTLOOP ||
      m_structType == ENDLESSLOOP);
    assert(m_iNumOutEdges == 2);
    if (m_OutEdges[0] != m_loopFollow)
        return m_OutEdges[0];
    return m_OutEdges[1];
}

bool BasicBlock::isAncestorOf(BasicBlock *other) {           
    return ((loopStamps[0] < other->loopStamps[0] && 
             loopStamps[1] > other->loopStamps[1]) ||
            (revLoopStamps[0] < other->revLoopStamps[0] && 
             revLoopStamps[1] > other->revLoopStamps[1]));
/*    return (m_DFTfirst < other->m_DFTfirst && m_DFTlast > other->m_DFTlast) ||
      (m_DFTrevlast < other->m_DFTrevlast &&
       m_DFTrevfirst > other->m_DFTrevfirst);*/
}

void BasicBlock::simplify() {
    for (std::list<RTL*>::iterator it = m_pRtls->begin();
      it != m_pRtls->end(); it++)
        (*it)->simplify();
}
        
bool BasicBlock::hasBackEdgeTo(BasicBlock* dest) {
//  assert(HasEdgeTo(dest) || dest == this);
    return dest == this || dest->isAncestorOf(this);
}

// Return true if every parent (i.e. forward in edge source) of this node has 
// had its code generated
bool BasicBlock::allParentsGenerated()
{
    for (unsigned int i = 0; i < m_InEdges.size(); i++)
        if (!m_InEdges[i]->hasBackEdgeTo(this) && 
            m_InEdges[i]->traversed != DFS_CODEGEN)
            return false;
    return true;
}

// Emits a goto statement (at the correct indentation level) with the 
// destination label for dest. Also places the label just before the 
// destination code if it isn't already there.  If the goto is to the return 
// block, emit a 'return' instead.  Also, 'continue' and 'break' statements 
// are used instead if possible
void BasicBlock::emitGotoAndLabel(HLLCode *hll, int indLevel, PBB dest)
{
    // is this a goto to the ret block?
    if (dest->getType() == RET) { // WAS: check about size of ret bb
        hll->AddReturnStatement(indLevel, dest->getReturnVal());
    } else { 
        if (loopHead && (loopHead == dest || loopHead->loopFollow == dest)) {
            if (loopHead == dest)
                hll->AddContinue(indLevel);
            else
                hll->AddBreak(indLevel);
        } else {
            hll->AddGoto(indLevel, dest->ord);

            dest->hllLabel = true;
        }
    }
}

// Generates code for each non CTI (except procedure calls) statement within 
// the block.
void BasicBlock::WriteBB(HLLCode *hll, int indLevel)
{
    // allocate space for a label to be generated for this node and add this to
    // the generated code. The actual label can then be generated now or back 
    // patched later
    hll->AddLabel(indLevel, ord);

    if (m_pRtls)
        for (std::list<RTL*>::iterator it = m_pRtls->begin();
          it != m_pRtls->end(); it++) 
            (*it)->generateCode(hll, this, indLevel);

    // save the indentation level that this node was written at
    indentLevel = indLevel;
}

void BasicBlock::generateCode(HLLCode *hll, int indLevel, PBB latch, 
    std::list<PBB> &followSet, std::list<PBB> &gotoSet) {
    // If this is the follow for the most nested enclosing conditional, then
    // don't generate anything. Otherwise if it is in the follow set
    // generate a goto to the follow
    PBB enclFollow = followSet.size() == 0 ? NULL : 
                     followSet.back();

    if (isIn(gotoSet, this) && !isLatchNode() && 
        ((latch && this == latch->loopHead->loopFollow) || 
        !allParentsGenerated())) {
        emitGotoAndLabel(hll, indLevel, this);
        return;
    } else if (isIn(followSet, this)) {
        if (this != enclFollow) {
            emitGotoAndLabel(hll, indLevel, this);
            return;
        } else return;
    }

    // Has this node already been generated?
    if (traversed == DFS_CODEGEN) {
        // this should only occur for a loop over a single block
        assert(sType == Loop && lType == PostTested && latchNode == this);
        return;
    } else
        traversed = DFS_CODEGEN;

    // if this is a latchNode and the current indentation level is
    // the same as the first node in the loop, then this write out its body 
    // and return otherwise generate a goto
    if (isLatchNode())
        if (indLevel == latch->loopHead->indentLevel + 
                        (latch->loopHead->lType == PreTested ? 1 : 0)) {
            WriteBB(hll, indLevel);
            return;
        } else {
            // unset its traversed flag
            traversed = UNTRAVERSED;

            emitGotoAndLabel(hll, indLevel, this);
            return;
        }
        
    switch(sType) {
        case Loop:
        case LoopCond:
            // add the follow of the loop (if it exists) to the follow set
            if (loopFollow)
                followSet.push_back(loopFollow);

            if (lType == PreTested) {
                assert(latchNode->m_OutEdges.size() == 1);

                // write the body of the block (excluding the predicate)
                WriteBB(hll, indLevel);

                // write the 'while' predicate
                Exp *cond = getCond();
                if (m_OutEdges[BTHEN] == loopFollow) {
                    cond = new Unary(opNot, cond);
                    cond = cond->simplify();
                }
                hll->AddPretestedLoopHeader(indLevel, cond);

                // write the code for the body of the loop
                PBB loopBody = (m_OutEdges[BELSE] == loopFollow) ? 
                                m_OutEdges[BTHEN] : m_OutEdges[BELSE];
                loopBody->generateCode(hll, indLevel + 1, latchNode, 
                    followSet, gotoSet);

                // if code has not been generated for the latch node, generate 
                // it now
                if (latchNode->traversed != DFS_CODEGEN) {
                    latchNode->traversed = DFS_CODEGEN;
                    latchNode->WriteBB(hll, indLevel+1);
                }

                // rewrite the body of the block (excluding the predicate) at 
                // the next nesting level after making sure another label 
                // won't be generated
                hllLabel = false;
                WriteBB(hll, indLevel+1);

                // write the loop tail
                hll->AddPretestedLoopEnd(indLevel);
            } else {
                // write the loop header
                if (lType == Endless)
                    hll->AddEndlessLoopHeader(indLevel);
                else
                    hll->AddPosttestedLoopHeader(indLevel);

                // if this is also a conditional header, then generate code 
                // for the conditional. Otherwise generate code for the loop 
                // body.
                if (sType == LoopCond) {
                    // set the necessary flags so that generateCode can 
                    // successfully be called again on this node
                    sType = Cond;
                    traversed = UNTRAVERSED;
                    generateCode(hll, indLevel + 1, latchNode, followSet, 
                                 gotoSet);
                } else {
                    WriteBB(hll, indLevel+1);

                    // write the code for the body of the loop
                    m_OutEdges[0]->generateCode(hll, indLevel + 1, latchNode, 
                                             followSet, gotoSet);
                }

                if (lType == PostTested) {
                    // if code has not been generated for the latch node, 
                    // generate it now
                    if (latchNode->traversed != DFS_CODEGEN) {
                        latchNode->traversed = DFS_CODEGEN;
                        latchNode->WriteBB(hll, indLevel+1);
                    }
                        
                    //hll->AddPosttestedLoopEnd(indLevel, getCond());
                    // MVE: the above seems to fail when there is a call in
                    // the middle of the loop (so loop is 2 BBs)
                    // Just a wild stab:
                    hll->AddPosttestedLoopEnd(indLevel, latchNode->getCond());
                } else {
                    assert(lType == Endless);

                    // if code has not been generated for the latch node, 
                    // generate it now
                    if (latchNode->traversed != DFS_CODEGEN) {
                        latchNode->traversed = DFS_CODEGEN;
                        latchNode->WriteBB(hll, indLevel+1);
                    }

                    // write the closing bracket for an endless loop
                    hll->AddEndlessLoopEnd(indLevel);
                }
            }

            // write the code for the follow of the loop (if it exists)
            if (loopFollow) {
                // remove the follow from the follow set
                followSet.resize(followSet.size()-1);

                if (loopFollow->traversed != DFS_CODEGEN)
                    loopFollow->generateCode(hll, indLevel, latch, followSet,
                                             gotoSet);
                else
                    emitGotoAndLabel(hll, indLevel, loopFollow);
            }
            break;

        case Cond:
        {
            // reset this back to LoopCond if it was originally of this type
            if (latchNode)
                sType = LoopCond;

            // for 2 way conditional headers that are effectively jumps into 
            // or out of a loop or case body, we will need a new follow node
            PBB tmpCondFollow = NULL;

            // keep track of how many nodes were added to the goto set so that 
            // the correct number are removed
            int gotoTotal = 0;

            // add the follow to the follow set if this is a case header
            if (cType == Case)
                followSet.push_back(condFollow);
            else if (cType != Case && condFollow) {
                // For a structured two conditional header, its follow is 
                // added to the follow set
                //myLoopHead = (sType == LoopCond ? this : loopHead);

                if (usType == Structured)
                    followSet.push_back(condFollow);
        
                // Otherwise, for a jump into/outof a loop body, the follow is 
                // added to the goto set.  The temporary follow is set for any 
                // unstructured conditional header branch that is within the 
                // same loop and case.
                else {
                    if (usType == JumpInOutLoop) {
                        // define the loop header to be compared against
                            PBB myLoopHead = (sType == LoopCond ? this : loopHead);
                        gotoSet.push_back(condFollow);
                        gotoTotal++;
        
                        // also add the current latch node, and the loop header
                        // of the follow if they exist
                        if (latch) {
                            gotoSet.push_back(latch);
                            gotoTotal++;
                        }
                        
                        if (condFollow->loopHead && 
                            condFollow->loopHead != myLoopHead) {
                            gotoSet.push_back(condFollow->loopHead);
                            gotoTotal++;
                        }
                    }

                    if (cType == IfThen)
                        tmpCondFollow = m_OutEdges[BELSE];
                    else
                        tmpCondFollow = m_OutEdges[BTHEN];

                    // for a jump into a case, the temp follow is added to the 
                    // follow set
                    if (usType == JumpIntoCase)
                        followSet.push_back(tmpCondFollow);
                }
            }

            // write the body of the block (excluding the predicate)
            WriteBB(hll, indLevel);

            // write the conditional header 
            SWITCH_INFO* psi;
            if (cType == Case) {
                // The CaseStatement will be in the last RTL this BB
                RTL* last = m_pRtls->back();
                CaseStatement* cs = (CaseStatement*)last->getHlStmt();
                psi = cs->getSwitchInfo();
                // Write the switch header (i.e. "switch(var) {")
                hll->AddCaseCondHeader(indLevel, psi->pSwitchVar);
            } else {
                Exp *cond = getCond();
                if (cType == IfElse) {
                    cond = new Unary(opNot, cond);
                    cond = cond->simplify();
                }
                if (cType == IfThenElse)
                    hll->AddIfElseCondHeader(indLevel, cond);
                else
                    hll->AddIfCondHeader(indLevel, cond);
            }

            // write code for the body of the conditional
            if (cType != Case) {
                PBB succ = (cType == IfElse ? m_OutEdges[BELSE] : 
                    m_OutEdges[BTHEN]);

                // emit a goto statement if the first clause has already been 
                // generated or it is the follow of this node's enclosing loop
                if (succ->traversed == DFS_CODEGEN || 
                    (loopHead && succ == loopHead->loopFollow))
                    emitGotoAndLabel(hll, indLevel + 1, succ);
                else        
                    succ->generateCode(hll, indLevel + 1, latch, followSet, 
                                    gotoSet);

                // generate the else clause if necessary
                if (cType == IfThenElse) {
                    // generate the 'else' keyword and matching brackets
                    hll->AddIfElseCondOption(indLevel);

                    succ = m_OutEdges[BELSE];

                    // emit a goto statement if the second clause has already 
                    // been generated
                    if (succ->traversed == DFS_CODEGEN)
                        emitGotoAndLabel(hll, indLevel + 1, succ);
                    else
                        succ->generateCode(hll, indLevel + 1, latch, 
                                        followSet, gotoSet);

                    // generate the closing bracket
                    hll->AddIfElseCondEnd(indLevel);
                       } else {
                    // generate the closing bracket
                    hll->AddIfCondEnd(indLevel);
                }
            } else { // case header
                // generate code for each out branch
                for (unsigned int i = 0; i < m_OutEdges.size(); i++) {
                    // emit a case label
                    // FIXME: Not valid for all switch types
                    Const caseVal(psi->iLower+i);
                    hll->AddCaseCondOption(indLevel, &caseVal);

                    // generate code for the current outedge
                    PBB succ = m_OutEdges[i];
//assert(succ->caseHead == this || succ == condFollow || HasBackEdgeTo(succ));
                    if (succ->traversed == DFS_CODEGEN)
                        emitGotoAndLabel(hll, indLevel + 1, succ);
                    else
                        succ->generateCode(hll, indLevel + 1, latch, 
                            followSet, gotoSet);

                    // generate the 'break' statement
                    hll->AddCaseCondOptionEnd(indLevel);
                }
                // generate the closing bracket
                hll->AddCaseCondEnd(indLevel);
            }


            // do all the follow stuff if this conditional had one
            if (condFollow) {
                // remove the original follow from the follow set if it was 
                // added by this header
                if (usType == Structured || usType == JumpIntoCase) {
                    assert(gotoTotal == 0);
                    followSet.resize(followSet.size()-1);
                } else // remove all the nodes added to the goto set
                    for (int i = 0; i < gotoTotal; i++)
                        gotoSet.resize(gotoSet.size()-1);

                // do the code generation (or goto emitting) for the new 
                // conditional follow if it exists, otherwise do it for the 
                // original follow
                if (!tmpCondFollow)
                    tmpCondFollow = condFollow;
                        
                if (tmpCondFollow->traversed == DFS_CODEGEN)
                    emitGotoAndLabel(hll, indLevel, tmpCondFollow);
                else
                    tmpCondFollow->generateCode(hll, indLevel, latch,
                        followSet, gotoSet);
            }
            break;
        } 
        case Seq:
            // generate code for the body of this block
            WriteBB(hll, indLevel);

            // return if this is the 'return' block (i.e. has no out edges)
            // after emmitting a 'return' statement
            if (getType() == RET) {
                hll->AddReturnStatement(indLevel, getReturnVal());
                return;
            }

            // return if this doesn't have any out edges (emit a warning)
            if (m_OutEdges.size() == 0) {
                std::cerr << "WARNING: no out edge for BB: " << std::endl;
                this->print(std::cerr, false);
                std::cerr << std::endl;
                return;
            }

            // generate code for its successor if it hasn't already been 
            // visited and is in the same loop/case and is not the latch 
            // for the current most enclosing loop.  The only exception 
            // for generating it when it is not in the same loop is when 
            // it is only reached from this node
            PBB child = m_OutEdges[0];
            if (child->traversed == DFS_CODEGEN || 
                ((child->loopHead != loopHead) && 
                 (!child->allParentsGenerated() || 
                  isIn(followSet, child))) ||
                (latch && latch->loopHead->loopFollow == child) ||
                !(caseHead == child->caseHead || 
                  (caseHead && child == caseHead->condFollow)))
                emitGotoAndLabel(hll, indLevel, m_OutEdges[0]);
            else
                m_OutEdges[0]->generateCode(hll, indLevel, latch,
                     followSet, gotoSet);
            break;
    }
}

// Get the destination proc
// Note: this must be a call BB!
Proc* BasicBlock::getDestProc() {
    // The last Statement of the last RTL should be a CallStatement
    CallStatement* call = (CallStatement*)
      (m_pRtls->back()->getHlStmt());
    assert(call->getKind() == STMT_CALL);
    Proc* proc = call->getDestProc();
    if (proc == NULL) {
        std::cerr << "Indirect calls not handled yet\n";
        assert(0);
    }
    return proc;
}

void BasicBlock::setLoopStamps(int &time, std::vector<PBB> &order) {
    // timestamp the current node with the current time and set its traversed 
    // flag
    traversed = DFS_LNUM;
    loopStamps[0] = time;

    // recurse on unvisited children and set inedges for all children
    for (unsigned int i = 0; i < m_OutEdges.size(); i++) {
        // set the in edge from this child to its parent (the current node)
        // (not done here, might be a problem)
        // outEdges[i]->inEdges.Add(this);

        // recurse on this child if it hasn't already been visited
        if (m_OutEdges[i]->traversed != DFS_LNUM)
            m_OutEdges[i]->setLoopStamps(++time, order);
    }

    // set the the second loopStamp value
    loopStamps[1] = ++time;

    // add this node to the ordering structure as well as recording its 
    // position within the ordering
    ord = order.size();
    order.push_back(this);
}

void BasicBlock::setRevLoopStamps(int &time)
{
    // timestamp the current node with the current time and set its traversed 
    // flag
    traversed = DFS_RNUM;
    revLoopStamps[0] = time;

    // recurse on the unvisited children in reverse order
    for (int i = m_OutEdges.size() - 1; i >= 0; i--) {
        // recurse on this child if it hasn't already been visited
        if (m_OutEdges[i]->traversed != DFS_RNUM)
            m_OutEdges[i]->setRevLoopStamps(++time);
    }

    // set the the second loopStamp value
    revLoopStamps[1] = ++time;
}

void BasicBlock::setRevOrder(std::vector<PBB> &order)
{
    // Set this node as having been traversed during the post domimator 
    // DFS ordering traversal
    traversed = DFS_PDOM;
        
    // recurse on unvisited children 
    for (unsigned int i = 0; i < m_InEdges.size(); i++)
        if (m_InEdges[i]->traversed != DFS_PDOM)
            m_InEdges[i]->setRevOrder(order);

    // add this node to the ordering structure and record the post dom. order
    // of this node as its index within this ordering structure
    revOrd = order.size();
    order.push_back(this);
}

void BasicBlock::setCaseHead(PBB head, PBB follow) 
{
    assert(!caseHead);

    traversed = DFS_CASE;

    // don't tag this node if it is the case header under investigation 
    if (this != head)
        caseHead = head;

    // if this is a nested case header, then it's member nodes will already 
    // have been tagged so skip straight to its follow
    if (getType() == NWAY && this != head) {
        if (condFollow->traversed != DFS_CASE && condFollow != follow)
            condFollow->setCaseHead(head, follow);
    } else
        // traverse each child of this node that:
        //   i) isn't on a back-edge,
        //  ii) hasn't already been traversed in a case tagging traversal and,
        // iii) isn't the follow node.
        for (unsigned int i = 0; i < m_OutEdges.size(); i++)
            if (!hasBackEdgeTo(m_OutEdges[i]) && 
                m_OutEdges[i]->traversed != DFS_CASE && 
                m_OutEdges[i] != follow)
                m_OutEdges[i]->setCaseHead(head, follow);
}

void BasicBlock::setStructType(structType s) {
    // if this is a conditional header, determine exactly which type of 
    // conditional header it is (i.e. switch, if-then, if-then-else etc.)
    if (s == Cond) {
        if (getType() == NWAY) 
            cType = Case;
        else if (m_OutEdges[BELSE] == condFollow)
            cType = IfThen;
        else if (m_OutEdges[BTHEN] == condFollow)
            cType = IfElse;
        else
            cType = IfThenElse;
    }

    sType = s;
}

void BasicBlock::setUnstructType(unstructType us) {
    assert((sType == Cond || sType == LoopCond) && cType != Case);
    usType = us;
}

unstructType BasicBlock::getUnstructType() {
    assert((sType == Cond || sType == LoopCond) && cType != Case);
    return usType;
}

void BasicBlock::setLoopType(loopType l) {
    assert (sType == Loop || sType == LoopCond);
    lType = l;

    // set the structured class (back to) just Loop if the loop type is 
    // PreTested OR it's PostTested and is a single block loop
    if (lType == PreTested || (lType == PostTested && this == latchNode))
        sType = Loop;
}

loopType BasicBlock::getLoopType() {
    assert (sType == Loop || sType == LoopCond);
    return lType;
}

void BasicBlock::setCondType(condType c) {
    assert (sType == Cond || sType == LoopCond);
    cType = c;
}

condType BasicBlock::getCondType() {
    assert (sType == Cond || sType == LoopCond);
    return cType;
}

bool BasicBlock::inLoop(PBB header, PBB latch) {
   assert(header->latchNode == latch);
   assert(header == latch || 
          ((header->loopStamps[0] > latch->loopStamps[0] && 
            latch->loopStamps[1] > header->loopStamps[1]) ||
          (header->loopStamps[0] < latch->loopStamps[0] && 
           latch->loopStamps[1] < header->loopStamps[1])));
   // this node is in the loop if it is the latch node OR
   // this node is within the header and the latch is within this when using 
   // the forward loop stamps OR
   // this node is within the header and the latch is within this when using 
   // the reverse loop stamps 
   return this == latch ||
          (header->loopStamps[0] < loopStamps[0] && 
           loopStamps[1] < header->loopStamps[1] &&
           loopStamps[0] < latch->loopStamps[0] && 
           latch->loopStamps[1] < loopStamps[1]) ||
          (header->revLoopStamps[0] < revLoopStamps[0] && 
           revLoopStamps[1] < header->revLoopStamps[1] &&
           revLoopStamps[0] < latch->revLoopStamps[0] && 
           latch->revLoopStamps[1] < revLoopStamps[1]);
}

// Return the first statement number as a string.
// Used in dotty file generation
char* BasicBlock::getStmtNumber() {
    static char ret[12];
    rtlit rit; stmtlistIt sit;
    Statement* first = getFirstStmt(rit, sit);
    if (first)
        sprintf(ret, "%d", first->getNumber());
    else
        sprintf(ret, "bb%x", (unsigned)this);
    return ret;
} 

// Prepend an expression (usually an assignment representing a phi function)
// Proc is the enclosing Proc
void BasicBlock::prependStmt(Statement* s, UserProc* proc) {
    // Check the first RTL (if any)
    s->setBB(this);
    s->setProc(proc);
    if (m_pRtls->size()) {
        RTL* rtl = m_pRtls->front();
        if (rtl->getAddress() == 0) {
            // Append to this RTL
            rtl->appendStmt(s);
            return;
        }
    }
    // Otherwise, prepent a new RTL
    std::list<Statement*> listStmt;
    listStmt.push_back(s);
    RTL* rtl = new RTL(0, &listStmt);
    m_pRtls->push_front(rtl);
}

////////////////////////////////////////////////////


bool BasicBlock::calcLiveness(igraph& ig, int& localNum) {
    // Start with the liveness at the bottom of the BB
    LocationSet liveLocs;
    getLiveOut(liveLocs);
    // For each RTL in this BB
    std::list<RTL*>::reverse_iterator rit;
    for (rit = m_pRtls->rbegin(); rit != m_pRtls->rend(); rit++) {
        std::list<Statement*>& stmts = (*rit)->getList();
        std::list<Statement*>::reverse_iterator sit;
        // For each statement this RTL
        for (sit = stmts.rbegin(); sit != stmts.rend(); sit++) {
            Statement* s = *sit;
            LocationSet defs;
            s->getDefinitions(defs);
            // The definitions don't have refs yet
            defs.addSubscript(s);
            // Definitions kill uses
            liveLocs.makeDiff(defs);
            // Phi functions are a special case. The operands of phi functions
            // are uses, but they don't interfere with each other (since they
            // come via different BBs). However, we don't want to put these
            // uses into liveLocs, because then the livenesses will flow to
            // all predecessors. Only the appropriate livenesses from the
            // appropriate phi parameter should flow to the predecessor.
            // This is done in getLiveOut()
            if (s->isPhi()) continue;
            // Check for livenesses that overlap
            LocationSet uses;
            s->addUsedLocs(uses);
            LocSetIter uu;  
            // For each new use
            for (Exp* u = uses.getFirst(uu); u; u = uses.getNext(uu)) {
                // Only interested in subscripted vars
                if (!u->isSubscript()) continue;
                // Interference if we can find a live variable which differs
                // only in the reference
                if (liveLocs.findDifferentRef((RefExp*)u)) {
                    // We have an interference. Record it, but only if new
                    igraph::iterator gg = ig.find(u);
                    if (gg == ig.end()) {
                        ig[u] = localNum++;
                        if (VERBOSE || Boomerang::get()->debugLiveness)
                            std::cerr << "Interference of " << u <<
                            ", assigned local" << std::dec << localNum-1
                            << "\n";
                    }
                // Don't add the interfering variable to liveLocs, otherwise
                // we could register other interferences that will not exist
                // once this one is renamed
                } else
                    // Add the uses one at a time. Note: don't use makeUnion,
                    // because then we don't discover interferences from the
                    // same statement, e.g.
                    // blah := r24{2} + r24{3}
                    liveLocs.insert(u);
            }
            if (Boomerang::get()->debugLiveness)
                std::cerr << " ## Liveness: at top of " << s <<
                  ", liveLocs is " << liveLocs.prints() << "\n";
        }
    }
    // liveIn is what we calculated last time
    if (!(liveLocs == liveIn)) {
        liveIn = liveLocs;
        return true;        // A change
    } else
        // No change
        return false;
}

// Locations that are live at the end of this BB are the union of the
// locations that are live at the start of its successors
void BasicBlock::getLiveOut(LocationSet &liveout) {
    liveout.clear();
    for (unsigned i = 0; i < m_OutEdges.size(); i++) {
        PBB currBB = m_OutEdges[i];
        // First add the non-phi liveness
        liveout.makeUnion(currBB->liveIn);
        int j = currBB->whichPred(this);
        // The first RTL will have the phi functions, if any
        if (currBB->m_pRtls->size() == 0)
            continue;
        RTL* phiRtl = currBB->m_pRtls->front();
        std::list<Statement*>& stmts = phiRtl->getList();
        std::list<Statement*>::iterator it;
        for (it = stmts.begin(); it != stmts.end(); it++) {
            // Only interested in phi assignments. Note that it is possible
            // that some phi assignments have been converted to ordinary
            // assignments. So the below is a continue, not a break.
            if (!(*it)->isPhi()) continue;
            PhiExp* phi = (PhiExp*)((Assign*)(*it))->getRight();
            // Get the jth operand to the phi function; it has a use
            // from BB *this
            Statement* def = phi->getAt(j);
            // This will leak
            RefExp* r = new RefExp((*it)->getLeft()->clone(), def);
            liveout.insert(r);
            if (Boomerang::get()->debugLiveness)
                std::cerr << " ## Liveness: adding " << std::dec << r <<
                  " due to ref to phi " << *it << " in BB at " << std::hex <<
                  getLowAddr() << "\n";
        }
    }
}

// Basically the "whichPred" function as per Briggs, Cooper, et al
// (and presumably "Cryton, Ferante, Rosen, Wegman, and Zadek").
// Return -1 if not found
int BasicBlock::whichPred(PBB pred) {
    int n = m_InEdges.size();
    for (int i=0; i < n; i++) {
        if (m_InEdges[i] == pred)
            return i;
    }
    assert(0);
    return -1;
}
