//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CJoinOrderTest.cpp
//
//	@doc:
//		Test for join ordering
//---------------------------------------------------------------------------
#include "gpos/io/COstreamString.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CQueryContext.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/ops.h"

#include "gpopt/xforms/CJoinOrder.h"
#include "gpopt/xforms/CJoinOrderMinCard.h"

#include "unittest/base.h"
#include "unittest/gpopt/xforms/CJoinOrderTest.h"
#include "unittest/gpopt/CTestUtils.h"

ULONG CJoinOrderTest::m_ulTestCounter = 0;  // start from first test

	// minidump files
const CHAR *rgszJoinOrderFileNames[] =
{
	"../data/dxl/minidump/JoinOptimizationLevelGreedyNonPartTblInnerJoin.mdp",
	"../data/dxl/minidump/JoinOptimizationLevelQueryNonPartTblInnerJoin.mdp"
};

//---------------------------------------------------------------------------
//	@function:
//		CJoinOrderTest::EresUnittest
//
//	@doc:
//		Unittest for predicate utilities
//
//---------------------------------------------------------------------------
GPOS_RESULT
CJoinOrderTest::EresUnittest()
{

	CUnittest rgut[] =
		{
		GPOS_UNITTEST_FUNC(EresUnittest_ExpandMinCard),
		GPOS_UNITTEST_FUNC(EresUnittest_RunTests)
		};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}


//---------------------------------------------------------------------------
//	@function:
//		CJoinOrderTest::EresUnittest_ExpandMinCard
//
//	@doc:
//		Expansion expansion based on cardinality of intermediate results
//
//---------------------------------------------------------------------------
GPOS_RESULT
CJoinOrderTest::EresUnittest_ExpandMinCard()
{
	CAutoMemoryPool amp;
	IMemoryPool *pmp = amp.Pmp();

	// array of relation names
	CWStringConst rgscRel[] =
	{
		GPOS_WSZ_LIT("Rel10"),
		GPOS_WSZ_LIT("Rel3"),
		GPOS_WSZ_LIT("Rel4"),
		GPOS_WSZ_LIT("Rel6"),
		GPOS_WSZ_LIT("Rel7"),
		GPOS_WSZ_LIT("Rel8"),
		GPOS_WSZ_LIT("Rel12"),
		GPOS_WSZ_LIT("Rel13"),
		GPOS_WSZ_LIT("Rel5"),
		GPOS_WSZ_LIT("Rel14"),
		GPOS_WSZ_LIT("Rel15"),
		GPOS_WSZ_LIT("Rel1"),
		GPOS_WSZ_LIT("Rel11"),
		GPOS_WSZ_LIT("Rel2"),
		GPOS_WSZ_LIT("Rel9"),
	};

	// array of relation IDs
	ULONG rgulRel[] =
	{
		GPOPT_TEST_REL_OID10,
		GPOPT_TEST_REL_OID3,
		GPOPT_TEST_REL_OID4,
		GPOPT_TEST_REL_OID6,
		GPOPT_TEST_REL_OID7,
		GPOPT_TEST_REL_OID8,
		GPOPT_TEST_REL_OID12,
		GPOPT_TEST_REL_OID13,
		GPOPT_TEST_REL_OID5,
		GPOPT_TEST_REL_OID14,
		GPOPT_TEST_REL_OID15,
		GPOPT_TEST_REL_OID1,
		GPOPT_TEST_REL_OID11,
		GPOPT_TEST_REL_OID2,
		GPOPT_TEST_REL_OID9,
	};

	const ULONG ulRels = GPOS_ARRAY_SIZE(rgscRel);
	GPOS_ASSERT(GPOS_ARRAY_SIZE(rgulRel) == ulRels);

	// setup a file-based provider
	CMDProviderMemory *pmdp = CTestUtils::m_pmdpf;
	pmdp->AddRef();
	CMDAccessor mda(pmp, CMDCache::Pcache());
	mda.RegisterProvider(CTestUtils::m_sysidDefault, pmdp);

	{
		// install opt context in TLS
		CAutoOptCtxt aoc
				(
				pmp,
				&mda,
				NULL,  /* pceeval */
				CTestUtils::Pcm(pmp)
				);

		CExpression *pexprNAryJoin =
				CTestUtils::PexprLogicalNAryJoin(pmp, rgscRel, rgulRel, ulRels, false /*fCrossProduct*/);

		// derive stats on input expression
		CExpressionHandle exprhdl(pmp);
		exprhdl.Attach(pexprNAryJoin);
		exprhdl.DeriveStats(pmp, pmp, NULL /*prprel*/, NULL /*pdrgpstatCtxt*/);

		DrgPexpr *pdrgpexpr = GPOS_NEW(pmp) DrgPexpr(pmp);
		for (ULONG ul = 0; ul < ulRels; ul++)
		{
			CExpression *pexprChild = (*pexprNAryJoin)[ul];
			pexprChild->AddRef();
			pdrgpexpr->Append(pexprChild);
		}
		DrgPexpr *pdrgpexprPred = CPredicateUtils::PdrgpexprConjuncts(pmp, (*pexprNAryJoin)[ulRels]);
		pdrgpexpr->AddRef();
		pdrgpexprPred->AddRef();
		CJoinOrderMinCard jomc(pmp, pdrgpexpr, pdrgpexprPred);
		CExpression *pexprResult = jomc.PexprExpand();
		{
			CAutoTrace at(pmp);
			at.Os() << std::endl << "INPUT:" << std::endl << *pexprNAryJoin << std::endl;
			at.Os() << std::endl << "OUTPUT:" << std::endl << *pexprResult << std::endl;
		}
		pexprResult->Release();
		pexprNAryJoin->Release();
		pdrgpexpr->Release();
		pdrgpexprPred->Release();
	}

	return GPOS_OK;
}

//	run all Minidump-based tests with plan matching
GPOS_RESULT
CJoinOrderTest::EresUnittest_RunTests()
{
	return CTestUtils::EresUnittest_RunTests
	(
	 rgszJoinOrderFileNames,
	 &m_ulTestCounter,
	 GPOS_ARRAY_SIZE(rgszJoinOrderFileNames)
	 );
}
// EOF
